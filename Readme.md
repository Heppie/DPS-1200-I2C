# HP DPS Control — ESP32-C3 WiFi Dashboard

Monitor and control HP Proliant "Common Slot" server power supplies (DPS-460, DPS-750, DPS-1200) over WiFi from any browser, using an ESP32-C3 and the original I2C adapter hardware.

> This branch is the ESP32-C3 port. The original Arduino Uno sketches live on the `master` branch.

---

## Features

- **First-boot WiFi provisioning** — connect to the `HP-DPS-Setup` access point, enter your WiFi credentials in a browser, and the device saves them and reboots onto your network
- **Live web dashboard** at `http://hp-dps-control.local` showing:
  - Input voltage & current (AC grid)
  - Output voltage & current (12 V DC)
  - Internal temperature
  - Fan RPM
- **Power on/off** toggle button (via GPIO → optocoupler)
- **Fan speed control** slider (0–100%)
- **Re-provisioning** — navigate to `/reset` to clear saved credentials and return to AP mode

---

## Hardware

### Power Supply

Any HP Proliant Common Slot supply works — DPS-460, DPS-750, or DPS-1200. These are widely available used for under $20. The I2C bus is a 3.3 V interface; the adapter handles all level shifting so the ESP32-C3 connects directly.

### Adapter

Use the original Butt Simple Ideas adapter (see `master` branch). It provides:
- 3.3 V regulator
- 5 V ↔ 3.3 V I2C level shift (not needed for ESP32-C3, but harmless)
- Optocoupler for on/off control

### Wiring to ESP32-C3

| Adapter pin | ESP32-C3 GPIO |
|-------------|---------------|
| SDA         | GPIO 4        |
| SCL         | GPIO 5        |
| On/Off      | GPIO 3        |
| 3.3 V       | 3V3           |
| GND         | GND           |

> **Note:** The default I2C address is 0x5F for the PIC and 0x57 for the EEPROM (A0–A2 jumpers all open). If you cannot communicate with the supply, try swapping SDA and SCL — some Common Slot variants require this.

---

## Building & Flashing

This is a [PlatformIO](https://platformio.org/) project targeting the `esp32-c3-devkitm-1` board.

```bash
cd esp32/
pio run --target upload
pio device monitor
```

All required libraries (`Wire`, `WiFi`, `WebServer`, `DNSServer`, `Preferences`, `ESPmDNS`) are bundled with the ESP32 Arduino framework — no external dependencies.

---

## First-Time Setup

1. Flash the firmware.
2. On first boot (no saved credentials), the device starts an open access point called **HP-DPS-Setup**.
3. Connect your phone or laptop to that network — a captive portal should open automatically. If not, navigate to `http://192.168.4.1`.
4. Enter your WiFi SSID and password and tap **Connect**.
5. The device saves the credentials, reboots, and connects to your network.
6. The Serial monitor prints the assigned IP address, and the dashboard is accessible at `http://hp-dps-control.local`.

To re-provision (e.g. to change networks), navigate to `http://hp-dps-control.local/reset`.

---

## Project Structure

```
esp32/
├── platformio.ini
└── src/
    ├── main.cpp          — boot logic, WiFi connect, poll loop
    ├── hpdps.h/.cpp      — I2C driver (register reads/writes, CRC, scaling)
    ├── provisioning.h/.cpp — AP mode captive portal, NVS credential storage
    └── dashboard.h/.cpp  — HTTP web server, JSON data endpoint, dashboard HTML
```

---

## I2C Protocol Notes

The Common Slot supplies use a proprietary protocol (not PMBus/SMBus). All PIC reads and writes require a CRC-8 checksum:

- **Polynomial:** P(x) = x⁸ + x² + x¹ + x⁰
- Read checksum: `((0xFF − ((addr<<1) + reg)) + 1) & 0xFF`
- Write checksum: `((0xFF − ((addr<<1) + reg + LSB + MSB)) + 1) & 0xFF`

EEPROM reads and writes do **not** require a checksum.

| Register | Measurement        | Scale  | Unit |
|----------|--------------------|--------|------|
| 0x08     | Input voltage      | ÷ 32   | V AC |
| 0x0A     | Input current      | ÷ 128  | A    |
| 0x0E     | Output voltage     | ÷ 256  | V DC |
| 0x10     | Output current     | ÷ 128  | A    |
| 0x1C     | Temperature        | ÷ 32   | °F   |
| 0x1E     | Fan speed          | raw    | RPM  |
| 0x40     | Fan PWM (write)    | 0–0x1000 | —  |

---

## Credits

This project builds on the original work by Butt Simple Ideas, LLC and the reverse-engineering community:

- **Original Arduino sketches & adapter hardware:** [Butt Simple Ideas](http://www.buttsimpleideas.com/) — Garry Mercaldi
- **DPS-1200 reverse engineering:** [Dr. Tune / Richard Aplin](https://github.com/raplin/DPS-1200FB)
- **DPS charger project:** [slundell](https://github.com/slundell/dps_charger)
