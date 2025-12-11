#include <WiFi.h>
#include "esp_wifi.h"
#include "WiFiClient.h"
#include "esp_event.h"
#include "nvs_flash.h"
 
 const char* ssid = "riniPhone";
 const char* password = "MadaraUchiha";
void setup() {

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  delay(100);
  Serial.println("Starting Wi-Fi jammer...");
 
  // The channel you want to disrupt
  int channel = 6; // Change to any channel from 1-13 as needed
  WiFi.disconnect();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
 wifi_country_t country = {
  .cc = "IE",
  .schan = 1,
  .nchan = 13,
  .policy = WIFI_COUNTRY_POLICY_MANUAL };
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
 
  Serial.println("Jamming started on channel " + String(channel));
}
 
void loop() {
  // This loop sends de-authentication packets in the background
}