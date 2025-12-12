#include "GPS.h"

// ESP32 core requires explicit HardwareSerial object
extern HardwareSerial Serial2; 

GPSReader::GPSReader(int rxPin, int txPin, int baudrate)
  : _rxPin(rxPin), _txPin(txPin), _baudrate(baudrate) {}

void GPSReader::begin() {
    Serial2.begin(_baudrate, SERIAL_8N1, _rxPin, _txPin);
    Serial.println("GPS initialized, waiting for data...");
}

void GPSReader::update() {
    // Feed characters from hardware serial to TinyGPS++
    while (Serial2.available() > 0) {
        _gps.encode(Serial2.read());
    }

    // Print GPS info once per second
    if (millis() - _lastPrint > 1000) {
        _lastPrint = millis();
        printStatus();
    }

    // No GPS data? Wiring issue
    if (millis() > 5000 && _gps.charsProcessed() < 10) {
        Serial.println("** No GPS data received! Check wiring and baudrate **");
        delay(2000);
    }
}

void GPSReader::printStatus() {
    printLocation();
    printAltitude();
    printSpeed();
    printDateTime();
    Serial.println();
}

void GPSReader::printLocation() {
    if (_gps.location.isValid()) {
        Serial.print("LAT= ");
        Serial.println(_gps.location.lat(), 6);
        Serial.print("LNG= ");
        Serial.println(_gps.location.lng(), 6);
    } else {
        Serial.println("Location: INVALID...");
    }
}

void GPSReader::printAltitude() {
    if (_gps.altitude.isValid()) {
        Serial.print("Altitude: ");
        Serial.println(_gps.altitude.meters());
    }
}

void GPSReader::printSpeed() {
    if (_gps.speed.isValid()) {
        Serial.print("Speed: ");
        Serial.print(_gps.speed.kmph());
        Serial.println(" km/h");
    }
}

void GPSReader::printDateTime() {
    if (_gps.date.isValid() && _gps.time.isValid()) {
        Serial.printf("Date: %02u-%02u-%04u\n",
                      _gps.date.day(),
                      _gps.date.month(),
                      _gps.date.year());

        Serial.printf("Time (UTC): %02u:%02u:%02u\n",
                      _gps.time.hour(),
                      _gps.time.minute(),
                      _gps.time.second());
    }
}
