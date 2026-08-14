#include "oled.h"
#include <U8g2lib.h>
#include <Wire.h>

// Full 128x64 SSD1306 constructor — the 72x40 ER variant sends wrong
// column-offset init commands for this panel. Physical visible area is
// 72x40 starting at column 28 within the 128-column controller space.
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, 6, 5);
static bool ready = false;

// All draw calls offset by X_OFF so content lands in the visible window
static const uint8_t X_OFF = 28;
static const uint8_t Y_OFF = 0;

bool oled_init(uint8_t preferredAddr) {
    uint8_t candidates[2] = { preferredAddr,
                               (uint8_t)(preferredAddr == 0x3C ? 0x3D : 0x3C) };
    for (uint8_t a : candidates) {
        display.setI2CAddress(a << 1);
        if (display.begin()) { ready = true; return true; }
    }
    ready = false;
    return false;
}

static void ds(uint8_t x, uint8_t y, const char *s) {
    display.drawStr(X_OFF + x, Y_OFF + y, s);
}

void oled_show_ap() {
    if (!ready) return;
    display.clearBuffer();
    display.setFont(u8g2_font_5x7_tf);
    ds(0,  7, "HP-DPS Setup");
    ds(0, 17, "HP-DPS-Setup");
    ds(0, 27, "192.168.4.1");
    display.sendBuffer();
}

void oled_show_connecting(const char *ssid) {
    if (!ready) return;
    char trunc[15];
    strncpy(trunc, ssid, 14);
    trunc[14] = '\0';
    display.clearBuffer();
    display.setFont(u8g2_font_5x7_tf);
    ds(0,  7, "Connecting...");
    ds(0, 17, trunc);
    display.sendBuffer();
}

void oled_update(const DpsSensors &s, const char *ip) {
    if (!ready) return;
    char line[16];
    display.clearBuffer();
    display.setFont(u8g2_font_5x7_tf);

    snprintf(line, sizeof(line), "%.2fV  %.1fA", s.out_v, s.out_a);
    ds(0,  7, line);

    snprintf(line, sizeof(line), "%.0fW  %.1f%%", s.out_w, s.efficiency);
    ds(0, 17, line);

    snprintf(line, sizeof(line), "%.1fC %uRPM", s.temp_c, s.fan_rpm);
    ds(0, 27, line);

    ds(0, 37, ip);

    display.sendBuffer();
}
