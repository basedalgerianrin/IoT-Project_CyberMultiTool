#ifndef WEB_MENU_SERVER_H
#define WEB_MENU_SERVER_H

#include <WiFi.h>
#include <NetworkClient.h>
#include <FS.h>        // Must come before WebServer.h in ESP32 core 3.x
#include <WebServer.h>
#include <ESPmDNS.h>

#include "MenuDisplay.h"
#include "WiFiSniffer.h"
#include "StrawberryJam.h"
#include "GPS.h"

class WebMenuServer {
public:
    WebMenuServer(const char* ssid, const char* password, MenuDisplay& display,
                  WiFiSniffer& sniffer, GPSReader& gps);

    void setJam(StrawberryJam* jam);  // Set reference to jam module
    void begin();
    void handleClient();

private:
    const char* _ssid;
    const char* _password;
    MenuDisplay& _display;
    WiFiSniffer& _sniffer;
    GPSReader& _gps;
    StrawberryJam* _jam;

    WebServer _server;

    void handleRoot();
    void handleSniffer();
    void handleSensor();
    void handleGPS();
    void handleJam();
    void handleNotFound();
};

#endif
