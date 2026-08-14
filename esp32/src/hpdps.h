#pragma once
#include <stdint.h>

#define DPS_ADDR    0x5F
#define EEPROM_ADDR 0x57
#define ONOFF_PIN   3
#define SDA_PIN     5
#define SCL_PIN     6
#define LED_PIN     8

#define IDENTITY_MODEL_LEN  27  // 26 EEPROM bytes + null
#define IDENTITY_PART_LEN   11  // 10 EEPROM bytes + null

struct DpsSensors {
    float grid_v;
    float grid_a;
    float in_w;
    float out_v;
    float out_a;
    float out_w;
    float efficiency;
    float temp_c;
    uint16_t fan_rpm;
    bool valid;
};

void hpdps_init();
bool hpdps_read(uint8_t reg, uint16_t &out);
bool hpdps_write(uint8_t reg, uint16_t val);
bool hpdps_read_all(DpsSensors &out);
void hpdps_set_fan(uint8_t percent);
void hpdps_set_power(bool on);
bool hpdps_read_eeprom(uint8_t offset, uint8_t *buf, uint8_t len);
void hpdps_read_identity(char *model, char *part_num);
