#include "WiFiSniffer.h"

// ─── Static member definitions ───────────────────────────────────────────────

unsigned long WiFiSniffer::_total     = 0;
unsigned long WiFiSniffer::_beacons   = 0;
unsigned long WiFiSniffer::_deauths   = 0;
unsigned long WiFiSniffer::_probeReqs = 0;
unsigned long WiFiSniffer::_probeResps= 0;
unsigned long WiFiSniffer::_dataPkts  = 0;

PacketEntry WiFiSniffer::_log[MAX_PACKET_LOG];
int WiFiSniffer::_logIndex = 0;
int WiFiSniffer::_logCount = 0;

// Per-device tracking
DeviceStats WiFiSniffer::_devices[MAX_UNIQUE_MACS];
uint8_t WiFiSniffer::_deviceCount = 0;

// Channel hopping state
bool WiFiSniffer::_hopping = false;
uint8_t WiFiSniffer::_currentChannel = 1;
uint8_t WiFiSniffer::_hopIndex = 0;
unsigned long WiFiSniffer::_lastHop = 0;

// Non-overlapping 2.4 GHz channels — covers ~90% of all WiFi devices
const uint8_t WiFiSniffer::_scanChannels[NUM_SCAN_CHANNELS] = { 1, 6, 11 };
portMUX_TYPE WiFiSniffer::_mux = portMUX_INITIALIZER_UNLOCKED;

// ─── Known camera manufacturer OUI prefixes ─────────────────────────────────
// These are the first 3 bytes of MAC addresses registered to companies
// that make surveillance cameras and WiFi camera modules.
// Source: IEEE OUI database + academic research (DeWiCam, LocCams)

struct OUIEntry {
    uint8_t prefix[3];
    const char* vendor;
};

// Keep this table small — only the most common camera OUIs.
// Sorted roughly by likelihood of being a hidden camera.
static const OUIEntry CAMERA_OUIS[] = {
    // Espressif — ESP32-CAM modules, very common in cheap spy cameras
    { {0x18, 0xFE, 0x34}, "Espressif" },
    { {0x5C, 0xCF, 0x7F}, "Espressif" },
    { {0xCC, 0x50, 0xE3}, "Espressif" },
    { {0x84, 0xF3, 0xEB}, "Espressif" },

    // Hikvision — world's largest surveillance camera manufacturer
    { {0x18, 0x68, 0xCB}, "Hikvision" },
    { {0x28, 0x57, 0xBE}, "Hikvision" },
    { {0x44, 0x19, 0xB6}, "Hikvision" },
    { {0x54, 0xC4, 0x15}, "Hikvision" },
    { {0xC0, 0x56, 0xE3}, "Hikvision" },
    { {0xBC, 0xAD, 0x28}, "Hikvision" },

    // Dahua — another major surveillance manufacturer
    { {0x3C, 0xEF, 0x8C}, "Dahua" },
    { {0x40, 0x2C, 0x76}, "Dahua" },
    { {0xA0, 0xBD, 0x1D}, "Dahua" },

    // Wyze / Tuya Smart — cheap smart cameras sold on Amazon
    { {0x2C, 0xAA, 0x8E}, "Wyze" },
    { {0x7C, 0x78, 0xB2}, "Tuya" },
    { {0xD8, 0x1E, 0xA0}, "Tuya" },
};

#define NUM_CAMERA_OUIS (sizeof(CAMERA_OUIS) / sizeof(CAMERA_OUIS[0]))

// ─── Constructor ─────────────────────────────────────────────────────────────

WiFiSniffer::WiFiSniffer(uint8_t channel)
    : _channel(channel) {}

// ─── Start sniffing ──────────────────────────────────────────────────────────

void WiFiSniffer::begin() {
    Serial.println(F("=== ESP32 WiFi Sniffer ==="));

    // Enable promiscuous mode on top of existing WiFi station connection
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&WiFiSniffer::snifferCallback);

    _currentChannel = _channel;
    esp_wifi_set_channel(_channel, WIFI_SECOND_CHAN_NONE);

    Serial.print(F("Sniffing on channel "));
    Serial.println(_channel);
}

// ─── Reset everything ────────────────────────────────────────────────────────

