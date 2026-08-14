# HP DPS Control — ESP32-C3 WiFi Dashboard

Monitor and control HP Proliant "Common Slot" server power supplies (DPS-460, DPS-750, DPS-1200) over WiFi from any browser, using an ESP32-C3.

> This branch is the ESP32-C3 port. The original Arduino Uno sketches live on the `master` branch.

---

## Features

- **First-boot WiFi provisioning** — connect to the `HP-DPS-Setup` access point, scan for networks, pick yours, enter the password, and the device saves and reboots
- **Falls back to AP mode** if it can't reach the configured network (credentials kept)
- **Live web dashboard** at `http://hp-dps-control.local` showing:
  - Input voltage & current (AC grid)
  - Output voltage & current (12 V DC)
  - Calculated input & output power (W)
  - Efficiency (%)
  - Temperature (°C)
  - Fan RPM
  - PSU model and part number (read from EEPROM)
- **Fan speed control** slider (0–100%)
- **Power on/off** toggle (optional — requires GPIO wiring, see below)
- **0.42" OLED display** support — shows AP IP, connecting status, and live readings; auto-detected at boot
- **Re-provisioning** — navigate to `/reset` to clear saved credentials and return to AP mode

---

## Hardware

### Power Supply

Any HP Proliant Common Slot supply — DPS-460, DPS-750, or DPS-1200. Widely available used for under $20. The PSU I2C bus runs at 3.3 V.

### ESP32-C3 Board

Tested with the ESP32-C3 development board with integrated 0.42" SSD1306 OLED ([AliExpress](https://www.aliexpress.com/item/1005007892774677.html)). Any ESP32-C3 board works — the OLED is optional.

### Wiring

Connect the PSU I2C connector directly to the ESP32-C3:

| PSU pin | ESP32-C3 GPIO | Note                          |
|---------|---------------|-------------------------------|
| SDA     | GPIO 5        |                               |
| SCL     | GPIO 6        |                               |
| GND     | GND           |                               |
| On/Off  | GPIO 3        | Optional — see below          |

> **3.3 V only.** The ESP32-C3 is not 5 V tolerant. The PSU I2C bus is 3.3 V so no level shifting is needed for a direct connection.

> **Note:** The default I2C addresses are 0x5F (PIC) and 0x57 (EEPROM). If you can't communicate with the supply, try swapping SDA and SCL — some Common Slot variants require this.

### Optional: Power On/Off Control

To enable the power button on the dashboard, wire GPIO 3 to the PSU PS_ON line via an optocoupler, then uncomment `-DENABLE_ONOFF` in `platformio.ini`. Without this wiring the button is disabled but everything else works normally.

### Optional: OLED Display

A 0.42" SSD1306 OLED on the same I2C bus (GPIO 5/6) is auto-detected at boot. If found it shows:

- **AP mode:** SSID and IP to connect to for setup
- **Connecting:** target network name
- **Running:** Vout, Iout, power, efficiency, temperature, fan RPM, and IP address

---

## Building & Flashing

[PlatformIO](https://platformio.org/) project targeting `esp32-c3-devkitm-1`.

```bash
cd esp32/
pio run --target upload
pio device monitor
```

Dependencies are declared in `platformio.ini` and fetched automatically:

- `WebServer` — HTTP server
- `Adafruit SSD1306` + `Adafruit GFX Library` — OLED display

---

## First-Time Setup

1. Flash the firmware.
2. On first boot the device starts an open access point: **HP-DPS-Setup**.
3. Connect your phone or laptop to that network — a captive portal opens automatically. If not, go to `http://192.168.4.1`.
4. Hit **Scan for Networks**, pick yours from the list, enter the password.
5. Tap **Save & Connect**. The device reboots and joins your network.
6. The dashboard is at `http://hp-dps-control.local`.

To change networks, go to `http://hp-dps-control.local/reset`.

---

## Project Structure

```
esp32/
├── platformio.ini
└── src/
    ├── main.cpp              — boot sequence, WiFi connect, poll loop
    ├── hpdps.h / hpdps.cpp   — I2C driver: register reads/writes, CRC, scaling
    ├── provisioning.h / .cpp — AP captive portal, NVS credential & display storage
    ├── dashboard.h / .cpp    — HTTP server, JSON endpoint, dashboard HTML/CSS/JS
    └── oled.h / oled.cpp     — SSD1306 display (AP, connecting, live readings)
```

---

## I2C Protocol Notes

The Common Slot supplies use a proprietary protocol (not PMBus/SMBus). All PIC (0x5F) reads and writes require a CRC-8 checksum. EEPROM (0x57) reads require no checksum.

- Read checksum: `((0xFF − ((addr<<1) + reg)) + 1) & 0xFF`
- Write checksum: `((0xFF − ((addr<<1) + reg + LSB + MSB)) + 1) & 0xFF`

| Register   | Measurement        | Scale    | Unit |
|------------|--------------------|----------|------|
| 0x08       | Input voltage      | ÷ 32     | V AC |
| 0x0A       | Input current      | ÷ 128    | A    |
| 0x0E       | Output voltage     | ÷ 256    | V DC |
| 0x10       | Output current     | ÷ 128    | A    |
| 0x1C       | Temperature        | ÷ 32, then (raw − 32) × 5/9 | °C |
| 0x1E       | Fan speed          | raw      | RPM  |
| 0x40       | Fan PWM (write)    | 0–0x1000 | —    |
| EEPROM 0x32–0x4B | Model name   | 26 bytes | ASCII |
| EEPROM 0x4D–0x56 | Part number  | 10 bytes | ASCII |

---

## Credits

- **Original Arduino sketches & adapter hardware:** [Butt Simple Ideas](http://www.buttsimpleideas.com/) — Garry Mercaldi
- **DPS-1200 reverse engineering:** [Dr. Tune / Richard Aplin](https://github.com/raplin/DPS-1200FB)
- **DPS charger project:** [slundell](https://github.com/slundell/dps_charger)
