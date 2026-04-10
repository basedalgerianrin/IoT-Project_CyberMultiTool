#ifndef MENU_DISPLAY_H
#define MENU_DISPLAY_H

#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_FT6206.h>
#include "WiFiSniffer.h"
#include "StrawberryJam.h"
#include "GPS.h"

enum PageState {
    PAGE_NONE = -1,    // No touch input (do nothing)
    PAGE_MENU = 0,
    PAGE_SNIFFER = 1,
    PAGE_JAM = 2,
    PAGE_SENSOR = 3,
    PAGE_GPS = 4
};

class MenuDisplay {
public:
    MenuDisplay();
    void begin();
    void drawMenu();
    PageState checkTouch();

    bool checkBackButton(int tftX, int tftY);
    void drawBackButton();

    void showPacketSnifferPage();
    void showStrawberryJamPage();
    void showWirelessSensorPage();
    void showGPSPage();

    void drawSnifferData(WiFiSniffer &sniffer);
    void drawSensorData(WiFiSniffer &sniffer);
    void drawJamData(WiFiSniffer &sniffer, StrawberryJam &jam);
    void drawGPSData(GPSReader &gps);

private:
    TFT_eSPI tft;
    Adafruit_FT6206 touch;
};

#endif
