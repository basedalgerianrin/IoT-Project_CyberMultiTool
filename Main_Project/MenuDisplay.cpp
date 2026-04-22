#include "MenuDisplay.h"

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

#define BACK_BTN_W 70
#define BACK_BTN_H 35
#define BACK_BTN_X (240 - BACK_BTN_W - 5)
#define BACK_BTN_Y (320 - BACK_BTN_H - 5)

MenuDisplay::MenuDisplay() : _lastTotal(0), _graphInited(false) {
    memset(_graphHistory, 0, sizeof(_graphHistory));
}

void MenuDisplay::begin() {
    Wire.begin(21, 22);
    tft.begin();
    tft.setRotation(0);
    tft.fillScreen(COL_BLACK);
    tft.setTextColor(COL_WHITE);
    tft.setTextSize(2);

    if (!touch.begin(40)) {
        tft.setCursor(10, 10);
        tft.println("FT6206 NOT FOUND!");
    }
}

// ── Text helpers ────────────────────────────────────────────────────────────
void MenuDisplay::drawText(const char* text, int16_t x, int16_t y, uint8_t size) {
    tft.setTextSize(size);
    tft.setCursor(x, y);
    tft.print(text);
}

void MenuDisplay::drawText(const String &text, int16_t x, int16_t y, uint8_t size) {
    drawText(text.c_str(), x, y, size);
}

void MenuDisplay::drawCentered(const char* text, int16_t cx, int16_t cy, uint8_t size) {
    tft.setTextSize(size);
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(cx - x1 - (int16_t)(w / 2), cy - y1 - (int16_t)(h / 2));
    tft.print(text);
}

void MenuDisplay::drawCentered(const String &text, int16_t cx, int16_t cy, uint8_t size) {
    drawCentered(text.c_str(), cx, cy, size);
}

// ── Main Menu ────────────────────────────────────────────────────────────────
void MenuDisplay::drawMenu() {
    tft.fillScreen(COL_BLACK);
    tft.setTextColor(COL_WHITE);
    drawText("MAIN MENU", 60, 10, 3);

    tft.drawRect(20, 60, 200, 40, COL_WHITE);
    drawText("Packet Sniffer", 40, 70, 2);

    tft.drawRect(20, 120, 200, 40, COL_RED);
    tft.setTextColor(COL_RED);
    drawText("Strawberry Jam", 40, 130, 2);
    tft.setTextColor(COL_WHITE);

    tft.drawRect(20, 180, 200, 40, COL_WHITE);
    drawText("Wireless Sensor", 40, 190, 2);

    tft.drawRect(20, 240, 200, 40, COL_WHITE);
    drawText("GPS", 40, 250, 2);
}

// ── Back Button ──────────────────────────────────────────────────────────────
void MenuDisplay::drawBackButton() {
    tft.fillRect(BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H, COL_RED);
    tft.drawRect(BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H, COL_WHITE);
    tft.setTextColor(COL_WHITE);
    drawText("< BACK", BACK_BTN_X + 5, BACK_BTN_Y + 8, 1);
}

bool MenuDisplay::checkBackButton(int tftX, int tftY) {
    return (tftX >= BACK_BTN_X && tftX <= (BACK_BTN_X + BACK_BTN_W) &&
            tftY >= BACK_BTN_Y && tftY <= (BACK_BTN_Y + BACK_BTN_H));
}

// ── Touch Handling ──────────────────────────────────────────────────────────
PageState MenuDisplay::checkTouch() {
    if (!touch.touched()) return PAGE_NONE;

    TS_Point p = touch.getPoint();

    int x = p.x;
    int y = 320 - p.y;

    if (checkBackButton(x, y)) return PAGE_MENU;

    const int X_MIN = 20;
    const int X_MAX = 220;

    if (x > X_MIN && x < X_MAX) {
        if (y > 60 && y < 100) return PAGE_SNIFFER;
        if (y > 120 && y < 160) return PAGE_JAM;
        if (y > 180 && y < 220) return PAGE_SENSOR;
        if (y > 240 && y < 280) return PAGE_GPS;
    }

    return PAGE_NONE;
}

