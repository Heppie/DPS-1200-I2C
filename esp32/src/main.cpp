#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "hpdps.h"
#include "provisioning.h"
#include "dashboard.h"
#include "oled.h"

static DpsSensors sensors = {};
static bool       powerOn  = false;
static uint8_t    fanPct   = 0;
static uint32_t   lastPoll = 0;
static char       model[IDENTITY_MODEL_LEN]  = {};
static char       part_num[IDENTITY_PART_LEN] = {};
static char       ipStr[16] = {};
static bool       oledOn = false;

static uint32_t lastBlink = 0;
static bool     ledState  = false;

static void led_blink(uint32_t interval) {
    if (millis() - lastBlink >= interval) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? LOW : HIGH);
        lastBlink = millis();
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // active-low, start off
    delay(500);
    Serial.println("\nHP DPS Control starting...");

    hpdps_init();
    hpdps_read_identity(model, part_num);
    if (model[0]) Serial.printf("PSU: %s [%s]\n", model, part_num);

    oledOn = oled_init(provisioning_oled_addr());

    if (!provisioning_has_credentials()) {
        Serial.println("No WiFi credentials, starting AP...");
        if (oledOn) oled_show_ap();
        provisioning_run_ap();
        ESP.restart();
    }

    String ssid, pass;
    provisioning_load(ssid, pass);
    Serial.printf("Connecting to %s\n", ssid.c_str());
    if (oledOn) oled_show_connecting(ssid.c_str());

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    uint32_t t = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - t > 15000) {
            Serial.println("WiFi timeout, returning to setup portal...");
            WiFi.disconnect();
            if (oledOn) oled_show_ap();
            provisioning_run_ap();
            ESP.restart();
        }
        led_blink(250);
        delay(250);
        Serial.print(".");
    }
    digitalWrite(LED_PIN, LOW); // active-low = on solid when connected
    WiFi.localIP().toString().toCharArray(ipStr, sizeof(ipStr));
    Serial.printf("\nConnected! IP: %s\n", ipStr);

    if (MDNS.begin("hp-dps-control")) {
        Serial.println("mDNS: http://hp-dps-control.local");
    }

    webserver_init(&sensors, &powerOn, &fanPct, model, part_num);
    webserver_start();
    Serial.println("Dashboard ready.");
}

void loop() {
    webserver_handle();

    if (millis() - lastPoll >= 2000) {
        hpdps_read_all(sensors);
        if (oledOn) oled_update(sensors, ipStr);
        lastPoll = millis();
    }
}
