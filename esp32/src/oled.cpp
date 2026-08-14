#include "oled.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Physical panel is 72x40 but sits at column offset 28 within the
// 128x64 SSD1306 controller space. Init as 128x64 and apply X_OFF.
#define X_OFF 28
#define Y_OFF 16

static Adafruit_SSD1306 display(128, 64, &Wire, -1);
static bool ready = false;

bool oled_init(uint8_t preferredAddr) {
    uint8_t candidates[2] = { preferredAddr,
                               (uint8_t)(preferredAddr == 0x3C ? 0x3D : 0x3C) };
    for (uint8_t a : candidates) {
        if (display.begin(SSD1306_SWITCHCAPVCC, a)) {
            ready = true;
            display.clearDisplay();
            display.display();
            return true;
        }
    }
    return false;
}

void oled_show_ap() {
    if (!ready) return;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(X_OFF, Y_OFF +  0); display.print("HP-DPS Setup");
    display.setCursor(X_OFF, Y_OFF + 10); display.print("HP-DPS-Setup");
    display.setCursor(X_OFF, Y_OFF + 20); display.print("192.168.4.1");
    display.display();
}

void oled_show_connecting(const char *ssid) {
    if (!ready) return;
    char trunc[15];
    strncpy(trunc, ssid, 14);
    trunc[14] = '\0';
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(X_OFF, Y_OFF +  0); display.print("Connecting...");
    display.setCursor(X_OFF, Y_OFF + 10); display.print(trunc);
    display.display();
}

void oled_update(const DpsSensors &s, const char *ip) {
    if (!ready) return;
    char line[16];
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    snprintf(line, sizeof(line), "%.2fV  %.1fA", s.out_v, s.out_a);
    display.setCursor(X_OFF, Y_OFF +  0); display.print(line);

    snprintf(line, sizeof(line), "%.0fW  %.1f%%", s.out_w, s.efficiency);
    display.setCursor(X_OFF, Y_OFF + 10); display.print(line);

    snprintf(line, sizeof(line), "%.1fC %uRPM", s.temp_c, s.fan_rpm);
    display.setCursor(X_OFF, Y_OFF + 20); display.print(line);

    display.setCursor(X_OFF, Y_OFF + 30); display.print(ip);

    display.display();
}
