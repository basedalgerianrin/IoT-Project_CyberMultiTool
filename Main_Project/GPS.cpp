#include "GPS.h"

GPSReader::GPSReader(int rxPin, int txPin, int baudrate) 
    : _rxPin(rxPin), _txPin(txPin), _baudrate(baudrate), _lastPrint(0) {
}

void GPSReader::begin() {
    Serial2.begin(_baudrate, SERIAL_8N1, _rxPin, _txPin);
}

void GPSReader::update() {
    while (Serial2.available()) {
        _gps.encode(Serial2.read());
    }
}

void GPSReader::printStatus() {
    Serial.println(F("--- GPS Status ---"));
    printLocation();
    printAltitude();
    printSpeed();
    printDateTime();
}

void GPSReader::printLocation() {
    if (_gps.location.isValid()) {
        Serial.print(F("Location: "));
        Serial.print(_gps.location.lat(), 6);
        Serial.print(F(","));
        Serial.println(_gps.location.lng(), 6);
    } else {
        Serial.println(F("Location: Invalid"));
    }
}

void GPSReader::printAltitude() {
    if (_gps.altitude.isValid()) {
        Serial.print(F("Altitude: "));
        Serial.print(_gps.altitude.meters());
        Serial.println(F("m"));
    } else {
        Serial.println(F("Altitude: Invalid"));
    }
}

void GPSReader::printSpeed() {
    if (_gps.speed.isValid()) {
        Serial.print(F("Speed: "));
        Serial.print(_gps.speed.kmph());
        Serial.println(F(" km/h"));
    } else {
        Serial.println(F("Speed: Invalid"));
    }
}

void GPSReader::printDateTime() {
    if (_gps.date.isValid() && _gps.time.isValid()) {
        Serial.print(F("Date/Time: "));
        Serial.print(_gps.date.day());
        Serial.print(F("/"));
        Serial.print(_gps.date.month());
        Serial.print(F("/"));
        Serial.print(_gps.date.year());
        Serial.print(F(" "));
        Serial.print(_gps.time.hour());
        Serial.print(F(":"));
        Serial.print(_gps.time.minute());
        Serial.println(F(" UTC"));
    } else {
        Serial.println(F("Date/Time: Invalid"));
    }
}

// --- Getter methods ---
float GPSReader::getLatitude() {
    return _gps.location.isValid() ? _gps.location.lat() : 0;
}

float GPSReader::getLongitude() {
    return _gps.location.isValid() ? _gps.location.lng() : 0;
}

float GPSReader::getAltitude() {
    return _gps.altitude.isValid() ? _gps.altitude.meters() : 0;
}

float GPSReader::getSpeed() {
    return _gps.speed.isValid() ? _gps.speed.kmph() : 0;
}

String GPSReader::getDateTime() {
    if (_gps.date.isValid() && _gps.time.isValid()) {
        return String(_gps.date.day()) + "/" +
               String(_gps.date.month()) + "/" +
               String(_gps.date.year()) + " " +
               String(_gps.time.hour()) + ":" +
               String(_gps.time.minute());
    }
    return "Invalid";
}


