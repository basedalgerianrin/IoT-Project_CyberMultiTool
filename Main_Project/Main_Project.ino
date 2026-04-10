#include <Arduino.h>
#include "PinDiagnostic.h"
#include "MenuDisplay.h"
#include "WiFiSniffer.h"
#include "StrawberryJam.h"
#include "BlueJam.h"
#include "GPS.h"
#include "WebMenuServer.h"

#define COOLDOWN_MS 500
#define SNIFFER_REFRESH_MS 1000 // How often to refresh the sniffer display
#define SENSOR_REFRESH_MS  1000 // How often to refresh the sensor hot/cold display
#define JAM_REFRESH_MS     500  // How often to refresh the jam display
#define GPS_REFRESH_MS     2000 // How often to refresh the GPS display

// --- TFT Menu and Sensors ---
MenuDisplay menuDisplay;
WiFiSniffer sniffer(1);             // Fixed channel for packet sniffer
StrawberryJam jam;                  // WiFi deauth module
BlueJam blueJam;                   // Bluetooth jammer (needs nRF24L01 hardware)
GPSReader gpsReader(16, 17, 9600);  // RX=16, TX=17

// --- Web Server (connects WiFi first, sniffer piggybacks on top) ---
WebMenuServer webServer("riniPhone", "MadaraUchiha", menuDisplay, sniffer, gpsReader);

// --- Page Management ---
PageState currentPage = PAGE_MENU;
bool pageNeedsRedraw = true;
unsigned long lastPressTime = 0;
unsigned long lastSnifferRefresh = 0;
unsigned long lastSensorRefresh = 0;
unsigned long lastJamRefresh = 0;
unsigned long lastGPSRefresh = 0;

void setup() {
    Serial.begin(115200);
    delay(200);

    // Run hardware diagnostics FIRST — before any module grabs the pins.
    PinDiagnostic::runAll();

    // Web server MUST init first — it connects WiFi.
    webServer.begin();
    webServer.setJam(&jam);
    menuDisplay.begin();
    sniffer.begin();
    gpsReader.begin();

    // Try to init Bluetooth jammer hardware (OK if nRF24 not connected)
    if (blueJam.begin()) {
        Serial.println(F("Bluetooth jammer ready (nRF24L01 detected)."));
    } else {
        Serial.println(F("No nRF24L01 — Bluetooth jam disabled, WiFi deauth still works."));
    }
}

void loop() {
    // --- Handle touch input ---
    // PAGE_NONE = no touch, PAGE_MENU = back button, others = menu selection
    PageState newSelection = PAGE_NONE;
    if (millis() - lastPressTime >= COOLDOWN_MS) {
        newSelection = menuDisplay.checkTouch();
    }

    if (newSelection != PAGE_NONE) {
        if (currentPage == PAGE_MENU && newSelection > PAGE_MENU) {
            // Navigate from menu to a subpage
            currentPage = newSelection;
            pageNeedsRedraw = true;
            lastPressTime = millis();
        } else if (currentPage != PAGE_MENU && newSelection == PAGE_MENU) {
            // Back button pressed — return to menu
            if (currentPage == PAGE_SENSOR && sniffer.isHopping()) {
                sniffer.lockChannel(WiFi.channel());
            }
            if (currentPage == PAGE_JAM) {
                if (jam.isRunning()) jam.stopAttack();
                if (blueJam.isRunning()) blueJam.stop();
            }
            currentPage = PAGE_MENU;
            pageNeedsRedraw = true;
            lastPressTime = millis();
        }
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
            lastSnifferRefresh = 0;  // Force immediate first draw
        }
        if (millis() - lastSnifferRefresh >= SNIFFER_REFRESH_MS) {
            menuDisplay.drawSnifferData(sniffer);
            lastSnifferRefresh = millis();
        }
    }
    else if (currentPage == PAGE_JAM) {
        if (pageNeedsRedraw) {
            menuDisplay.showStrawberryJamPage();
            pageNeedsRedraw = false;
            lastJamRefresh = 0;  // Force immediate first draw

            // Auto-start WiFi deauth on strongest nearby device
            if (sniffer.getDeviceCount() > 0 && !jam.isRunning()) {
                int bestIdx = 0;
                int8_t bestRssi = -127;
                for (int i = 0; i < sniffer.getDeviceCount(); i++) {
                    DeviceStats d = sniffer.getDevice(i);
                    if (d.rssi > bestRssi) {
                        bestRssi = d.rssi;
                        bestIdx = i;
                    }
                }
                DeviceStats target = sniffer.getDevice(bestIdx);
                jam.startAttack(target.mac, BROADCAST_MAC, target.channel);
            }

            // Also start Bluetooth jam if hardware is present
            if (blueJam.isHardwarePresent() && !blueJam.isRunning()) {
                blueJam.start(JAM_BLUETOOTH);
            }
        }

        // Keep sending attacks
        jam.update();
        blueJam.update();

        // Refresh display
        if (millis() - lastJamRefresh >= JAM_REFRESH_MS) {
            menuDisplay.drawJamData(sniffer, jam);
            lastJamRefresh = millis();
        }
    }
    else if (currentPage == PAGE_SENSOR) {
        if (pageNeedsRedraw) {
            menuDisplay.showWirelessSensorPage();
            pageNeedsRedraw = false;
            lastSensorRefresh = 0;  // Force immediate first draw
            sniffer.startHopping();
        }
        if (millis() - lastSensorRefresh >= SENSOR_REFRESH_MS) {
            menuDisplay.drawSensorData(sniffer);
            lastSensorRefresh = millis();

            int susIdx = sniffer.getStrongestSuspicious();
            if (susIdx >= 0 && sniffer.isHopping()) {
                DeviceStats sus = sniffer.getDevice(susIdx);
                sniffer.lockChannel(sus.channel);
            }
        }
        sniffer.channelHop();
    }
    else if (currentPage == PAGE_GPS) {
        if (pageNeedsRedraw) {
            menuDisplay.showGPSPage();
            pageNeedsRedraw = false;
            lastGPSRefresh = 0;  // Force immediate first draw
        }
        if (millis() - lastGPSRefresh >= GPS_REFRESH_MS) {
            menuDisplay.drawGPSData(gpsReader);
            lastGPSRefresh = millis();
        }
    }

    // --- Update sensors ---
    gpsReader.update();

    // --- Handle web server ---
    webServer.handleClient();
}