void WiFiSniffer::resetCounters() {
    taskENTER_CRITICAL(&_mux);
    _total = _beacons = _deauths = _probeReqs = _probeResps = _dataPkts = 0;
    _logIndex = _logCount = 0;
    _deviceCount = 0;
    taskEXIT_CRITICAL(&_mux);
}

// ─── Getters for summary dashboard ──────────────────────────────────────────

unsigned long WiFiSniffer::getTotal()      { taskENTER_CRITICAL(&_mux); unsigned long v = _total;       taskEXIT_CRITICAL(&_mux); return v; }
unsigned long WiFiSniffer::getBeacons()    { taskENTER_CRITICAL(&_mux); unsigned long v = _beacons;     taskEXIT_CRITICAL(&_mux); return v; }
unsigned long WiFiSniffer::getDeauths()    { taskENTER_CRITICAL(&_mux); unsigned long v = _deauths;     taskEXIT_CRITICAL(&_mux); return v; }
unsigned long WiFiSniffer::getProbeReqs()  { taskENTER_CRITICAL(&_mux); unsigned long v = _probeReqs;   taskEXIT_CRITICAL(&_mux); return v; }
unsigned long WiFiSniffer::getProbeResps() { taskENTER_CRITICAL(&_mux); unsigned long v = _probeResps;  taskEXIT_CRITICAL(&_mux); return v; }
unsigned long WiFiSniffer::getDataPkts()   { taskENTER_CRITICAL(&_mux); unsigned long v = _dataPkts;    taskEXIT_CRITICAL(&_mux); return v; }
unsigned int  WiFiSniffer::getUniqueMacs() { taskENTER_CRITICAL(&_mux); unsigned int  v = _deviceCount; taskEXIT_CRITICAL(&_mux); return v; }

// ─── Packet log access ──────────────────────────────────────────────────────

int WiFiSniffer::getLogCount() { taskENTER_CRITICAL(&_mux); int v = _logCount; taskEXIT_CRITICAL(&_mux); return v; }

PacketEntry WiFiSniffer::getLogEntry(int index) {
    taskENTER_CRITICAL(&_mux);
    int start = (_logCount < MAX_PACKET_LOG) ? 0 : _logIndex;
    int pos = (start + index) % MAX_PACKET_LOG;
    PacketEntry e = _log[pos];
    taskEXIT_CRITICAL(&_mux);
    return e;
}

// ─── OUI lookup — check if a MAC belongs to a known camera manufacturer ─────

bool WiFiSniffer::isKnownCameraOUI(const uint8_t mac[6]) {
    for (int i = 0; i < (int)NUM_CAMERA_OUIS; i++) {
        if (memcmp(mac, CAMERA_OUIS[i].prefix, 3) == 0) return true;
    }
    return false;
}

const char* WiFiSniffer::getOUIVendor(const uint8_t mac[6]) {
    for (int i = 0; i < (int)NUM_CAMERA_OUIS; i++) {
        if (memcmp(mac, CAMERA_OUIS[i].prefix, 3) == 0) {
            return CAMERA_OUIS[i].vendor;
        }
    }
    return nullptr;
}

// ─── Per-device tracking ────────────────────────────────────────────────────
// Called from the sniffer callback for every packet.
// Tracks per-device packet counts, RSSI (smoothed), frame sizes, and OUI.
//
// Frame size analysis (from research):
//   WiFi cameras use VBR video codecs that produce a bimodal packet size
//   distribution: large frames (~1000-1500 bytes, I-frame fragments near MTU)
//   and small frames (~100-600 bytes, P/B-frame fragments). Normal devices
//   don't produce this pattern consistently.

