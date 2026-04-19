#ifndef STRAWBERRY_JAM_H
#define STRAWBERRY_JAM_H

// ╔══════════════════════════════════════════════════════════════════╗
// ║  STRAWBERRY JAM — WiFi Deauthentication Module                  ║
// ║                                                                  ║
// ║  FOR EDUCATIONAL / AUTHORIZED TESTING ONLY.                      ║
// ║  Sending deauth frames on networks you do not own is ILLEGAL     ║
// ║  in most jurisdictions. Use only on YOUR OWN network in a        ║
// ║  private lab environment.                                        ║
// ╚══════════════════════════════════════════════════════════════════╝
//
// How it works:
//   802.11 deauthentication frames are standard WiFi management frames.
//   An AP sends them to tell a client "you are disconnected" (reason code).
//   By spoofing the AP's MAC address (BSSID) as the source, we can send
//   a deauth frame that the client believes came from the real AP,
//   causing it to disconnect.
//
//   Frame structure (26 bytes):
//     [0-1]   Frame Control: 0xC0 0x00 (deauthentication)
//     [2-3]   Duration: 0x00 0x00
//     [4-9]   Destination MAC (the client to disconnect)
//     [10-15] Source MAC (spoofed as the AP's BSSID)
//     [16-21] BSSID (the AP's MAC address)
//     [22-23] Sequence number: 0x00 0x00
//     [24-25] Reason code: 0x07 0x00 (Class 3 frame from non-associated station)

#include <Arduino.h>
#include "esp_wifi.h"
#include "WiFiSniffer.h"

// How many deauth frames to send per burst (more = more effective but noisier)
#define DEAUTH_BURST_COUNT   5

// Delay between bursts in milliseconds
#define DEAUTH_BURST_DELAY  100

// Broadcast MAC — targets ALL clients on an AP
static const uint8_t BROADCAST_MAC[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

class StrawberryJam {
public:
    StrawberryJam();

    // Send a deauth burst targeting a specific client on a specific AP.
    // bssid  = the AP's MAC address (we spoof this as the sender)
    // client = the client MAC to disconnect (or BROADCAST_MAC for all clients)
    // channel = WiFi channel the AP operates on
    // Returns the number of frames successfully sent.
    int sendDeauth(const uint8_t bssid[6], const uint8_t client[6], uint8_t channel);

    // Start/stop continuous deauth attack on a target
    void startAttack(const uint8_t bssid[6], const uint8_t client[6], uint8_t channel);
    void stopAttack();
    bool isRunning();

    // Call from loop() — sends periodic bursts while attack is active
    void update();

    // Stats
    unsigned long getFramesSent();

private:
    bool _running;
    uint8_t _bssid[6];
    uint8_t _client[6];
    uint8_t _channel;
    unsigned long _framesSent;
    unsigned long _lastBurst;
};

#endif
