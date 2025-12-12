#ifndef MENU_DISPLAY_H
#define MENU_DISPLAY_H

#include <TFT_eSPI.h>
#include <SPI.h>
#include <Adafruit_FT6206.h>

class MenuDisplay {
public:
    MenuDisplay();

    void begin();
    void drawMenu();

    int checkTouch();  // Returns selected menu option (1–4), or 0 if none

    // Example sub-pages
    void showPacketSnifferPage();
    void showStrawberryJamPage();
    void showWirelessSensorPage();
    void showGPSPage();

private:
    TFT_eSPI tft;
    Adafruit_FT6206 touch;
};

#endif