void WiFiSniffer::trackDevice(const uint8_t mac[6], int8_t rssi, PktType pktType, uint16_t frameLen) {
    // Search for existing device
    for (uint8_t i = 0; i < _deviceCount; i++) {
        if (memcmp(_devices[i].mac, mac, 6) == 0) {
            // Found — update stats
            DeviceStats& d = _devices[i];

            // Smooth the RSSI using exponential moving average.
            d.rssi = (int8_t)(RSSI_SMOOTH_ALPHA * rssi + (1.0f - RSSI_SMOOTH_ALPHA) * d.rssi);
            d.channel = _currentChannel;  // Keep channel fresh after hopping

            d.totalPkts++;
            if (pktType == PKT_DATA) d.dataPkts++;

            // Track frame size distribution (bimodal = camera signature)
            if (frameLen > 1000)                  d.largePkts++;
            else if (frameLen >= 100 && frameLen <= 600) d.smallPkts++;

            d.lastSeen = millis();
            return;
        }
    }

    // New device — add if there's room
    if (_deviceCount < MAX_UNIQUE_MACS) {
        DeviceStats& d = _devices[_deviceCount];
        memcpy(d.mac, mac, 6);
        d.rssi       = rssi;
        d.totalPkts  = 1;
        d.dataPkts   = (pktType == PKT_DATA) ? 1 : 0;
        d.largePkts  = (frameLen > 1000) ? 1 : 0;
        d.smallPkts  = (frameLen >= 100 && frameLen <= 600) ? 1 : 0;
        d.firstSeen  = millis();
        d.lastSeen   = millis();
        d.channel    = _currentChannel;
        d.knownCamOUI = isKnownCameraOUI(mac);
        _deviceCount++;
    }
}

// ─── Device access ──────────────────────────────────────────────────────────

int WiFiSniffer::getDeviceCount() {
    taskENTER_CRITICAL(&_mux);
    int count = _deviceCount;
    taskEXIT_CRITICAL(&_mux);
    return count;
}

DeviceStats WiFiSniffer::getDevice(int index) {
    taskENTER_CRITICAL(&_mux);
    DeviceStats d = {};
    if (index >= 0 && index < _deviceCount) {
        d = _devices[index];
    }
    taskEXIT_CRITICAL(&_mux);
    return d;
}

// ─── Suspicious device detection ────────────────────────────────────────────
// A device is "suspicious" (potential hidden camera) if ANY of these are true:
//
// 1. HIGH DATA RATIO: >60% of its packets are data frames AND it has sent
//    at least 20 packets. Cameras stream constantly = mostly data frames.
//
// 2. BIMODAL FRAME SIZES: It has both large (>1000 byte) and small (100-600
//    byte) frames with at least 5 of each. This is the I-frame / P-frame
//    signature of video codecs (H.264/H.265), even through encryption.
//
// 3. KNOWN CAMERA OUI: The MAC prefix matches Hikvision, Dahua, Espressif
//    (ESP32-CAM), Wyze, or Tuya AND it's sending data frames.
//    OUI alone isn't enough (Espressif makes smart bulbs too), so we require
//    at least some data traffic.

bool WiFiSniffer::isDeviceSuspicious(const DeviceStats& d) {
    if (d.totalPkts >= SUSPECT_MIN_PACKETS) {
        if ((d.dataPkts * 100 / d.totalPkts) >= SUSPECT_DATA_RATIO) return true;
    }
    if (d.largePkts >= SUSPECT_BIMODAL_MIN && d.smallPkts >= SUSPECT_BIMODAL_MIN) return true;
    if (d.knownCamOUI && d.dataPkts > 5) return true;
    return false;
}

bool WiFiSniffer::isSuspicious(int index) {
    taskENTER_CRITICAL(&_mux);
    if (index < 0 || index >= _deviceCount) { taskEXIT_CRITICAL(&_mux); return false; }
    bool sus = isDeviceSuspicious(_devices[index]);
    taskEXIT_CRITICAL(&_mux);
    return sus;
}

int WiFiSniffer::getSuspiciousCount() {
    taskENTER_CRITICAL(&_mux);
    int count = 0;
    for (int i = 0; i < _deviceCount; i++) {
        if (isDeviceSuspicious(_devices[i])) count++;
    }
    taskEXIT_CRITICAL(&_mux);
    return count;
}

int WiFiSniffer::getStrongestSuspicious() {
    taskENTER_CRITICAL(&_mux);
    int bestIndex = -1;
    int8_t bestRssi = -127;
    for (int i = 0; i < _deviceCount; i++) {
        if (isDeviceSuspicious(_devices[i]) && _devices[i].rssi > bestRssi) {
            bestRssi = _devices[i].rssi;
            bestIndex = i;
        }
    }
    taskEXIT_CRITICAL(&_mux);
    return bestIndex;
}

