#ifndef PIN_DIAGNOSTIC_H
#define PIN_DIAGNOSTIC_H

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

// ─── Pin assignments for all modules ─────────────────────────────────────────
// TFT Display (ILI9341) — SPI
#define DIAG_TFT_MISO  19
#define DIAG_TFT_MOSI  23
#define DIAG_TFT_SCLK  18
#define DIAG_TFT_CS    15
#define DIAG_TFT_DC    33
#define DIAG_TFT_RST    4

// Touch Controller (FT6206) — I2C
#define DIAG_I2C_SDA   21
#define DIAG_I2C_SCL   22
#define DIAG_FT6206_ADDR 0x38  // FT6206 default I2C address

// GPS Module — UART (Serial2)
#define DIAG_GPS_RX    16
#define DIAG_GPS_TX    17
#define DIAG_GPS_BAUD  9600

class PinDiagnostic {
public:
    // Run all diagnostics and print results to Serial.
    // Call this in setup() BEFORE initialising any modules,
    // so we test the raw hardware connections first.
    static void runAll();

private:
    static void printHeader();
    static bool testGPIO(int pin, const char* name);
    static void testSPI();
    static void testTFTChipSelect();
    static void testTFTReset();
    static void testI2C();
    static void testUART();
    static void printSummary(int passed, int failed);
};

#endif
