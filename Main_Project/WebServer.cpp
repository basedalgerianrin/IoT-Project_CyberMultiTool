#include "WebServer.h"

WebMenuServer::WebMenuServer(const char* ssid, const char* password, MenuDisplay& display, WiFiSniffer& sniffer, GPSReader& gps)
  : _ssid(ssid), _password(password), _display(display), _sniffer(sniffer), _gps(gps), _webServer(80) {}

void WebMenuServer::begin() {
    WiFi.begin(_ssid, _password);
    while (WiFi.status() != WL_CONNECTED) delay(500);

    _webServer.on("/", std::bind(&WebMenuServer::handleRoot, this));
    _webServer.on("/sniffer", std::bind(&WebMenuServer::handleSniffer, this));
    _webServer.on("/jam", std::bind(&WebMenuServer::handleJam, this));
    _webServer.on("/sensor", std::bind(&WebMenuServer::handleSensor, this));
    _webServer.on("/gps", std::bind(&WebMenuServer::handleGPS, this));
    _webServer.onNotFound(std::bind(&WebMenuServer::handleNotFound, this));

    _webServer.begin();
}

void WebMenuServer::handleClient() {
    _webServer.handleClient();
}

void WebMenuServer::handleRoot() {
    _webServer.send(200, "text/html", "<h1>ESP32 Menu</h1>");
}

void WebMenuServer::handleSniffer() {
    _webServer.send(200, "text/plain", "Packet Sniffer activated!");
}

void WebMenuServer::handleJam() {
    _webServer.send(200, "text/plain", "Strawberry Jam activated!");
}

void WebMenuServer::handleSensor() {
    _webServer.send(200, "text/plain", "Wireless Sensor activated!");
}

void WebMenuServer::handleGPS() {
    _webServer.send(200, "text/plain", "GPS data active!");
}

void WebMenuServer::handleNotFound() {
    _webServer.send(404, "text/plain", "404: Not Found");
}
