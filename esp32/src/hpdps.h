#pragma once
#include <stdint.h>

#define DPS_ADDR    0x5F
#define ONOFF_PIN   3
#define SDA_PIN     4
#define SCL_PIN     5

struct DpsSensors {
    float grid_v;
    float grid_a;
    float out_v;
    float out_a;
    float temp_f;
    uint16_t fan_rpm;
    bool valid;
};

void hpdps_init();
bool hpdps_read(uint8_t reg, uint16_t &out);
bool hpdps_write(uint8_t reg, uint16_t val);
bool hpdps_read_all(DpsSensors &out);
void hpdps_set_fan(uint8_t percent);
void hpdps_set_power(bool on);
