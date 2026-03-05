#ifndef GPS_H
#define GPS_H

#include <Arduino.h>
#include <TinyGPS++.h> 

class GPSReader {
public:
    // Default pins for ESP32 Serial2: RX=16, TX=17
    GPSReader(int rxPin = 16, int txPin = 17, int baudrate = 9600);
    void begin();
    void update();

    float getLatitude();
    float getLongitude();

private:
    int _rxPin, _txPin, _baudrate;
    TinyGPSPlus _gps;
};

#endif
