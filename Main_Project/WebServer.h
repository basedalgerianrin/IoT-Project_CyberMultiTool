#ifndef WEB_MENU_SERVER_H
#define WEB_MENU_SERVER_H

#include <WiFi.h>
#include <NetworkClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include "MenuDisplay.h"
#include "WiFiSniffer.h"
#include "GPS.h"

class WebMenuServer {
public:
    WebMenuServer(const char* ssid, const char* password, MenuDisplay& display, WiFiSniffer& sniffer, GPSReader& gps);

    void begin();
    void handleClient();

private:
    const char* _ssid;
    const char* _password;
    MenuDisplay& _display;
    WiFiSniffer& _sniffer;
    GPSReader& _gps;

    WebServer _webServer; // <- ensure correct WebServer type

    void handleRoot();
    void handleSniffer();
    void handleJam();
    void handleSensor();
    void handleGPS();
    void handleNotFound();
};

#endif
