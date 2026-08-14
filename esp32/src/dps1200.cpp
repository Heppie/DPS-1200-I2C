#include "dps1200.h"
#include <Wire.h>
#include <Arduino.h>

void dps1200_init() {
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000);
    pinMode(ONOFF_PIN, OUTPUT);
    digitalWrite(ONOFF_PIN, LOW);
}

bool dps1200_read(uint8_t reg, uint16_t &out) {
    uint8_t cs = ((0xFF - ((DPS_ADDR << 1) + reg)) + 1) & 0xFF;

    Wire.beginTransmission(DPS_ADDR);
    Wire.write(reg);
    Wire.write(cs);
    if (Wire.endTransmission(false) != 0) return false;

    if (Wire.requestFrom((uint8_t)DPS_ADDR, (uint8_t)3) != 3) return false;

    uint8_t b0 = Wire.read();
    uint8_t b1 = Wire.read();
    Wire.read(); // response checksum, not verified

    out = ((uint16_t)b1 << 8) | b0;
    return true;
}

bool dps1200_write(uint8_t reg, uint16_t val) {
    uint8_t lsb = val & 0xFF;
    uint8_t msb = val >> 8;
    uint8_t cs  = ((0xFF - ((DPS_ADDR << 1) + reg + lsb + msb)) + 1) & 0xFF;

    Wire.beginTransmission(DPS_ADDR);
    Wire.write(reg);
    Wire.write(lsb);
    Wire.write(msb);
    Wire.write(cs);
    return Wire.endTransmission() == 0;
}

bool dps1200_read_all(DpsSensors &s) {
    uint16_t raw;
    s.valid = false;

    if (!dps1200_read(0x08, raw)) return false;
    s.grid_v = raw / 32.0f;

    if (!dps1200_read(0x0A, raw)) return false;
    s.grid_a = raw / 128.0f;

    if (!dps1200_read(0x0E, raw)) return false;
    s.out_v = raw / 256.0f;

    if (!dps1200_read(0x10, raw)) return false;
    s.out_a = raw / 128.0f;

    if (!dps1200_read(0x1C, raw)) return false;
    s.temp_f = raw / 32.0f;

    if (!dps1200_read(0x1E, raw)) return false;
    s.fan_rpm = raw;

    s.valid = true;
    return true;
}

void dps1200_set_fan(uint8_t percent) {
    if (percent > 100) percent = 100;
    uint16_t val = (uint16_t)((percent / 100.0f) * 0x1000);
    dps1200_write(0x40, val);
}

void dps1200_set_power(bool on) {
    digitalWrite(ONOFF_PIN, on ? HIGH : LOW);
}
