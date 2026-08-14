#include "hpdps.h"
#include <Wire.h>
#include <Arduino.h>

void hpdps_init() {
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000);
    pinMode(ONOFF_PIN, OUTPUT);
    digitalWrite(ONOFF_PIN, LOW);
}

bool hpdps_read(uint8_t reg, uint16_t &out) {
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

bool hpdps_write(uint8_t reg, uint16_t val) {
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

bool hpdps_read_all(DpsSensors &s) {
    uint16_t raw;
    s.valid = false;

    if (!hpdps_read(0x08, raw)) return false;
    s.grid_v = raw / 32.0f;

    if (!hpdps_read(0x0A, raw)) return false;
    s.grid_a = raw / 128.0f;

    if (!hpdps_read(0x0E, raw)) return false;
    s.out_v = raw / 256.0f;

    if (!hpdps_read(0x10, raw)) return false;
    s.out_a = raw / 128.0f;

    if (!hpdps_read(0x1C, raw)) return false;
    s.temp_c = (raw / 32.0f - 32.0f) * 5.0f / 9.0f;

    if (!hpdps_read(0x1E, raw)) return false;
    s.fan_rpm = raw;

    s.in_w       = s.grid_v * s.grid_a;
    s.out_w      = s.out_v  * s.out_a;
    s.efficiency = (s.in_w > 1.0f) ? (s.out_w / s.in_w * 100.0f) : 0.0f;

    s.valid = true;
    return true;
}

bool hpdps_read_eeprom(uint8_t offset, uint8_t *buf, uint8_t len) {
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write(offset);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((uint8_t)EEPROM_ADDR, len) != len) return false;
    for (uint8_t i = 0; i < len; i++) buf[i] = Wire.read();
    return true;
}

void hpdps_read_identity(char *model, char *part_num) {
    uint8_t buf[26];

    if (hpdps_read_eeprom(0x32, buf, 26)) {
        uint8_t j = 0;
        for (uint8_t i = 0; i < 26 && j < IDENTITY_MODEL_LEN - 1; i++) {
            if (buf[i] >= 0x20 && buf[i] < 0x7F) model[j++] = (char)buf[i];
        }
        while (j > 0 && model[j - 1] == ' ') j--;
        model[j] = '\0';
    } else {
        model[0] = '\0';
    }

    if (hpdps_read_eeprom(0x4D, buf, 10)) {
        uint8_t j = 0;
        for (uint8_t i = 0; i < 10 && j < IDENTITY_PART_LEN - 1; i++) {
            if (buf[i] >= 0x20 && buf[i] < 0x7F) part_num[j++] = (char)buf[i];
        }
        while (j > 0 && part_num[j - 1] == ' ') j--;
        part_num[j] = '\0';
    } else {
        part_num[0] = '\0';
    }
}

void hpdps_set_fan(uint8_t percent) {
    if (percent > 100) percent = 100;
    uint16_t val = (uint16_t)((percent / 100.0f) * 0x1000);
    hpdps_write(0x40, val);
}

void hpdps_set_power(bool on) {
    digitalWrite(ONOFF_PIN, on ? HIGH : LOW);
}
