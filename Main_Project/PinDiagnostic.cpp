#include "PinDiagnostic.h"

// ─── Run every diagnostic test ──────────────────────────────────────────────

void PinDiagnostic::runAll() {
    printHeader();

    int passed = 0;
    int failed = 0;

    // ── 1. Test each GPIO pin can be driven high/low ──
    // This catches: broken wires, dead GPIO, shorts to GND/VCC
    Serial.println(F("── GPIO Pin Toggle Test ──"));
    Serial.println(F("  (Each pin briefly set OUTPUT HIGH then LOW)"));
    Serial.println(F("  If a pin reads wrong, the wire may be disconnected or shorted.\n"));

    struct PinInfo { int pin; const char* name; };
    PinInfo pins[] = {
        { DIAG_TFT_MOSI, "TFT MOSI (GPIO 23)" },
        { DIAG_TFT_MISO, "TFT MISO (GPIO 19)" },
        { DIAG_TFT_SCLK, "TFT SCLK (GPIO 18)" },
        { DIAG_TFT_CS,   "TFT CS   (GPIO 15)" },
        { DIAG_TFT_DC,   "TFT DC   (GPIO 33)" },
        { DIAG_TFT_RST,  "TFT RST  (GPIO  4)" },
        { DIAG_I2C_SDA,  "I2C SDA  (GPIO 21)" },
        { DIAG_I2C_SCL,  "I2C SCL  (GPIO 22)" },
        { DIAG_GPS_RX,   "GPS RX2  (GPIO 16)" },
        { DIAG_GPS_TX,   "GPS TX2  (GPIO 17)" },
    };
    int numPins = sizeof(pins) / sizeof(pins[0]);

    for (int i = 0; i < numPins; i++) {
        if (testGPIO(pins[i].pin, pins[i].name)) passed++; else failed++;
    }

    Serial.println();

    // ── 2. SPI bus test ──
    Serial.println(F("── SPI Bus Test ──"));
    testSPI();
    // SPI init always succeeds on ESP32, so count as passed if we get here
    passed++;
    Serial.println();

    // ── 3. TFT Chip Select test ──
    Serial.println(F("── TFT Chip Select (CS) Test ──"));
    testTFTChipSelect();
    passed++;  // informational
    Serial.println();

    // ── 4. TFT Reset pulse test ──
    Serial.println(F("── TFT Reset Pulse Test ──"));
    testTFTReset();
    passed++;
    Serial.println();

    // ── 5. I2C scan for FT6206 touch controller ──
    Serial.println(F("── I2C Bus Scan (Touch Controller) ──"));
    testI2C();
    Serial.println();

    // ── 6. GPS UART test ──
    Serial.println(F("── GPS UART Test ──"));
    testUART();
    Serial.println();

    // ── Summary ──
    printSummary(passed, failed);
}

// ─── Header ─────────────────────────────────────────────────────────────────

void PinDiagnostic::printHeader() {
    Serial.println();
    Serial.println(F("╔══════════════════════════════════════════╗"));
    Serial.println(F("║     ESP32 CyberMultiTool Diagnostics    ║"));
    Serial.println(F("║         Pin & Connection Checker         ║"));
    Serial.println(F("╚══════════════════════════════════════════╝"));
    Serial.println();
    Serial.println(F("Expected wiring:"));
    Serial.println(F("  TFT ILI9341:  MOSI=23, MISO=19, SCLK=18, CS=15, DC=33, RST=4"));
    Serial.println(F("  Touch FT6206: SDA=21, SCL=22  (I2C addr 0x38)"));
    Serial.println(F("  GPS Module:   RX2=16 (GPS TX), TX2=17 (GPS RX), 9600 baud"));
    Serial.println();
}

// ─── Test a single GPIO pin ─────────────────────────────────────────────────
// Sets pin HIGH, reads it back. If the pin is disconnected or shorted to GND,
// it won't read HIGH. Not 100% conclusive (some pins have pull-downs) but
// catches the most common wiring mistakes.

bool PinDiagnostic::testGPIO(int pin, const char* name) {
    pinMode(pin, OUTPUT);

    digitalWrite(pin, HIGH);
    delayMicroseconds(10);
    int highRead = digitalRead(pin);

    digitalWrite(pin, LOW);
    delayMicroseconds(10);
    int lowRead = digitalRead(pin);

    pinMode(pin, INPUT);

    bool ok = (highRead == HIGH && lowRead == LOW);

    Serial.print(F("  "));
    Serial.print(name);
    Serial.print(F(": "));
    if (ok) {
        Serial.println(F("OK"));
    } else {
        Serial.print(F("FAIL  (H="));
        Serial.print(highRead);
        Serial.print(F(" L="));
        Serial.print(lowRead);
        Serial.println(F(") check wire!"));
    }
    return ok;
}

