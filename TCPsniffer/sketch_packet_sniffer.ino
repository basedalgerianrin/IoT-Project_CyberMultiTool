#include <WiFi.h>
#include <WebServer.h>
#include "esp_wifi.h"

   #define MAX_PACKET_LOG 50
#define HTML_REFRESH_MS 1000

WebServer server(80);

String packetLog[MAX_PACKET_LOG];
int packetIndex = 0;
volatile bool sniffPaused = false;   // <<< NEW

// ---- Store packet hex dump ----
void savePacket(const String &s) {
  packetLog[packetIndex] = s;
  packetIndex = (packetIndex + 1) % MAX_PACKET_LOG;
}

// ---- Dump packet data into hex string ----
String hexDump(const uint8_t* data, uint16_t len) {
  String out;
  for (int i = 0; i < len; i++) {
    char buff[4];
    sprintf(buff, "%02X ", data[i]);
    out += buff;
    if ((i + 1) % 16 == 0)
      out += "\n";
  }
  return out;
}

// ---- Sniffer callback ----
void snifferPacketHandler(void *buf, wifi_promiscuous_pkt_type_t type) {
  if (sniffPaused) return;  // <<< NEW: ignore packets if paused

  const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;

  uint16_t len = pkt->rx_ctrl.sig_len;
  const uint8_t *data = pkt->payload;

  String entry = "";
  entry += "Packet (" + String(len) + " bytes), type: " + String(type) + "\n";
  entry += hexDump(data, len);
  entry += "\n-----------------------------\n";

  savePacket(entry);
}
