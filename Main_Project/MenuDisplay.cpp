#include "MenuDisplay.h"

MenuDisplay::MenuDisplay() : tft(TFT_eSPI()) {}

void MenuDisplay::begin() {
    tft.init();
    tft.setRotation(0);

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);

    // Initialize FT6206 Capacitive Touch
    if (!touch.begin(40)) {          // Threshold = 40
        tft.setCursor(10, 10);
        tft.println("FT6206 NOT FOUND!");
        while (1);
    }
}

void MenuDisplay::drawMenu() {
    tft.fillScreen(TFT_BLACK);

    tft.drawString("MAIN MENU", 60, 10, 4);

    // Option 1
    tft.drawRect(20, 60, 200, 40, TFT_WHITE);
    tft.drawString("Packet Sniffer", 40, 70, 2);

    // Option 2
    tft.drawRect(20, 120, 200, 40, TFT_WHITE);
    tft.drawString("Strawberry Jam", 40, 130, 2);

    // Option 3
    tft.drawRect(20, 180, 200, 40, TFT_WHITE);
    tft.drawString("Wireless Sensor", 40, 190, 2);

    // Option 4
    tft.drawRect(20, 240, 200, 40, TFT_WHITE);
    tft.drawString("GPS", 40, 250, 2);
}


/*
 * Touch detection using Adafruit_FT6206
 *
 * Returns:
 *   1 = Packet Sniffer
 *   2 = Strawberry Jam
 *   3 = Wireless Sensor
 *   4 = GPS
 *   0 = nothing touched
 */
int MenuDisplay::checkTouch() {
    if (!touch.touched()) return 0;

    TS_Point p = touch.getPoint();

    // FT6206 origin is top-left, but coordinates may need rotation
    // If the screen is rotated, adjust as needed:

    int x = p.y;  // Swap
    int y = 320 - p.x;

    // OPTION 1
    if (x > 20 && x < 220 && y > 60 && y < 100) return 1;

    // OPTION 2
    if (x > 20 && x < 220 && y > 120 && y < 160) return 2;

    // OPTION 3
    if (x > 20 && x < 220 && y > 180 && y < 220) return 3;

    // OPTION 4
    if (x > 20 && x < 220 && y > 240 && y < 280) return 4;

    return 0; // No valid selection
}


/*************************************
 *          SUB-PAGE SCREENS
 *************************************/

void MenuDisplay::showPacketSnifferPage() {
    tft.fillScreen(TFT_BLACK);
    tft.drawString("PACKET SNIFFER", 40, 20, 4);
}

void MenuDisplay::showStrawberryJamPage() {
    tft.fillScreen(TFT_BLACK);
    tft.drawString("STRAWBERRY JAM", 30, 20, 4);
}

void MenuDisplay::showWirelessSensorPage() {
    tft.fillScreen(TFT_BLACK);
    tft.drawString("WIRELESS SENSOR", 20, 20, 4);
}

void MenuDisplay::showGPSPage() {
    tft.fillScreen(TFT_BLACK);
    tft.drawString("GPS PAGE", 80, 20, 4);
}

