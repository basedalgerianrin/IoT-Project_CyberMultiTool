#ifndef WIFI_SNIFFER_H
#define WIFI_SNIFFER_H

#include <WiFi.h>
#include "esp_wifi.h"

class WiFiSniffer {
public:
    WiFiSniffer(uint8_t channel = 1);

    void begin();                // Initialize WiFi in promiscuous mode
    void resetCounters();        // Reset counters manually
    unsigned long getPackets();  // Get total packets
    unsigned long getDeauths();  // Get deauth count
    const char* getSSID();       // Get discovered SSID

private:
    static void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);

    uint8_t _channel;
    static unsigned long _pkts;
    static unsigned long _deauths;
    static bool _ssid_locked;
    static char _ssid[33];
};

#endif