#ifndef HEATMAP_DISPLAY_H
#define HEATMAP_DISPLAY_H

#include "MenuDisplay.h"
#include <TFT_eSPI.h>
#include "WiFiSniffer.h"

struct DeviceInfo {
    int rssi;
    char ssid[33];
};

class HeatmapDisplay {
public:
    HeatmapDisplay();
    void begin();
    void update(DeviceInfo device);

private:
    TFT_eSPI tft;

    int mapRSSIToColor(int rssi);
};

#endif