// ── Pages ────────────────────────────────────────────────────────────────────
void MenuDisplay::showPacketSnifferPage() {
    tft.fillScreen(COL_BLACK);
    tft.setTextColor(COL_WHITE);
    drawText("PKT SNIFFER", 40, 20, 3);
    drawBackButton();
}

void MenuDisplay::showStrawberryJamPage() {
    tft.fillScreen(COL_BLACK);
    tft.setTextColor(COL_RED);
    drawText("STRAWBERRY JAM", 30, 10, 3);
    tft.setTextColor(COL_WHITE);
    drawText("WiFi Deauth Attack", 40, 40, 2);
    drawBackButton();
}

void MenuDisplay::showWirelessSensorPage() {
    tft.fillScreen(COL_BLACK);
    tft.setTextColor(COL_WHITE);
    drawText("WIRELESS SENSOR", 20, 20, 3);
    drawBackButton();
    drawLockButton(false, 1);
}

void MenuDisplay::showGPSPage() {
    tft.fillScreen(COL_BLACK);
    tft.setTextColor(COL_WHITE);
    drawText("GPS PAGE", 80, 20, 3);
    drawBackButton();
}

// ── Sniffer display ──────────────────────────────────────────────────────────
void MenuDisplay::drawSnifferData(WiFiSniffer &sniffer) {
    // Clear stats area (above graph)
    tft.fillRect(0, 60, tft.width(), 100, COL_BLACK);
    tft.setTextColor(COL_WHITE);
    drawText("Pkts: " + String(sniffer.getTotal()), 10, 65, 2);
    drawText("Deauths: " + String(sniffer.getDeauths()), 10, 90, 2);
    drawText("Beacons: " + String(sniffer.getBeacons()), 10, 115, 2);
    drawText("MACs: " + String(sniffer.getUniqueMacs()), 130, 115, 2);

    // Draw the live scrolling packet graph
    drawPacketGraph(sniffer);
}

// ── Live scrolling packet graph ─────────────────────────────────────────────
void MenuDisplay::drawPacketGraph(WiFiSniffer &sniffer) {
    const int GX = 10, GY = 155, GH = 100, GW = GRAPH_W;

    unsigned long currentTotal = sniffer.getTotal();
    if (!_graphInited) {
        _lastTotal = currentTotal;
        _graphInited = true;
    }
    uint16_t delta = (uint16_t)min(currentTotal - _lastTotal, (unsigned long)UINT16_MAX);
    _lastTotal = currentTotal;

    memmove(_graphHistory, &_graphHistory[1], (GW - 1) * sizeof(_graphHistory[0]));
    _graphHistory[GW - 1] = delta;

    uint16_t maxVal = 1;
    for (int i = 0; i < GW; i++) {
        if (_graphHistory[i] > maxVal) maxVal = _graphHistory[i];
    }

    tft.fillRect(GX, GY, GW, GH + 15, COL_BLACK);
    tft.drawRect(GX - 1, GY - 1, GW + 2, GH + 2, COL_DARKGREY);
    for (int g = 1; g <= 3; g++) {
        int gy = GY + GH - (GH * g / 4);
        tft.drawFastHLine(GX, gy, GW, tft.color565(30, 30, 50));
    }

    bool deauthAlert = sniffer.getDeauths() > 5;
    for (int i = 0; i < GW; i++) {
        if (_graphHistory[i] == 0) continue;
        int barH = (int)((unsigned long)_graphHistory[i] * GH / maxVal);
        if (barH > GH) barH = GH;
        if (barH < 1) barH = 1;

        uint16_t color;
        if (deauthAlert && i >= GW - 3) {
            color = COL_RED;
        } else {
            int g = map(barH, 0, GH, 80, 255);
            int b = map(barH, 0, GH, 255, 40);
            color = tft.color565(0, g, b);
        }

        tft.drawFastVLine(GX + i, GY + GH - barH, barH, color);
    }

    tft.setTextColor(COL_DARKGREY);
    drawText("pkt/s", GX, GY + GH + 3, 1);
    tft.setTextColor(COL_CYAN);
    drawText(String(delta) + "/s", GX + 50, GY + GH + 3, 1);
    tft.setTextColor(COL_DARKGREY);
    drawText("max:" + String(maxVal), GX + 130, GY + GH + 3, 1);
}

