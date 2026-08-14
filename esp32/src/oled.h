#pragma once
#include "hpdps.h"

void oled_init(uint8_t addr);
void oled_show_ap();
void oled_show_connecting(const char *ssid);
void oled_update(const DpsSensors &s, const char *ip);
