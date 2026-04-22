#include "GPS.h"

GPSReader::GPSReader(int rxPin, int txPin, int baudrate) 
    : _rxPin(rxPin), _txPin(txPin), _baudrate(baudrate), _lastPrint(0) {
}

void GPSReader::begin() {
    Serial2.begin(_baudrate, SERIAL_8N1, _rxPin, _txPin);

    // Sync time via NTP (fallback when GPS has no satellite fix)
    configTime(GPS_UTC_OFFSET_SEC, 0, "pool.ntp.org", "time.google.com");
    Serial.print(F("[GPS] Syncing NTP time..."));
    struct tm t;
    if (getLocalTime(&t, 5000)) {
        Serial.println(F(" OK"));
    } else {
        Serial.println(F(" failed (no WiFi?)"));
    }
}

void GPSReader::update() {
    while (Serial2.available()) {
        _gps.encode(Serial2.read());
    }

    // if (millis() - _lastPrint >= 1000) {  // Commented out — flooding Serial, hiding IP
    //     _lastPrint = millis();
    //     printStatus();
    // }
}

void GPSReader::printStatus() {
    Serial.println(F("--- GPS Status ---"));

    // Diagnostic: chars processed tells us if wiring is working at all
    Serial.print(F("Chars received: "));
    Serial.println(_gps.charsProcessed());

    if (_gps.charsProcessed() < 10) {
        Serial.println(F("WARNING: No data from GPS. Check wiring - is RX/TX swapped?"));
        Serial.println(F("  GPS TX -> ESP32 GPIO 16 (RX2)"));
        Serial.println(F("  GPS RX -> ESP32 GPIO 17 (TX2)"));
        return;
    }

    Serial.print(F("Sentences with fix: "));
    Serial.println(_gps.sentencesWithFix());

    if (_gps.failedChecksum() > 0) {
        Serial.print(F("WARNING: Failed checksums: "));
        Serial.println(_gps.failedChecksum());
        Serial.println(F("Possible baud rate mismatch or noisy wiring."));
    }

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
        Serial.println(F(" UTC (GPS)"));
    } else {
        struct tm t;
        if (getLocalTime(&t, 0)) {
            Serial.print(F("Date/Time: "));
            Serial.print(t.tm_mday);
            Serial.print(F("/"));
            Serial.print(t.tm_mon + 1);
            Serial.print(F("/"));
            Serial.print(t.tm_year + 1900);
            Serial.print(F(" "));
            Serial.print(t.tm_hour);
            Serial.print(F(":"));
            Serial.print(t.tm_min);
            Serial.println(F(" IST (NTP)"));
        } else {
            Serial.println(F("Date/Time: No fix / No NTP"));
        }
    }
}

// --- Getter methods ---
double GPSReader::getLatitude() {
    return _gps.location.isValid() ? _gps.location.lat() : 0.0;
}

double GPSReader::getLongitude() {
    return _gps.location.isValid() ? _gps.location.lng() : 0.0;
}

double GPSReader::getAltitude() {
    return _gps.altitude.isValid() ? _gps.altitude.meters() : 0.0;
}

double GPSReader::getSpeed() {
    return _gps.speed.isValid() ? _gps.speed.kmph() : 0.0;
}

String GPSReader::getDateTime() {
    if (_gps.date.isValid() && _gps.time.isValid()) {
        char buf[20];
        snprintf(buf, sizeof(buf), "%02d/%02d/%04d %02d:%02d",
                 _gps.date.day(), _gps.date.month(), _gps.date.year(),
                 _gps.time.hour(), _gps.time.minute());
        return String(buf);
    }

    // Fallback: use NTP time when GPS has no fix
    struct tm t;
    if (getLocalTime(&t, 0)) {
        char buf[24];
        snprintf(buf, sizeof(buf), "%02d/%02d/%04d %02d:%02d",
                 t.tm_mday, t.tm_mon + 1, t.tm_year + 1900,
                 t.tm_hour, t.tm_min);
        return String(buf) + " (NTP)";
    }

    return "No fix / No NTP";
}


