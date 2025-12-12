#include <TinyGPS++.h>

#define RXD2 16   // GPS TX → ESP32 RX2
#define TXD2 17   // GPS RX → ESP32 TX2
#define GPS_BAUDRATE 9600

TinyGPSPlus gps;

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 + NEO-6M GPS Test");

  // Start hardware UART2
  Serial2.begin(GPS_BAUDRATE, SERIAL_8N1, RXD2, TXD2);

  Serial.println("Waiting for GPS data...");
}

void loop() {

  // Feed data from GPS to TinyGPS++
  while (Serial2.available() > 0) {
    char c = Serial2.read();
    gps.encode(c);
  }

  // Display location once per second
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 1000) {
    lastPrint = millis();

    if (gps.location.isValid()) {
      Serial.print("LAT= ");
      Serial.println(gps.location.lat(), 6);
      Serial.print("LNG= ");
      Serial.println(gps.location.lng(), 6);
    } else {
      Serial.println("Location: INVALID...");
    }

    if (gps.altitude.isValid()) {
      Serial.print("Altitude: ");
      Serial.println(gps.altitude.meters());
    }

    if (gps.speed.isValid()) {
      Serial.print("Speed: ");
      Serial.print(gps.speed.kmph());
      Serial.println(" km/h");
    }

    if (gps.date.isValid() && gps.time.isValid()) {
      Serial.printf("Date: %02u-%02u-%04u\n",
                    gps.date.day(),
                    gps.date.month(),
                    gps.date.year());
      Serial.printf("Time (UTC): %02u:%02u:%02u\n",
                    gps.time.hour(),
                    gps.time.minute(),
                    gps.time.second());
    }

    Serial.println();
  }

  // If no data received within 5s → wiring issue
  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println("** No GPS data received! Check wiring and baudrate **");
    delay(2000);
  }
}

