#include <Arduino.h>
#include "MenuDisplay.h"
#include "WiFiSniffer.h"
#include "GPS.h"
#include "WebServer.h"

#define COOLDOWN_MS 500

// --- TFT Menu and Sensors ---
MenuDisplay menuDisplay;
WiFiSniffer sniffer(1);              // Fixed channel for packet sniffer
GPSReader gpsReader(16, 17, 9600);  // RX=16, TX=17 (example pins)

// --- Web Server ---
WebMenuServer webServer("riniPhone", "MadaraUchiha", menuDisplay, sniffer, gpsReader);

// --- Page Management ---
PageState currentPage = PAGE_MENU;
bool pageNeedsRedraw = true;
unsigned long lastPressTime = 0;

void setup() {
    Serial.begin(115200);
    delay(200);

    // Initialize TFT and sensors
    menuDisplay.begin();
    sniffer.begin();
    gpsReader.begin();

    // Start web server
    webServer.begin();
}

void loop() {
    // --- Handle touch input ---
    PageState newSelection = PAGE_MENU;
    if (millis() - lastPressTime >= COOLDOWN_MS) {
        newSelection = menuDisplay.checkTouch();
    }

    if (currentPage == PAGE_MENU && newSelection != PAGE_MENU) {
        currentPage = newSelection;
        pageNeedsRedraw = true;
        lastPressTime = millis();
    } else if (currentPage != PAGE_MENU && newSelection == PAGE_MENU) {
        currentPage = PAGE_MENU;
        pageNeedsRedraw = true;
        lastPressTime = millis();
    }

    // --- Draw TFT pages ---
    if (currentPage == PAGE_MENU && pageNeedsRedraw) {
        menuDisplay.drawMenu();
        pageNeedsRedraw = false;
    } 
    else if (currentPage == PAGE_SNIFFER) {
        if (pageNeedsRedraw) {
            menuDisplay.showPacketSnifferPage();
            pageNeedsRedraw = false;
        }
        // Optionally update sniffer stats on TFT here
    } 
    else if (currentPage == PAGE_JAM && pageNeedsRedraw) {
        menuDisplay.showStrawberryJamPage();
        pageNeedsRedraw = false;
    } 
    else if (currentPage == PAGE_SENSOR && pageNeedsRedraw) {
        menuDisplay.showWirelessSensorPage();
        pageNeedsRedraw = false;
        // Optionally update heatmap if device info is available
    } 
    else if (currentPage == PAGE_GPS && pageNeedsRedraw) {
        menuDisplay.showGPSPage();
        pageNeedsRedraw = false;
        // Optionally update GPS info on TFT
    }

    // --- Update sensors ---
    sniffer.resetCounters();    // optional: reset counters periodically
    gpsReader.update();

    // --- Handle web server ---
    webServer.handleClient();   // process HTTP requests
}