// ─── Channel hopping ────────────────────────────────────────────────────────
// Scans channels 1, 6, 11 (non-overlapping) to discover devices on other
// channels. Call channelHop() from loop() — it handles the dwell timing.
// Once a suspicious device is found, call lockChannel() to stay on its
// channel for maximum RSSI accuracy.

void WiFiSniffer::channelHop() {
    if (!_hopping) return;
    if (millis() - _lastHop < CHANNEL_DWELL_MS) return;

    _hopIndex = (_hopIndex + 1) % NUM_SCAN_CHANNELS;
    _currentChannel = _scanChannels[_hopIndex];
    esp_wifi_set_channel(_currentChannel, WIFI_SECOND_CHAN_NONE);
    _lastHop = millis();
}

bool WiFiSniffer::isHopping() { return _hopping; }

void WiFiSniffer::lockChannel(uint8_t ch) {
    _hopping = false;
    _currentChannel = ch;
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
    Serial.print(F("Locked on channel "));
    Serial.println(ch);
}

void WiFiSniffer::startHopping() {
    _hopping = true;
    _hopIndex = 0;
    _currentChannel = _scanChannels[0];
    esp_wifi_set_channel(_currentChannel, WIFI_SECOND_CHAN_NONE);
    _lastHop = millis();
    Serial.println(F("Channel hopping started (1, 6, 11)"));
}

uint8_t WiFiSniffer::getCurrentChannel() { return _currentChannel; }

// ─── Utility ────────────────────────────────────────────────────────────────

const char* WiFiSniffer::typeToString(PktType t) {
    switch (t) {
        case PKT_BEACON:     return "Beacon";
        case PKT_DEAUTH:     return "Deauth";
        case PKT_PROBE_REQ:  return "Probe Req";
        case PKT_PROBE_RESP: return "Probe Resp";
        case PKT_DATA:       return "Data";
        default:             return "Other";
    }
}

String WiFiSniffer::macToString(const uint8_t mac[6]) {
    char buf[18];
    sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}

// ─── Main callback — called for every WiFi packet the radio picks up ────────

void WiFiSniffer::snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    const wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    const uint8_t* frame = pkt->payload;
    int8_t rssi = pkt->rx_ctrl.rssi;
    uint16_t frameLen = pkt->rx_ctrl.sig_len;

    // Validate minimum frame length (need at least 16 bytes for MAC extraction)
    if (frameLen < 16) return;

    // ── Classify frame and extract MAC OUTSIDE the lock (read-only on pkt buffer) ──
    uint8_t fc = frame[0];
    PktType pktType;

    switch (fc) {
        case 0x80: pktType = PKT_BEACON;     break;
        case 0x40: pktType = PKT_PROBE_REQ;  break;
        case 0x50: pktType = PKT_PROBE_RESP; break;
        case 0xC0:
        case 0xA0: pktType = PKT_DEAUTH;     break;
        case 0x08:
        case 0x88: pktType = PKT_DATA;       break;
        default:   pktType = PKT_OTHER;       break;
    }

    // Copy source MAC to stack (safe to read from pkt buffer without lock)
    uint8_t srcMac[6];
    memcpy(srcMac, &frame[10], 6);
    unsigned long now = millis();

    // ── Minimal critical section: only shared state updates ──
    taskENTER_CRITICAL(&_mux);

    _total++;
    switch (pktType) {
        case PKT_BEACON:     _beacons++;   break;
        case PKT_PROBE_REQ:  _probeReqs++; break;
        case PKT_PROBE_RESP: _probeResps++;break;
        case PKT_DEAUTH:     _deauths++;   break;
        case PKT_DATA:       _dataPkts++;  break;
        default: break;
    }

    trackDevice(srcMac, rssi, pktType, frameLen);

    PacketEntry& entry = _log[_logIndex];
    entry.timestamp = now;
    memcpy(entry.srcMac, srcMac, 6);
    entry.rssi = rssi;
    entry.type = pktType;
    _logIndex = (_logIndex + 1) % MAX_PACKET_LOG;
    if (_logCount < MAX_PACKET_LOG) _logCount++;

    taskEXIT_CRITICAL(&_mux);
}
