#include <ThingSpeak.h>

#include <WiFi.h>
#include "esp_wifi.h"

// Callback function to handle received packets
void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type) {
    const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
    const wifi_ieee80211_packet_t * ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
    const wifi_ieee80211_mac_hdr_t * hdr = &ipkt->hdr;

    // Print RSSI and MAC addresses
    Serial.printf("RSSI: %d dBm | SRC: %02X:%02X:%02X:%02X:%02X:%02X | DST: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  ppkt->rx_ctrl.rssi,
                  hdr->addr2[0], hdr->addr2[1], hdr->addr2[2],
                  hdr->addr2[3], hdr->addr2[4], hdr->addr2[5],
                  hdr->addr1[0], hdr->addr1[1], hdr->addr1[2],
                  hdr->addr1[3], hdr->addr1[4], hdr->addr1[5]);
}

// Wi-Fi packet structures
typedef struct {
    uint8_t addr1[6]; // Destination MAC
    uint8_t addr2[6]; // Source MAC
    uint8_t addr3[6];
    uint16_t seq_ctrl;
} wifi_ieee80211_mac_hdr_t;

typedef struct {
    wifi_ieee80211_mac_hdr_t hdr;
    uint8_t payload[0];
} wifi_ieee80211_packet_t;

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Set Wi-Fi to station mode and disconnect from any network
    WiFi.mode(WIFI_MODE_NULL);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_filter(NULL); // Capture all packet types
    esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);

    Serial.println("ESP32 is now in promiscuous mode...");
}

void loop() {
    // Nothing here — packets are handled in the callback
}


