#include "MenuDisplay.h"

// --- Back button constants ---
#define BACK_BTN_W 60
#define BACK_BTN_H 30
#define BACK_BTN_X (240 - BACK_BTN_W - 10)
#define BACK_BTN_Y (320 - BACK_BTN_H - 10)

MenuDisplay::MenuDisplay() : tft(TFT_eSPI()) {}

void MenuDisplay::begin() {
    Wire.begin(21, 22);  // SDA=21, SCL=22 for FT6206 touch controller

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

    tft.drawRect(20, 120, 200, 40, TFT_RED);
    tft.setTextColor(TFT_RED);
    tft.drawString("Strawberry Jam", 40, 130, 2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

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
    if (!touch.touched()) return PAGE_NONE;

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

    return PAGE_NONE;  // Touch missed all buttons — ignore it
}

// --- Pages ---
void MenuDisplay::showPacketSnifferPage() {
    tft.fillScreen(TFT_BLACK);
    tft.drawString("PACKET SNIFFER", 40, 20, 4);
    drawBackButton();
}

void MenuDisplay::showStrawberryJamPage() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.drawString("STRAWBERRY JAM", 30, 10, 4);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("WiFi Deauth Attack", 40, 40, 2);
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
    tft.drawString("Pkts: " + String(sniffer.getTotal()), 10, 70, 2);
    tft.drawString("Deauths: " + String(sniffer.getDeauths()), 10, 100, 2);
    tft.drawString("Beacons: " + String(sniffer.getBeacons()), 10, 130, 2);
}

// --- Strawberry Jam display ---
// Shows the attack status and target info on the TFT.
// Automatically targets the device with the strongest RSSI from the sniffer.
void MenuDisplay::drawJamData(WiFiSniffer &sniffer, StrawberryJam &jam) {
    tft.fillRect(0, 60, tft.width(), tft.height() - 90, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);

    if (jam.isRunning()) {
        // Attack is active — show red pulsing status
        tft.setTextColor(TFT_RED);
        tft.drawString("JAMMING", tft.width()/2, 80, 4);

        tft.setTextColor(TFT_WHITE);
        tft.drawString("Frames sent:", tft.width()/2, 120, 2);
        tft.setTextColor(TFT_YELLOW);
        tft.drawString(String(jam.getFramesSent()), tft.width()/2, 145, 4);

    } else {
        tft.setTextColor(TFT_DARKGREY);
        tft.drawString("IDLE", tft.width()/2, 80, 4);

        tft.setTextColor(TFT_WHITE);
        tft.drawString("Waiting for target...", tft.width()/2, 120, 2);
    }

    // Show detected devices count
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_CYAN);
    tft.drawString("Devices: " + String(sniffer.getDeviceCount()), 10, 180, 2);

    // Show the strongest device (auto-target)
    if (sniffer.getDeviceCount() > 0) {
        // Find the device with strongest RSSI (most likely nearby target)
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
        tft.setTextColor(TFT_YELLOW);
        tft.drawString("Target:", 10, 210, 2);
        tft.setTextColor(TFT_WHITE);
        tft.drawString(WiFiSniffer::macToString(target.mac), 10, 235, 2);
        tft.drawString("RSSI: " + String(target.rssi) + " dBm", 10, 255, 1);
    }
}

// --- Wireless Sensor: Hot/Cold camera detector display ---
void MenuDisplay::drawSensorData(WiFiSniffer &sniffer) {
    tft.fillRect(0, 50, tft.width(), tft.height() - 90, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);

    // Show current channel info at top
    tft.setTextColor(TFT_CYAN);
    tft.drawString("Ch:" + String(sniffer.getCurrentChannel()) +
        (sniffer.isHopping() ? " [SCAN]" : " [LOCK]"), tft.width()/2, 55, 1);

    int targetIdx = sniffer.getStrongestSuspicious();

    if (targetIdx >= 0) {
        DeviceStats target = sniffer.getDevice(targetIdx);
        int8_t rssi = target.rssi;

        int r, g, b;
        if (rssi > -45)      { r = 255; g = 0;   b = 0;   }
        else if (rssi > -60) { r = 255; g = 140; b = 0;   }
        else if (rssi > -75) { r = 0;   g = 100; b = 255; }
        else                 { r = 0;   g = 30;  b = 150; }

        uint16_t color = tft.color565(r, g, b);
        tft.fillCircle(tft.width()/2, 140, 55, color);

        tft.setTextColor(TFT_WHITE);
        const char* label;
        if      (rssi > -45) label = "HOT!";
        else if (rssi > -60) label = "WARM";
        else if (rssi > -75) label = "COOL";
        else                 label = "COLD";

        tft.drawString(label, tft.width()/2, 130, 4);
        tft.drawString(String(rssi) + " dBm", tft.width()/2, 155, 2);

        tft.setTextDatum(MC_DATUM);
        const char* vendor = WiFiSniffer::getOUIVendor(target.mac);
        if (vendor) {
            tft.setTextColor(TFT_ORANGE);
            tft.drawString(String(vendor), tft.width()/2, 210, 2);
        }
        tft.setTextColor(TFT_YELLOW);
        tft.drawString("SUSPECT:", tft.width()/2, 228, 1);
        tft.setTextColor(TFT_WHITE);
        tft.drawString(WiFiSniffer::macToString(target.mac), tft.width()/2, 243, 2);

        int dataPercent = (target.totalPkts > 0) ? (target.dataPkts * 100 / target.totalPkts) : 0;
        tft.setTextColor(TFT_LIGHTGREY);
        tft.drawString(String(dataPercent) + "% data | L:" +
            String(target.largePkts) + " S:" + String(target.smallPkts),
            tft.width()/2, 263, 1);
    } else {
        tft.setTextColor(TFT_CYAN);
        tft.drawString("Scanning...", tft.width()/2, 100, 4);

        tft.setTextColor(TFT_DARKGREY);
        tft.drawString("Devices: " + String(sniffer.getDeviceCount()), tft.width()/2, 140, 2);
        tft.drawString("Suspicious: " + String(sniffer.getSuspiciousCount()), tft.width()/2, 165, 2);

        tft.setTextColor(TFT_WHITE);
        tft.drawString("Walk around to", tft.width()/2, 200, 2);
        tft.drawString("detect transmitters", tft.width()/2, 220, 2);
    }
}

// --- GPS live data display ---
void MenuDisplay::drawGPSData(GPSReader &gps) {
    tft.fillRect(0, 50, tft.width(), tft.height() - 90, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE);

    tft.drawString("Lat:", 10, 60, 2);
    tft.drawString(String(gps.getLatitude(), 6), 80, 60, 2);

    tft.drawString("Lon:", 10, 85, 2);
    tft.drawString(String(gps.getLongitude(), 6), 80, 85, 2);

    tft.drawString("Alt:", 10, 110, 2);
    tft.drawString(String(gps.getAltitude(), 1) + " m", 80, 110, 2);

    tft.drawString("Spd:", 10, 135, 2);
    tft.drawString(String(gps.getSpeed(), 1) + " km/h", 80, 135, 2);

    tft.setTextColor(TFT_CYAN);
    tft.drawString("Time:", 10, 170, 2);
    tft.drawString(gps.getDateTime(), 80, 170, 2);
}

