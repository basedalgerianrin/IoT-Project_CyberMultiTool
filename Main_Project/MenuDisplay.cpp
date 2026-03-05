#include "MenuDisplay.h"

// --- Back button constants ---
#define BACK_BTN_W 60
#define BACK_BTN_H 30
#define BACK_BTN_X (240 - BACK_BTN_W - 10)
#define BACK_BTN_Y (320 - BACK_BTN_H - 10)

MenuDisplay::MenuDisplay() : tft(TFT_eSPI()) {}

void MenuDisplay::begin() {
    tft.init();
    tft.setRotation(0); 
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);

    if (!touch.begin(40)) {
        tft.setCursor(10, 10);
        tft.println("FT6206 NOT FOUND!");
    } 
}

// --- Main Menu ---
void MenuDisplay::drawMenu() {
    tft.fillScreen(TFT_BLACK);
    tft.drawString("MAIN MENU", 60, 10, 4);

    tft.drawRect(20, 60, 200, 40, TFT_WHITE);
    tft.drawString("Packet Sniffer", 40, 70, 2);

    tft.drawRect(20, 120, 200, 40, TFT_WHITE);
    tft.drawString("Strawberry Jam", 40, 130, 2);

    tft.drawRect(20, 180, 200, 40, TFT_WHITE);
    tft.drawString("Wireless Sensor", 40, 190, 2);

    tft.drawRect(20, 240, 200, 40, TFT_WHITE);
    tft.drawString("GPS", 40, 250, 2);
}

// --- Back Button ---
void MenuDisplay::drawBackButton() {
    tft.fillRect(BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H, TFT_RED);
    tft.drawRect(BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("< BACK", BACK_BTN_X + 5, BACK_BTN_Y + 8, 1); 
}

bool MenuDisplay::checkBackButton(int tftX, int tftY) {
    return (tftX >= BACK_BTN_X && tftX <= (BACK_BTN_X + BACK_BTN_W) &&
            tftY >= BACK_BTN_Y && tftY <= (BACK_BTN_Y + BACK_BTN_H));
}

// --- Touch Handling ---
PageState MenuDisplay::checkTouch() {
    if (!touch.touched()) return PAGE_MENU;

    TS_Point p = touch.getPoint();

    int x = tft.width() - p.y;
    int y = p.x;

    if (checkBackButton(x, y)) return PAGE_MENU;

    const int X_MIN = 20;
    const int X_MAX = 220;

    if (x > X_MIN && x < X_MAX) {
        if (y > 60 && y < 100) return PAGE_SNIFFER;
        if (y > 120 && y < 160) return PAGE_JAM;
        if (y > 180 && y < 220) return PAGE_SENSOR;
        if (y > 240 && y < 280) return PAGE_GPS;
    }

    return PAGE_MENU;
}

// --- Pages ---
void MenuDisplay::showPacketSnifferPage() {
    tft.fillScreen(TFT_BLACK);
    tft.drawString("PACKET SNIFFER", 40, 20, 4);
    drawBackButton();
}

void MenuDisplay::showStrawberryJamPage() {
    tft.fillScreen(TFT_BLACK);
    tft.drawString("STRAWBERRY JAM", 30, 20, 4);
    drawBackButton();
}

void MenuDisplay::showWirelessSensorPage() {
    tft.fillScreen(TFT_BLACK);
    tft.drawString("WIRELESS SENSOR", 20, 20, 4);
    drawBackButton();
}

void MenuDisplay::showGPSPage() {
    tft.fillScreen(TFT_BLACK);
    tft.drawString("GPS PAGE", 80, 20, 4);
    drawBackButton();
}

// --- Sniffer display ---
void MenuDisplay::drawSnifferData(WiFiSniffer &sniffer) {
    tft.fillRect(0, 60, tft.width(), tft.height()-60, TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("Pkts: " + String(sniffer.getPackets()), 10, 70, 2);
    tft.drawString("Deauths: " + String(sniffer.getDeauths()), 10, 100, 2);
    tft.drawString("SSID: " + String(sniffer.getSSID()), 10, 130, 2);
}

// --- Heatmap update ---
int MenuDisplay::rssiToColor(int rssi) {
    int val = map(rssi, -90, -30, 0, 255);
    return constrain(val, 0, 255);
}

void MenuDisplay::updateHeatmap(int rssi) {
    uint16_t color = tft.color565(rssiToColor(rssi), 0, 255 - rssiToColor(rssi));
    tft.fillRect(60, 80, 120, 160, color);

    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("RSSI: " + String(rssi), tft.width()/2, 50, 4);
}