// ── Strawberry Jam display ───────────────────────────────────────────────────
void MenuDisplay::drawJamData(WiFiSniffer &sniffer, StrawberryJam &jam) {
    tft.fillRect(0, 60, tft.width(), tft.height() - 90, COL_BLACK);

    if (jam.isRunning()) {
        tft.setTextColor(COL_RED);
        drawCentered("JAMMING", tft.width() / 2, 80, 3);

        tft.setTextColor(COL_WHITE);
        drawCentered("Frames sent:", tft.width() / 2, 120, 2);
        tft.setTextColor(COL_YELLOW);
        drawCentered(String(jam.getFramesSent()), tft.width() / 2, 145, 3);

    } else {
        tft.setTextColor(COL_DARKGREY);
        drawCentered("IDLE", tft.width() / 2, 80, 3);

        tft.setTextColor(COL_WHITE);
        drawCentered("Waiting for target...", tft.width() / 2, 120, 2);
    }

    // Show detected devices count
    tft.setTextColor(COL_CYAN);
    drawText("Devices: " + String(sniffer.getDeviceCount()), 10, 180, 2);

    // Show the strongest device (auto-target)
    if (sniffer.getDeviceCount() > 0) {
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
        tft.setTextColor(COL_YELLOW);
        drawText("Target:", 10, 210, 2);
        tft.setTextColor(COL_WHITE);
        drawText(WiFiSniffer::macToString(target.mac), 10, 235, 2);
        drawText("RSSI: " + String(target.rssi) + " dBm", 10, 255, 1);
    }
}

// ── Wireless Sensor: Hot/Cold camera detector display ────────────────────────
void MenuDisplay::drawSensorData(WiFiSniffer &sniffer) {
    tft.fillRect(0, 50, tft.width(), tft.height() - 90, COL_BLACK);

    // Show current channel info at top
    tft.setTextColor(COL_CYAN);
    drawCentered("Ch:" + String(sniffer.getCurrentChannel()) +
        (sniffer.isHopping() ? " [SCAN]" : " [LOCK]"), tft.width() / 2, 55, 1);

    int targetIdx = sniffer.getStrongestSuspicious();

    if (targetIdx >= 0) {
        DeviceStats target = sniffer.getDevice(targetIdx);
        int8_t rssi = target.rssi;

        int r, g, b;
        if (rssi > -55)      { r = 255; g = 0;   b = 0;   }
        else if (rssi > -70) { r = 255; g = 140; b = 0;   }
        else if (rssi > -85) { r = 0;   g = 100; b = 255; }
        else                 { r = 0;   g = 30;  b = 150; }

        uint16_t color = tft.color565(r, g, b);
        tft.fillCircle(tft.width() / 2, 140, 55, color);

        tft.setTextColor(COL_WHITE);
        const char* label;
        if      (rssi > -55) label = "HOT!";
        else if (rssi > -70) label = "WARM";
        else if (rssi > -85) label = "COOL";
        else                 label = "COLD";

        drawCentered(label, tft.width() / 2, 130, 3);
        drawCentered(String(rssi) + " dBm", tft.width() / 2, 155, 2);

        const char* vendor = WiFiSniffer::getOUIVendor(target.mac);
        if (vendor) {
            tft.setTextColor(COL_ORANGE);
            drawCentered(String(vendor), tft.width() / 2, 210, 2);
        }
        tft.setTextColor(COL_YELLOW);
        drawCentered("SUSPECT:", tft.width() / 2, 228, 1);
        tft.setTextColor(COL_WHITE);
        drawCentered(WiFiSniffer::macToString(target.mac), tft.width() / 2, 243, 2);

        int dataPercent = (target.totalPkts > 0) ? (target.dataPkts * 100 / target.totalPkts) : 0;
        tft.setTextColor(COL_LIGHTGREY);
        drawCentered(String(dataPercent) + "% data | L:" +
            String(target.largePkts) + " S:" + String(target.smallPkts),
            tft.width() / 2, 263, 1);
    } else {
        tft.setTextColor(COL_CYAN);
        drawCentered("Scanning...", tft.width() / 2, 100, 3);

        tft.setTextColor(COL_DARKGREY);
        drawCentered("Devices: " + String(sniffer.getDeviceCount()), tft.width() / 2, 140, 2);
        drawCentered("Suspicious: " + String(sniffer.getSuspiciousCount()), tft.width() / 2, 165, 2);

        tft.setTextColor(COL_WHITE);
        drawCentered("Walk around to", tft.width() / 2, 200, 2);
        drawCentered("detect transmitters", tft.width() / 2, 220, 2);
    }
}

