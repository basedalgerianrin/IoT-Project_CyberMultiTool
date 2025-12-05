#include <TFT_eSPI.h>   // Graphics and touch library
#include <SPI.h>
#include <Adafruit_FT6206.h>

//Create Touchscreen object
Adafruit_FT6206 ctp = Adafruit_FT6206();

// Create TFT object
TFT_eSPI tft = TFT_eSPI(); 

// Touch screen calibration data (example — you’ll need to calibrate yours)
#define TOUCH_CALIBRATION 1

// Button structure
struct Button {
  int x, y, w, h;
  String label;
  uint16_t color;
};

Button buttons[3] = {
  {40, 60, 160, 40, "Start",  TFT_GREEN},
  {40, 120,160, 40, "Settings", TFT_BLUE},
  {40, 180,160, 40, "Exit",   TFT_RED}
};

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  Wire.begin(21,22);
  // Optional: Calibrate touch (run tft.getTouchRaw() if needed)
  drawMenu();
Serial.begin(9600);
if(!ctp.begin(40)){
  Serial.println("Couldnt function");
  while(1);
}
  Serial.println("Could function");

}

void loop() {
  uint16_t x, y;
  
if(!ctp.begin(40)){
  Serial.println("Couldnt function");
  while(1);
}
  Serial.println("Could function");

  // Check if the screen is touched
  if (tft.getTouch(&x, &y)) {
    int pressed = checkButtons(x, y);
    if (pressed != -1) {
      handleButton(pressed);
      delay(500); // debounce
      drawMenu(); // redraw after action
    }
  }
}

// Draw the main menu
void drawMenu() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Main Menu", 120, 20, 2);

  for (int i = 0; i < 3; i++) {
    drawButton(buttons[i]);
  }
}

// Draw individual button
void drawButton(Button btn) {
  tft.fillRoundRect(btn.x, btn.y, btn.w, btn.h, 8, btn.color);
  tft.drawRoundRect(btn.x, btn.y, btn.w, btn.h, 8, TFT_WHITE);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString(btn.label, btn.x + btn.w / 2, btn.y + 10, 2);
}

// Detect which button was pressed
int checkButtons(uint16_t x, uint16_t y) {
  for (int i = 0; i < 3; i++) {
    if (x > buttons[i].x && x < (buttons[i].x + buttons[i].w) &&
        y > buttons[i].y && y < (buttons[i].y + buttons[i].h)) {
      return i;
    }
  }
  return -1;
}

// Handle button actions
void handleButton(int index) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("You pressed: " + buttons[index].label, 120, 120, 2);
}

