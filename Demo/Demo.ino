#include <Arduino.h>
#include "WebServer.h"
#include "GPS.h"

const char* ssid = "riniPhone";
const char* password = "MadaraUchiha";

GPSReader gps; // Uses default pins 16(RX) and 17(TX)
WebMenuServer webServer(ssid, password, gps);

void setup() {
    Serial.begin(115200);
    gps.begin();
    webServer.begin();
}

void loop() {
    gps.update();        // Keep reading serial data from GPS module
    webServer.handleClient(); // Process web requests
}



