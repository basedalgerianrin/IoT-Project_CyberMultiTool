#include "MenuDisplay.h"
#include "GPS.h"
#define RXD2 16   // GPS TX → ESP32 RX2
#define TXD2 17   // GPS RX → ESP32 TX2
#define GPS_BAUDRATE 115200


MenuDisplay menuDisplay;
GPSReader gpsReader(RXD2, TXD2, GPS_BAUDRATE);

void setup() {
    Serial.begin(115200);
    delay(200);
    menuDisplay.begin();
    menuDisplay.drawMenu();
    Serial.println("ESP32 + GPS (Class Based)");
    gpsReader.begin();

}
   

void loop() {
    int sel = menu.checkTouch();

    switch (sel) {
        case 1:
            menu.showPacketSnifferPage();
            break;
        case 2:
            menu.showStrawberryJamPage();
            break;
        case 3:
            menu.showWirelessSensorPage();
            break;
        case 4:
            menu.showGPSPage();
            break;
    }
    gpsReader.update();


}

