#include <WiFi.h>
#include "esp_wifi.h"

// Fixed WiFi channel to monitor (1–13)
#define FIXED_CHANNEL 1

// Packet counters
unsigned long pkts = 0;
unsigned long deauths = 0;

// SSID tracking
bool ssid_locked = false;
char ssid[33] = "unknown";

/*
 * PROMISCUOUS MODE CALLBACK
 * Triggered on every WiFi packet received.
 */
void sniffer(void *buf, wifi_promiscuous_pkt_type_t type) {
  const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
  const uint8_t *payload = pkt->payload;

  pkts++; // count each packet

  uint8_t fc = payload[0]; // frame control byte

  bool isDeauth = (fc == 0xC0 || fc == 0xA0);
  bool isBeacon = (fc == 0x80);

  if (isDeauth) {
    deauths++;
  }

  // Extract SSID from beacon frames if available
  if (!ssid_locked && isBeacon) {
    uint8_t len = payload[37];
    if (len > 0 && len < 33) {
      memcpy(ssid, &payload[38], len);
      ssid[len] = 0;
      ssid_locked = true;
    }
  }

  // TCPDump-style printout
  Serial.printf(
      "[%lu ms] CH:%d RSSI:%d LEN:%d FC:0x%02X %s %s\n",
      millis(),
      FIXED_CHANNEL,
      pkt->rx_ctrl.rssi,
      pkt->rx_ctrl.sig_len,
      fc,
      isDeauth ? "DEAUTH" : "",
      isBeacon ? "BEACON" : ""
  );

  // OPTIONAL: Uncomment for raw hex dump of each packet
  /*
  for (int i = 0; i < pkt->rx_ctrl.sig_len; i++) {
      Serial.printf("%02X ", payload[i]);
  }
  Serial.println();
  */
}

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println("=== ESP32 WiFi Sniffer (No Buttons, No Channel Switching) ===");

  // Initialize WiFi in null mode
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();

  // Enable promiscuous sniffing
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&sniffer);

  // Set fixed channel
  esp_wifi_set_channel(FIXED_CHANNEL, WIFI_SECOND_CHAN_NONE);
  Serial.printf("Sniffing on channel %d...\n", FIXED_CHANNEL);
}

void loop() {
  // Reset deauth counter every second (used for activity tracking)
  static unsigned long last = 0;
  if (millis() - last >= 1000) {
    last = millis();
    deauths = 0;
  }

  // Nothing else to do; packets arrive in callback
}
