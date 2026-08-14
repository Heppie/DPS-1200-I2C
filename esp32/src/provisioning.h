#pragma once
#include <Arduino.h>

bool provisioning_has_credentials();
void provisioning_load(String &ssid, String &pass);
void provisioning_save(const String &ssid, const String &pass);
void provisioning_clear();
void provisioning_run_ap();
bool provisioning_oled_enabled();
uint8_t provisioning_oled_addr();
