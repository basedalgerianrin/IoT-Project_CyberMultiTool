
#ifndef MENU_DISPLAY_H
#define MENU_DISPLAY_H

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_FT6206.h>
#include "WiFiSniffer.h"
#include "StrawberryJam.h"
#include "GPS.h"

// TFT pin definitions (VSPI)
#define TFT_CS   15
#define TFT_DC   33
#define TFT_RST   4

// Color aliases for readability
#define COL_BLACK    ILI9341_BLACK
#define COL_WHITE    ILI9341_WHITE
#define COL_RED      ILI9341_RED
#define COL_GREEN    ILI9341_GREEN
#define COL_BLUE     ILI9341_BLUE
#define COL_CYAN     ILI9341_CYAN
#define COL_YELLOW   ILI9341_YELLOW
#define COL_ORANGE   ILI9341_ORANGE
#define COL_DARKGREY ILI9341_DARKGREY
#define COL_LIGHTGREY ILI9341_LIGHTGREY

// Global TFT display object — single driver, no conflicts
extern Adafruit_ILI9341 tft;

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

    // Lock/Scan toggle button on sensor page
    // Returns true if the lock button was pressed
    bool checkLockButton();
    void drawLockButton(bool locked, uint8_t channel);

private:
    Adafruit_FT6206 touch;

    // ── Live packet graph (scrolling bar chart like WiFi traffic monitor) ──
    static const int GRAPH_W = 220;       // graph width in pixels (one bar per pixel)
    uint16_t _graphHistory[220];            // packet count per sample period
    unsigned long _lastTotal;              // previous total for delta calculation
    bool _graphInited;                     // flag to zero-fill on first use
    void drawPacketGraph(WiFiSniffer &sniffer);

    // Helpers — replace TFT_eSPI drawString/setTextDatum
    void drawText(const char* text, int16_t x, int16_t y, uint8_t size);
    void drawText(const String &text, int16_t x, int16_t y, uint8_t size);
    void drawCentered(const char* text, int16_t cx, int16_t cy, uint8_t size);
    void drawCentered(const String &text, int16_t cx, int16_t cy, uint8_t size);
};

#endif