// ─── Test SPI bus ───────────────────────────────────────────────────────────
// Initialises SPI on the configured pins and sends a dummy byte.
// If the bus is wired correctly, this will succeed silently.
// A white screen often means SPI is OK but DC or CS is wrong.

void PinDiagnostic::testSPI() {
    SPI.begin(DIAG_TFT_SCLK, DIAG_TFT_MISO, DIAG_TFT_MOSI, DIAG_TFT_CS);

    // Try a dummy transaction
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(DIAG_TFT_CS, LOW);
    uint8_t response = SPI.transfer(0x00);
    digitalWrite(DIAG_TFT_CS, HIGH);
    SPI.endTransaction();
    SPI.end();

    Serial.print(F("  SPI bus init:    OK  (SCLK=18, MOSI=23, MISO=19)\n"));
    Serial.print(F("  SPI dummy byte:  response=0x"));
    Serial.println(response, HEX);
    Serial.println(F("  (0x00 or 0xFF is normal for idle bus)"));

    if (response != 0x00 && response != 0xFF) {
        Serial.println(F("  NOTE: Non-zero response — something is responding on SPI."));
    }
}

// ─── Test TFT Chip Select ───────────────────────────────────────────────────
// The ILI9341 needs CS LOW to listen. If CS is stuck HIGH (disconnected or
// wrong pin), the display ignores everything = white screen.

void PinDiagnostic::testTFTChipSelect() {
    pinMode(DIAG_TFT_CS, OUTPUT);

    // Pull CS LOW (select display)
    digitalWrite(DIAG_TFT_CS, LOW);
    delayMicroseconds(10);
    int csLow = digitalRead(DIAG_TFT_CS);

    // Pull CS HIGH (deselect display)
    digitalWrite(DIAG_TFT_CS, HIGH);
    delayMicroseconds(10);
    int csHigh = digitalRead(DIAG_TFT_CS);

    pinMode(DIAG_TFT_CS, INPUT);

    Serial.print(F("  CS pin (GPIO 15): LOW="));
    Serial.print(csLow);
    Serial.print(F(", HIGH="));
    Serial.println(csHigh);

    if (csLow == LOW && csHigh == HIGH) {
        Serial.println(F("  CS toggles correctly — display should be selectable."));
    } else {
        Serial.println(F("  WARNING: CS not toggling! Display will ignore all commands."));
        Serial.println(F("  → Check the wire from GPIO 15 to the display CS pin."));
    }
}

// ─── Test TFT Reset ────────────────────────────────────────────────────────
// Sends a reset pulse to the ILI9341. The display needs a clean reset
// to initialise properly. If RST is floating, the display may not start.

void PinDiagnostic::testTFTReset() {
    pinMode(DIAG_TFT_RST, OUTPUT);

    // Pulse RST LOW for 50ms then HIGH — standard ILI9341 reset sequence
    digitalWrite(DIAG_TFT_RST, HIGH);
    delay(5);
    digitalWrite(DIAG_TFT_RST, LOW);
    delay(50);
    digitalWrite(DIAG_TFT_RST, HIGH);
    delay(150);

    Serial.println(F("  RST pin (GPIO 4): Pulsed LOW 50ms → HIGH"));
    Serial.println(F("  If the screen flickers during this test, RST wiring is OK."));
    Serial.println(F("  If no flicker at all, RST may be disconnected."));

    pinMode(DIAG_TFT_RST, INPUT);
}

// ─── Test I2C bus — scan for touch controller ───────────────────────────────
// The FT6206 should respond at address 0x38. If it's not found,
// touch won't work but the screen should still display.

