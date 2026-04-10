# IoT CyberMultiTool

A portable cybersecurity multitool built on the **ESP32 DOIT DevKit V1**, featuring a touchscreen interface, WiFi packet analysis, hidden camera detection, signal jamming capabilities, and GPS tracking.

> **DISCLAIMER:** This project is developed strictly for **educational and authorized testing purposes** as part of an IoT module project at Atlantic Technological University. Unauthorized use of deauthentication or jamming features violates radio regulations in most jurisdictions. Use only on your own networks in a private lab environment.

## Features

### Packet Sniffer
- Real-time 802.11 WiFi packet capture using ESP32 promiscuous mode
- Frame classification: Beacon, Deauth, Probe Request/Response, Data
- Per-device tracking with MAC address logging
- Wireshark-style packet log on the web interface
- Summary dashboard with packet counts and unique MAC tracking

### Wireless Sensor (Hidden Camera Detector)
- Detects hidden WiFi cameras using three heuristics based on academic research ([DeWiCam](https://doi.org/10.1145/3098243.3098257), [LocCams](https://doi.org/10.1145/3495243.3560530)):
  - **High data-frame ratio** (>60% data frames = streaming device)
  - **Bimodal frame size distribution** (large I-frames + small P-frames = video codec signature)
  - **Known camera OUI lookup** (Hikvision, Dahua, Espressif, Wyze, Tuya)
- Hot/cold proximity indicator using RSSI exponential moving average
- Channel hopping across channels 1, 6, 11 with auto-lock on suspect's channel

### Strawberry Jam (WiFi Deauthentication)
- 802.11 deauthentication frame injection via `esp_wifi_80211_tx()`
- Auto-targets the strongest nearby device
- Configurable burst count and timing
- Live frame counter on TFT and web interface

### Blue Jam (Bluetooth Jammer)
- Bluetooth Classic jamming across 79 channels (2402-2480 MHz)
- BLE jamming across 40 channels with correct frequency mapping
- Uses nRF24L01+PA+LNA module via HSPI (raw register access, no library needed)
- 2 Mbps data rate, maximum TX power, 32-byte random payloads
- Gracefully disabled if nRF24 hardware is not connected

### GPS Tracking
- Real-time position, altitude, speed, and UTC time via TinyGPS++
- Live display on TFT and web interface

### Web Server Interface
- Dark-themed responsive web dashboard accessible from any browser
- Auto-refreshing pages for all modules (Sniffer, Sensor, Jam, GPS)
- Color-coded packet types, device tables with vendor identification
- Hot/cold gradient indicator for camera proximity
- Accessible via `http://esp32.local` (mDNS)

### Hardware Diagnostics
- Comprehensive pin checker run at startup
- GPIO toggle test for all 10 connected pins
- SPI bus verification, TFT chip select and reset pulse tests
- I2C scan for FT6206 touch controller (address 0x38)
- GPS UART test with 2-second NMEA listen

## Hardware

### Components
| Component | Model | Connection |
|-----------|-------|------------|
| Microcontroller | ESP32 DOIT DevKit V1 | - |
| Display | 2.8" ILI9341 TFT (240x320) | SPI (VSPI) |
| Touch | Adafruit FT6206 Capacitive | I2C |
| GPS | NEO-6M / compatible | UART (Serial2) |
| BT Jammer | nRF24L01+PA+LNA | SPI (HSPI) |

### Pin Wiring

#### TFT Display (ILI9341) - VSPI
| TFT Pin | ESP32 GPIO |
|---------|-----------|
| MOSI | 23 |
| MISO | 19 |
| SCLK | 18 |
| CS | 15 |
| DC | 33 |
| RST | 4 |

#### Touch Controller (FT6206) - I2C
| Touch Pin | ESP32 GPIO |
|-----------|-----------|
| SDA | 21 |
| SCL | 22 |

#### GPS Module - UART
| GPS Pin | ESP32 GPIO |
|---------|-----------|
| TX | 16 (RX2) |
| RX | 17 (TX2) |

#### nRF24L01 - HSPI
| nRF24 Pin | ESP32 GPIO |
|-----------|-----------|
| CE | 25 |
| CSN | 26 |
| MOSI | 13 |
| MISO | 12* |
| SCK | 14 |

> *GPIO 12 is a strapping pin. Add a 10K pull-down resistor if you experience boot issues.

## Software Dependencies

- **Arduino IDE 2.3.8+** with ESP32 Board Package (v3.3.7)
- **TFT_eSPI** - Display driver (configure `User_Setup.h` for ILI9341 + pin assignments)
- **Adafruit FT6206 Library** - Capacitive touch controller
- **TinyGPS++** - NMEA GPS sentence parsing
- **ESP32 WiFi / esp_wifi** - Built-in (promiscuous mode, raw frame TX)

No external library needed for the nRF24L01 - uses raw SPI register access.

## Building

1. Clone the repository
2. Open `Main_Project/Main_Project.ino` in Arduino IDE
3. Configure TFT_eSPI `User_Setup.h` for ILI9341 with the pin assignments above
4. Update WiFi credentials in `Main_Project.ino` line 22
5. Select board: **DOIT ESP32 DevKit V1**
6. Compile and upload

```
Sketch uses ~1,072 KB (81%) of program storage space
Global variables use ~53 KB (16%) of dynamic memory
```

## Project Structure

```
Main_Project/
  Main_Project.ino      # Entry point, page state machine, module orchestration
  WiFiSniffer.h/.cpp    # Packet capture, device tracking, camera detection
  MenuDisplay.h/.cpp    # TFT display, touch handling, all page renderers
  StrawberryJam.h/.cpp  # WiFi deauthentication module
  BlueJam.h/.cpp        # Bluetooth jammer (nRF24L01)
  GPS.h/.cpp            # GPS reader on Serial2
  WebMenuServer.h/.cpp  # Web server with dark-themed dashboard
  PinDiagnostic.h/.cpp  # Hardware connection diagnostics
  WebServer.h           # ESP32 core 3.x FS namespace fix
```

## Architecture

The system uses a page-based state machine on the main loop:

- **PAGE_MENU** - Main menu with touch buttons for each module
- **PAGE_SNIFFER** - Live packet counts (refreshes every 1s)
- **PAGE_JAM** - Deauth + BT jam status with auto-targeting (refreshes every 500ms)
- **PAGE_SENSOR** - Hot/cold camera proximity with channel hopping (refreshes every 1s)
- **PAGE_GPS** - Coordinates, altitude, speed, time (refreshes every 2s)

Thread safety is maintained via `portMUX_TYPE` spinlock between the WiFi promiscuous callback (runs on WiFi task) and main loop readers.

## Authors

- IoT Module Project - Atlantic Technological University

## License

For educational use only. See disclaimer above.
