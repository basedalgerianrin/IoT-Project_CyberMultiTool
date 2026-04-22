#ifndef WIFI_SNIFFER_H
#define WIFI_SNIFFER_H

#include <WiFi.h>
#include "esp_wifi.h"

// --- Packet types (used to label each captured packet) ---
enum PktType {
    PKT_DATA    = 0,
    PKT_BEACON  = 1,
    PKT_DEAUTH  = 2,
    PKT_PROBE_REQ  = 3,
    PKT_PROBE_RESP = 4,
    PKT_OTHER   = 5
};

// --- One entry in the packet log (like one row in Wireshark) ---
struct PacketEntry {
    unsigned long timestamp;    // millis() when captured
    uint8_t srcMac[6];         // source MAC address
    int8_t  rssi;              // signal strength in dBm
    PktType type;              // what kind of 802.11 frame
};

// --- How many packets to keep in the scrolling log ---
#define MAX_PACKET_LOG 30

// --- How many unique MACs to track ---
#define MAX_UNIQUE_MACS 50

// --- Per-device statistics (used by the Wireless Sensor) ---
// Each detected device gets one of these to track its behaviour.
// Hidden WiFi cameras stream video constantly, producing a flood
// of data frames with a bimodal size distribution (large I-frames
// near MTU + small P-frames). We track these patterns to detect them.
struct DeviceStats {
    uint8_t  mac[6];          // device MAC address
    int8_t   rssi;            // smoothed RSSI (exponential moving average)
    uint16_t totalPkts;       // total packets seen from this device
    uint16_t dataPkts;        // how many were data frames (0x08 / 0x88)
    uint16_t largePkts;       // frames > 1000 bytes (I-frame indicator)
    uint16_t smallPkts;       // frames 100-600 bytes (P-frame indicator)
    unsigned long firstSeen;  // millis() when first detected
    unsigned long lastSeen;   // millis() when last detected
    uint8_t  channel;         // which WiFi channel this device was seen on
    bool     knownCamOUI;     // true if MAC matches a known camera manufacturer
};

// --- Thresholds for "suspicious" device detection ---
#define SUSPECT_MIN_PACKETS   10    // need at least this many to judge
#define SUSPECT_DATA_RATIO    40    // percent data frames to flag
#define SUSPECT_BIMODAL_MIN   3     // need at least this many large+small frames

// --- RSSI smoothing factor (0.0-1.0, lower = more smoothing) ---
// Research says raw RSSI swings +/-15 dBm. EMA with alpha=0.3 gives
// a smooth reading that still responds within ~1 second.
#define RSSI_SMOOTH_ALPHA     0.3f

// --- Channel hopping for discovery ---
// Most WiFi devices operate on channels 1, 6, or 11 (non-overlapping).
// We hop across these three to find devices, then lock onto the suspect's channel.
#define NUM_SCAN_CHANNELS     3
#define CHANNEL_DWELL_MS      800   // how long to stay on each channel during scan

class WiFiSniffer {
public:
    WiFiSniffer(uint8_t channel = 1);

    void begin();
    void resetCounters();

    // --- Summary counters ---
    unsigned long getTotal();
    unsigned long getBeacons();
    unsigned long getDeauths();
    unsigned long getProbeReqs();
    unsigned long getProbeResps();
    unsigned long getDataPkts();
    unsigned int  getUniqueMacs();

    // --- Packet log (circular buffer) ---
    int           getLogCount();          // how many entries stored
    PacketEntry   getLogEntry(int index); // get one entry by index (0 = oldest)

    // --- Per-device tracking (Wireless Sensor) ---
    DeviceStats   getDevice(int index);       // get device by index
    int           getDeviceCount();           // how many devices tracked
    bool          isSuspicious(int index);    // is device at index suspicious?
    int           getSuspiciousCount();       // how many devices are flagged
    int           getStrongestSuspicious();   // index of suspicious device with best RSSI (-1 if none)

    // --- Channel hopping (call from loop() for discovery mode) ---
    void          channelHop();               // advance to next scan channel
    bool          isHopping();                // true if in scan mode
    void          lockChannel(uint8_t ch);    // stop hopping, stay on this channel
    void          startHopping();             // resume channel hopping
    uint8_t       getCurrentChannel();        // which channel we're on now

    // --- Utility ---
    static const char* typeToString(PktType t);
    static String macToString(const uint8_t mac[6]);
    static bool   isKnownCameraOUI(const uint8_t mac[6]);
    static const char* getOUIVendor(const uint8_t mac[6]);

private:
    static void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);

    uint8_t _channel;

    // --- Counters (static because callback is static) ---
    static unsigned long _total;
    static unsigned long _beacons;
    static unsigned long _deauths;
    static unsigned long _probeReqs;
    static unsigned long _probeResps;
    static unsigned long _dataPkts;

    // --- Packet log (circular buffer) ---
    static PacketEntry _log[MAX_PACKET_LOG];
    static int _logIndex;  // where to write next
    static int _logCount;  // how many stored (max MAX_PACKET_LOG)

    // --- Per-device tracking ---
    static DeviceStats _devices[MAX_UNIQUE_MACS];
    static uint8_t _deviceCount;
    static void trackDevice(const uint8_t mac[6], int8_t rssi, PktType pktType, uint16_t frameLen);
    static bool isDeviceSuspicious(const DeviceStats& d);

    // --- Channel hopping state ---
    static bool _hopping;              // true = scanning, false = locked
    static uint8_t _currentChannel;    // current channel we're on
    static uint8_t _hopIndex;          // which scan channel we're on (0-2)
    static unsigned long _lastHop;     // millis() of last channel switch
    static const uint8_t _scanChannels[NUM_SCAN_CHANNELS]; // {1, 6, 11}

    // --- Thread safety (callback runs on WiFi task, getters on main task) ---
    static portMUX_TYPE _mux;
};

#endif
