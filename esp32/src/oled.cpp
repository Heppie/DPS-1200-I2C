#include "oled.h"
#include <U8g2lib.h>
#include <Wire.h>

static U8G2_SSD1306_72X40_ER_F_HW_I2C display(U8G2_R0);
static bool ready = false;

void oled_init(uint8_t addr) {
    display.setI2CAddress(addr << 1);
    ready = display.begin();
    if (ready) {
        display.clearDisplay();
    }
}

void oled_show_ap() {
    if (!ready) return;
    display.clearBuffer();
    display.setFont(u8g2_font_5x7_tf);
    display.drawStr(0,  7, "HP-DPS Setup");
    display.drawStr(0, 17, "HP-DPS-Setup");
    display.drawStr(0, 27, "192.168.4.1");
    display.sendBuffer();
}

void oled_show_connecting(const char *ssid) {
    if (!ready) return;
    char trunc[15];
    strncpy(trunc, ssid, 14);
    trunc[14] = '\0';
    display.clearBuffer();
    display.setFont(u8g2_font_5x7_tf);
    display.drawStr(0,  7, "Connecting...");
    display.drawStr(0, 17, trunc);
    display.sendBuffer();
}

void oled_update(const DpsSensors &s, const char *ip) {
    if (!ready) return;
    char line[16];
    display.clearBuffer();
    display.setFont(u8g2_font_5x7_tf);

    snprintf(line, sizeof(line), "%.2fV  %.1fA", s.out_v, s.out_a);
    display.drawStr(0, 7, line);

    snprintf(line, sizeof(line), "%.0fW  %.1f%%", s.out_w, s.efficiency);
    display.drawStr(0, 17, line);

    snprintf(line, sizeof(line), "%.1fC %uRPM", s.temp_c, s.fan_rpm);
    display.drawStr(0, 27, line);

    display.drawStr(0, 37, ip);

    display.sendBuffer();
}
