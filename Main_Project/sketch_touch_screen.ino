#include <TFT_eSPI.h>   // Graphics and touch library
#include <SPI.h>


TFT_eSPI tft = TFT_eSPI();

void setup() {
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextFont(2);

 tft.drawString("MAIN MENU", 60, 10, 4);

  // Option boxes in portrait layout
  tft.drawRect(20, 60, 200, 40, TFT_WHITE);
  tft.drawString("Packet sniffer", 40, 70, 2);

  tft.drawRect(20, 120, 200, 40, TFT_WHITE);
  tft.drawString("Strawberry Jam", 40, 130, 2);

  tft.drawRect(20, 180, 200, 40, TFT_WHITE);
  tft.drawString("Wireless sensor", 40, 190, 2);

  tft.drawRect(20, 240, 200, 40, TFT_WHITE);
  tft.drawString("GPS", 40, 250, 2);
}

void loop() {
  // Nothing — display only
}