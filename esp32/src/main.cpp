#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "dps1200.h"
#include "provisioning.h"
#include "dashboard.h"

static DpsSensors sensors = {};
static bool       powerOn  = false;
static uint8_t    fanPct   = 0;
static uint32_t   lastPoll = 0;

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\nDPS1200 ESP32-C3 starting...");

    dps1200_init();

    if (!provisioning_has_credentials()) {
        Serial.println("No WiFi credentials, starting AP...");
        provisioning_run_ap();
        ESP.restart();
    }

    String ssid, pass;
    provisioning_load(ssid, pass);
    Serial.printf("Connecting to %s\n", ssid.c_str());

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    uint32_t t = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - t > 15000) {
            Serial.println("WiFi timeout, clearing credentials and restarting...");
            provisioning_clear();
            ESP.restart();
        }
        delay(250);
        Serial.print(".");
    }
    Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());

    if (MDNS.begin("dps1200")) {
        Serial.println("mDNS: http://dps1200.local");
    }

    webserver_init(&sensors, &powerOn, &fanPct);
    webserver_start();
    Serial.println("Dashboard ready.");
}

void loop() {
    webserver_handle();

    if (millis() - lastPoll >= 2000) {
        dps1200_read_all(sensors);
        lastPoll = millis();
    }
}
