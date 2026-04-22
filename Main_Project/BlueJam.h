#ifndef BLUE_JAM_H
#define BLUE_JAM_H

// ╔══════════════════════════════════════════════════════════════════╗
// ║  BLUE JAM — Bluetooth Interference Module (nRF24L01)            ║
// ║                                                                  ║
// ║  FOR EDUCATIONAL / AUTHORIZED TESTING ONLY.                      ║
// ║  Transmitting interference on 2.4 GHz without authorisation      ║
// ║  violates radio regulations in most countries.                   ║
// ╚══════════════════════════════════════════════════════════════════╝
//
// How it works:
//   Bluetooth Classic uses 79 channels (2402-2480 MHz), hopping 1600
//   times per second. BLE uses 40 channels in the same band.
//   The nRF24L01+ radio can transmit on any 2.4 GHz channel.
//   By rapidly hopping across all Bluetooth channels and flooding
//   each with data packets, we create enough interference to disrupt
//   Bluetooth connections.
//
// Hardware:
//   1x nRF24L01+PA+LNA module connected to ESP32 via SPI.
//   Uses HSPI bus (separate from TFT which uses VSPI).
//
// Wiring:
//   nRF24L01    →  ESP32
//   ─────────────────────
//   VCC         →  3.3V  (NOT 5V! nRF24 is 3.3V only)
//   GND         →  GND
//   CE          →  GPIO 25
//   CSN         →  GPIO 26
//   MOSI        →  GPIO 13
//   MISO        →  GPIO 12  (strapping pin — see note below)
//   SCK         →  GPIO 14
//   IRQ         →  not connected
//
//   NOTE: GPIO 12 must be LOW at boot or ESP32 won't start.
//   The nRF24L01 MISO is high-impedance when CSN is HIGH, so it
//   should be fine. If you get boot issues, add a 10K pull-down
//   resistor from GPIO 12 to GND.

#include <Arduino.h>
#include <SPI.h>

// ─── nRF24L01 Pin Assignments (HSPI bus) ─────────────────────────
#define NRF_CE   25
#define NRF_CSN  26
#define NRF_MOSI 13
#define NRF_MISO 12
#define NRF_SCK  14

// ─── Bluetooth Channel Ranges ────────────────────────────────────
// nRF24L01 channel N = frequency (2400 + N) MHz
// Bluetooth Classic: 2402-2480 MHz = nRF channels 2-80 (79 channels)
// BLE: 40 channels within 2402-2480 MHz (2 MHz spacing)

#define BT_CHANNEL_START   2    // 2402 MHz
#define BT_CHANNEL_END    80    // 2480 MHz
#define BT_NUM_CHANNELS   79    // Bluetooth Classic channel count

#define BLE_NUM_CHANNELS   40   // BLE channel count

// ─── Jam Modes ───────────────────────────────────────────────────
enum JamMode {
    JAM_OFF = 0,
    JAM_BLUETOOTH = 1,   // Bluetooth Classic (79 channels)
    JAM_BLE = 2          // Bluetooth Low Energy (40 channels)
};

class BlueJam {
public:
    BlueJam();

    // Initialise the nRF24L01 module. Returns true if the module responds.
    bool begin();

    // Check if nRF24L01 hardware is present and responding
    bool isHardwarePresent();

    // Start/stop jamming
    void start(JamMode mode);
    void stop();
    bool isRunning();
    JamMode getMode();

    // Call from loop() — hops channels and transmits interference.
    // This must be called as fast as possible for effective jamming.
    void update();

    // Stats
    unsigned long getPacketsSent();
    uint8_t getCurrentChannel();

private:
    SPIClass _hspi;
    bool _hardwarePresent;
    bool _running;
    JamMode _mode;
    uint8_t _currentChannel;
    uint8_t _bleHopIndex;
    unsigned long _packetsSent;

    // nRF24L01 raw register access
    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
    void setChannel(uint8_t channel);
    void transmitJunk();
    void powerUp();
    void powerDown();

    // BLE channel mapping (BLE channels don't map 1:1 to frequencies)
    static const uint8_t _bleChannels[BLE_NUM_CHANNELS];
};

#endif