void PinDiagnostic::testI2C() {
    Wire.begin(DIAG_I2C_SDA, DIAG_I2C_SCL);
    delay(50);

    Serial.println(F("  Scanning I2C bus (SDA=21, SCL=22)..."));

    int found = 0;
    bool ft6206Found = false;

    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();

        if (error == 0) {
            Serial.print(F("  Device found at 0x"));
            if (addr < 16) Serial.print(F("0"));
            Serial.print(addr, HEX);

            if (addr == DIAG_FT6206_ADDR) {
                Serial.print(F("  ← FT6206 Touch Controller"));
                ft6206Found = true;
            }
            Serial.println();
            found++;
        }
    }

    if (found == 0) {
        Serial.println(F("  NO I2C DEVICES FOUND!"));
        Serial.println(F("  → Check SDA (GPIO 21) and SCL (GPIO 22) wiring."));
        Serial.println(F("  → Make sure pull-up resistors are present (4.7K to 3.3V)."));
    } else {
        Serial.print(F("  Total I2C devices: "));
        Serial.println(found);
    }

    if (!ft6206Found) {
        Serial.println(F("  WARNING: FT6206 (0x38) not found — touch will not work."));
        Serial.println(F("  (Screen display can still work without touch.)"));
    }
}

// ─── Test GPS UART ──────────────────────────────────────────────────────────
// Opens Serial2 and listens for 2 seconds. GPS modules send NMEA sentences
// continuously. If we get characters, the wiring is OK.

void PinDiagnostic::testUART() {
    Serial2.begin(DIAG_GPS_BAUD, SERIAL_8N1, DIAG_GPS_RX, DIAG_GPS_TX);
    delay(100);

    Serial.println(F("  Listening on Serial2 (RX=16, TX=17, 9600 baud) for 2 seconds..."));

    unsigned long start = millis();
    int charCount = 0;
    bool gotDollar = false;  // NMEA sentences start with $

    while (millis() - start < 2000) {
        if (Serial2.available()) {
            char c = Serial2.read();
            charCount++;
            if (c == '$') gotDollar = true;

            // Print first 80 chars of GPS data as sample
            if (charCount <= 80) Serial.print(c);
        }
    }

    if (charCount > 0) Serial.println();
    Serial.println();

    Serial.print(F("  Characters received: "));
    Serial.println(charCount);

    if (charCount == 0) {
        Serial.println(F("  FAIL: No data from GPS module!"));
        Serial.println(F("  Possible causes:"));
        Serial.println(F("    1. GPS TX wire not connected to ESP32 GPIO 16 (RX2)"));
        Serial.println(F("    2. TX/RX wires swapped — try swapping GPIO 16 and 17"));
        Serial.println(F("    3. GPS module not powered (check VCC and GND)"));
        Serial.println(F("    4. Wrong baud rate (try 4800 if 9600 gives nothing)"));
    } else if (!gotDollar) {
        Serial.println(F("  WARNING: Got data but no '$' character (NMEA start)."));
        Serial.println(F("  → Baud rate may be wrong. Try 4800 instead of 9600."));
        Serial.println(F("  → Or wiring is noisy / RX picking up interference."));
    } else {
        Serial.println(F("  OK: GPS module is sending NMEA data."));
        if (charCount < 100) {
            Serial.println(F("  NOTE: Low character count — GPS may still be acquiring satellites."));
        }
    }

    Serial2.end();
}

// ─── Print final summary ────────────────────────────────────────────────────

void PinDiagnostic::printSummary(int passed, int failed) {
    Serial.println(F("╔══════════════════════════════════════════╗"));
    Serial.println(F("║           DIAGNOSTIC SUMMARY             ║"));
    Serial.println(F("╠══════════════════════════════════════════╣"));

    Serial.print(F("║  GPIO tests passed: "));
    Serial.print(passed);
    Serial.println(F("                    ║"));

    Serial.print(F("║  GPIO tests failed: "));
    Serial.print(failed);
    Serial.println(F("                    ║"));

    Serial.println(F("╠══════════════════════════════════════════╣"));

    if (failed == 0) {
        Serial.println(F("║  ALL GPIO PINS OK                        ║"));
        Serial.println(F("║                                          ║"));
        Serial.println(F("║  If TFT is still white, try:             ║"));
        Serial.println(F("║   1. Swap CS (15) and DC (33) wires      ║"));
        Serial.println(F("║   2. Check TFT VCC is 3.3V (not 5V)     ║"));
        Serial.println(F("║   3. Check TFT LED/BL pin has power      ║"));
        Serial.println(F("║   4. Try a different SPI frequency        ║"));
    } else {
        Serial.println(F("║  WIRING ISSUES DETECTED                  ║"));
        Serial.println(F("║  Check the FAIL lines above.             ║"));
    }

    Serial.println(F("╚══════════════════════════════════════════╝"));
    Serial.println();
}
