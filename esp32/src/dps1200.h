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

void dps1200_init();
bool dps1200_read(uint8_t reg, uint16_t &out);
bool dps1200_write(uint8_t reg, uint16_t val);
bool dps1200_read_all(DpsSensors &out);
void dps1200_set_fan(uint8_t percent);
void dps1200_set_power(bool on);
