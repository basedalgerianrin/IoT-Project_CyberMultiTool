#ifndef GPS_H
#define GPS_H

#include <Arduino.h>
#include <TinyGPS++.h>
#include <time.h>

// UTC offset for Irish Standard Time (IST = UTC+1 in summer, GMT = UTC+0 in winter)
#define GPS_UTC_OFFSET_SEC  3600   // +1 hour in seconds

class GPSReader {
public:
    GPSReader(int rxPin, int txPin, int baudrate);
    void begin();
    void update();
    void printStatus();

    // --- Getter methods for TFT display ---
    double getLatitude();
    double getLongitude();
    double getAltitude();
    double getSpeed();
    String getDateTime();

private:
    int _rxPin;
    int _txPin;
    int _baudrate;

    TinyGPSPlus _gps;
    unsigned long _lastPrint;

    void printLocation();
    void printAltitude();
    void printSpeed();
    void printDateTime();
};

#endif