// ── Lock/Scan toggle button ──────────────────────────────────────────────────
// Lock button — drawn on opposite side from back button
// Back button is at BACK_BTN_X (right side), so lock goes left
#define LOCK_BTN_X  5
#define LOCK_BTN_Y  BACK_BTN_Y
#define LOCK_BTN_W  90
#define LOCK_BTN_H  BACK_BTN_H

void MenuDisplay::drawLockButton(bool locked, uint8_t channel) {
    tft.fillRect(LOCK_BTN_X, LOCK_BTN_Y, LOCK_BTN_W, LOCK_BTN_H, COL_BLACK);

    uint16_t btnColor = locked ? COL_RED : COL_CYAN;
    tft.fillRect(LOCK_BTN_X, LOCK_BTN_Y, LOCK_BTN_W, LOCK_BTN_H, COL_DARKGREY);
    tft.drawRect(LOCK_BTN_X, LOCK_BTN_Y, LOCK_BTN_W, LOCK_BTN_H, btnColor);
    tft.setTextColor(COL_WHITE);
    if (locked) {
        drawText("LOCK " + String(channel), LOCK_BTN_X + 4, LOCK_BTN_Y + 8, 1);
    } else {
        drawText("SCAN", LOCK_BTN_X + 4, LOCK_BTN_Y + 8, 1);
    }
}

bool MenuDisplay::checkLockButton() {
    if (!touch.touched()) return false;

    TS_Point p = touch.getPoint();
    int x = p.x;
    int y = 320 - p.y;

    // Touch X is inverted from draw X on this panel
    int drawX = 240 - x;

    return (drawX >= LOCK_BTN_X && drawX <= (LOCK_BTN_X + LOCK_BTN_W) &&
            y >= LOCK_BTN_Y && y <= (LOCK_BTN_Y + LOCK_BTN_H));
}

// ── GPS live data display ────────────────────────────────────────────────────
void MenuDisplay::drawGPSData(GPSReader &gps) {
    tft.fillRect(0, 50, tft.width(), tft.height() - 90, COL_BLACK);
    tft.setTextColor(COL_WHITE);

    drawText("Lat:", 10, 60, 2);
    drawText(String(gps.getLatitude(), 6), 80, 60, 2);

    drawText("Lon:", 10, 85, 2);
    drawText(String(gps.getLongitude(), 6), 80, 85, 2);

    drawText("Alt:", 10, 110, 2);
    drawText(String(gps.getAltitude(), 1) + " m", 80, 110, 2);

    drawText("Spd:", 10, 135, 2);
    drawText(String(gps.getSpeed(), 1) + " km/h", 80, 135, 2);

    tft.setTextColor(COL_CYAN);
    drawText("Time:", 10, 170, 2);
    drawText(gps.getDateTime(), 80, 170, 2);
}
