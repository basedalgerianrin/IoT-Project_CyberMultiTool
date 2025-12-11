#include "MenuDisplay.h"

MenuDisplay menuDisplay;

void setup() {
    menuDisplay.begin();
    menuDisplay.drawMenu();
}

void loop() {
    // Nothing — display only
}

