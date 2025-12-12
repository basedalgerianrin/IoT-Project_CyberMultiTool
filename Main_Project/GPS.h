#ifndef GPS_H
#define GPS_H

#include <Arduino.h>
#include <TinyGPS++.h>

class GPSReader {
public:
    GPSReader(int rxPin, int txPin, int baudrate);

    void begin();
    void update();
    void printStatus();

private:
    int _rxPin;
    int _txPin;
    int _baudrate;

    TinyGPSPlus _gps;
    unsigned long _lastPrint = 0;

    void printLocation();
    void printAltitude();
    void printSpeed();
    void printDateTime();
};

#endif
