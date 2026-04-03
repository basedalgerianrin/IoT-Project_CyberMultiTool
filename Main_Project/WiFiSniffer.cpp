#include "WiFiSniffer.h"

// Static member definitions
unsigned long WiFiSniffer::_pkts = 0;
unsigned long WiFiSniffer::_deauths = 0;
bool WiFiSniffer::_ssid_locked = false;
char WiFiSniffer::_ssid[33] = "unknown";

WiFiSniffer::WiFiSniffer(uint8_t channel)
    : _channel(channel) {
}

void WiFiSniffer::begin() {
    Serial.println("=== ESP32 WiFi Sniffer (Class Based) ===");

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_NULL);
    esp_wifi_start();

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&WiFiSniffer::snifferCallback);
    esp_wifi_set_channel(_channel, WIFI_SECOND_CHAN_NONE);

    Serial.printf("Sniffing on channel %d...\n", _channel);
}

void WiFiSniffer::resetCounters() {
    _deauths = 0;
    _pkts = 0;
}

unsigned long WiFiSniffer::getPackets() {
    return _pkts;
}

unsigned long WiFiSniffer::getDeauths() {
    return _deauths;
}

const char* WiFiSniffer::getSSID() {
    return _ssid;
}

void WiFiSniffer::snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    const wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    const uint8_t* payload = pkt->payload;

    _pkts++; // Count all packets

    uint8_t fc = payload[0];
    bool isDeauth = (fc == 0xC0 || fc == 0xA0);
    bool isBeacon = (fc == 0x80);

    if (isDeauth) _deauths++;

    if (!_ssid_locked && isBeacon) {
        uint8_t len = payload[37];
        if (len > 0 && len < 33) {
            memcpy(_ssid, &payload[38], len);
            _ssid[len] = 0;
            _ssid_locked = true;
        }
    }
}