#include "provisioning.h"
#include <Preferences.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>

static const char PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>DPS1200 Setup</title>
<style>
  body{font-family:sans-serif;background:#1a1a2e;color:#eee;display:flex;
       justify-content:center;align-items:center;min-height:100vh;margin:0}
  .card{background:#16213e;padding:2rem;border-radius:12px;width:320px}
  h2{margin:0 0 1.5rem;color:#4fc3f7}
  label{display:block;margin-bottom:.3rem;font-size:.9rem;color:#aaa}
  input{width:100%;box-sizing:border-box;padding:.6rem;border-radius:6px;
        border:1px solid #333;background:#0f3460;color:#eee;font-size:1rem;margin-bottom:1rem}
  button{width:100%;padding:.75rem;border:none;border-radius:6px;
         background:#4fc3f7;color:#000;font-size:1rem;font-weight:bold;cursor:pointer}
  button:hover{background:#81d4fa}
</style>
</head>
<body>
<div class="card">
  <h2>DPS1200 WiFi Setup</h2>
  <form method="POST" action="/save">
    <label>WiFi Network (SSID)</label>
    <input type="text" name="ssid" placeholder="Your WiFi name" required>
    <label>Password</label>
    <input type="password" name="pass" placeholder="Your WiFi password">
    <button type="submit">Connect</button>
  </form>
</div>
</body>
</html>
)rawliteral";

static const char SAVED_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Saved</title>
<style>body{font-family:sans-serif;background:#1a1a2e;color:#eee;display:flex;
  justify-content:center;align-items:center;min-height:100vh;margin:0}
  .card{background:#16213e;padding:2rem;border-radius:12px;text-align:center}
  h2{color:#69f0ae}</style></head>
<body><div class="card"><h2>Saved!</h2><p>Connecting to WiFi and rebooting...</p></div></body>
</html>
)rawliteral";

bool provisioning_has_credentials() {
    Preferences prefs;
    prefs.begin("wifi", true);
    String ssid = prefs.getString("ssid", "");
    prefs.end();
    return ssid.length() > 0;
}

void provisioning_load(String &ssid, String &pass) {
    Preferences prefs;
    prefs.begin("wifi", true);
    ssid = prefs.getString("ssid", "");
    pass = prefs.getString("pass", "");
    prefs.end();
}

void provisioning_save(const String &ssid, const String &pass) {
    Preferences prefs;
    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();
}

void provisioning_clear() {
    Preferences prefs;
    prefs.begin("wifi", false);
    prefs.clear();
    prefs.end();
}

void provisioning_run_ap() {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(
        IPAddress(192, 168, 4, 1),
        IPAddress(192, 168, 4, 1),
        IPAddress(255, 255, 255, 0)
    );
    WiFi.softAP("DPS1200-Setup");
    Serial.println("AP started: DPS1200-Setup @ 192.168.4.1");

    DNSServer dns;
    dns.start(53, "*", IPAddress(192, 168, 4, 1));

    WebServer portal(80);
    bool saved = false;

    auto sendPortal = [&]() {
        portal.send_P(200, "text/html", PORTAL_HTML);
    };

    auto handleSave = [&]() {
        String ssid = portal.arg("ssid");
        String pass = portal.arg("pass");
        if (ssid.length() > 0) {
            provisioning_save(ssid, pass);
            portal.send_P(200, "text/html", SAVED_HTML);
            saved = true;
        } else {
            portal.sendHeader("Location", "/");
            portal.send(302, "text/plain", "");
        }
    };

    auto redirect = [&]() {
        portal.sendHeader("Location", "http://192.168.4.1/");
        portal.send(302, "text/plain", "");
    };

    portal.on("/", HTTP_GET, sendPortal);
    portal.on("/save", HTTP_POST, handleSave);
    // Captive portal detection endpoints
    portal.on("/generate_204", HTTP_GET, redirect);
    portal.on("/hotspot-detect.html", HTTP_GET, redirect);
    portal.on("/connecttest.txt", HTTP_GET, redirect);
    portal.on("/redirect", HTTP_GET, redirect);
    portal.onNotFound(redirect);

    portal.begin();

    while (!saved) {
        dns.processNextRequest();
        portal.handleClient();
    }

    portal.stop();
    dns.stop();
}
