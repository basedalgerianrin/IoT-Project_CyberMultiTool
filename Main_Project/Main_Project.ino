#include <Arduino.h>
#include "MenuDisplay.h"
#include "WiFiSniffer.h"
#include "GPS.h"

#define COOLDOWN_MS 500

// --- TFT Menu and Sensors ---
MenuDisplay menuDisplay;
WiFiSniffer sniffer(1);              // Fixed channel for packet sniffer
GPSReader gpsReader(16, 17, 9600);  // RX=16, TX=17 (example pins)

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
    }
    // else if (currentPage == PAGE_JAM && pageNeedsRedraw) {  // Commented out pending legal research
    //     menuDisplay.showStrawberryJamPage();
    //     pageNeedsRedraw = false;
    // }
    else if (currentPage == PAGE_SENSOR && pageNeedsRedraw) {
        menuDisplay.showWirelessSensorPage();
        pageNeedsRedraw = false;
    }
    else if (currentPage == PAGE_GPS && pageNeedsRedraw) {
        menuDisplay.showGPSPage();
        pageNeedsRedraw = false;
    }

    // --- Update sensors ---
    sniffer.resetCounters();
    gpsReader.update();
}
