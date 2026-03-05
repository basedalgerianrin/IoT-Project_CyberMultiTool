#include "HeatMap.h"
#include "MenuDisplay.h"

HeatmapDisplay::HeatmapDisplay() : tft(TFT_eSPI()) {}

void HeatmapDisplay::begin() {
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
}

int HeatmapDisplay::mapRSSIToColor(int rssi) {
    // Map -90..-30 dBm to 0..255
    int val = map(rssi, -90, -30, 0, 255);
    val = constrain(val, 0, 255);
    return val;
}

void HeatmapDisplay::update(DeviceInfo device) {
    tft.fillScreen(TFT_BLACK);

    int colorVal = mapRSSIToColor(device.rssi);
    uint16_t color = tft.color565(colorVal, 0, 255 - colorVal); // red=hot, blue=cold

    // Display as rectangle in center
    tft.fillRect(60, 80, 120, 160, color);

    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("RSSI: " + String(device.rssi), tft.width()/2, 50, 4);
}
