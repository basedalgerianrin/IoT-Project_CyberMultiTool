#include "GPS.h"

GPSReader::GPSReader(int rxPin, int txPin, int baudrate) 
    : _rxPin(rxPin), _txPin(txPin), _baudrate(baudrate) {}

void GPSReader::begin() {
    Serial2.begin(_baudrate, SERIAL_8N1, _rxPin, _txPin);
}

void GPSReader::update() {
    while (Serial2.available() > 0) {
        _gps.encode(Serial2.read());
    }
}

float GPSReader::getLatitude() {
    return _gps.location.isValid() ? _gps.location.lat() : 0.0;
}

float GPSReader::getLongitude() {
    return _gps.location.isValid() ? _gps.location.lng() : 0.0;
}

