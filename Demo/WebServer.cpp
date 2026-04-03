#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "GPS.h"

/******************** USER SETTINGS ********************/
const char* ssid = "riniPhone";
const char* password = "MadaraUchiha";

GPSReader gps(16, 17, 9600);
WebServer server(80);

/******************** BUILD WEB PAGE ********************/
String buildWebPage(float lat, float lng, int sats) {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>GPS Tracker</title>";
  html += "<style>body{font-family:Arial;text-align:center;background:#f4f4f4;color:#333;} ";
  html += "h1{background:#2c3e50;color:#fff;padding:15px;} .box{background:#fff; padding:20px; margin:20px auto; width:80%; border-radius:10px; box-shadow:0 4px 6px rgba(0,0,0,0.1);}</style></head><body>";
  
  html += "<h1>Live GPS Dashboard</h1>";
  html += "<div class='box'>";
  html += "<p><b>Satellites:</b> " + String(sats) + "</p>";
  html += "<p style='font-size:24px;'><b>Lat:</b> " + String(lat, 6) + "</p>";
  html += "<p style='font-size:24px;'><b>Lng:</b> " + String(lng, 6) + "</p>";
  
  // Dynamic Google Maps Link
  if (lat != 0.0) {
    html += "<br><a href='https://www.google.com/maps?q=" + String(lat, 6) + "," + String(lng, 6) + "' target='_blank'>";
    html += "<button style='padding:15px; background:#3498db; color:white; border:none; border-radius:5px; cursor:pointer;'>Open in Google Maps</button></a>";
  } else {
    html += "<p style='color:red;'>Waiting for Satellite Fix...</p>";
  }
  
  html += "</div><p>Auto-refreshing every 3 seconds</p>";
  html += "<meta http-equiv='refresh' content='3'>";
  html += "</body></html>";
  
  return html;
}

/******************** WEB HANDLERS ********************/
void handleRoot() {
  server.send(200, "text/html", buildWebPage(gps.getLat(), gps.getLng(), gps.getSats()));
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Location Not Found");
}

/******************** SETUP ********************/
void setup() {
  Serial.begin(115200);
  gps.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  if (MDNS.begin("gps-tracker")) {
    Serial.println("mDNS: http://gps-tracker.local");
  }

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  
  Serial.println("\nGPS Server Online!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

/******************** LOOP ********************/
void loop() {
  gps.update();       // Constantly feed data from the GPS module
  server.handleClient(); // Handle web browser requests
}
