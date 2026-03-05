#ifndef WEB_MENU_SERVER_H
#define WEB_MENU_SERVER_H

#include <WiFi.h>
#include <NetworkClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "GPS.h"

class WebMenuServer {
public:
    WebMenuServer(const char* ssid, const char* password, GPSReader& gps);
    void begin();
    void handleClient();

private:
    const char* _ssid;
    const char* _password;
    GPSReader& _gps;
    WebServer _webServer;

    void handleRoot();
    void handleGPS();
};

#endif