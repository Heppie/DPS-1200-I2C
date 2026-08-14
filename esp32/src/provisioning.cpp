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
<title>HP DPS Control Setup</title>
<style>
  *{box-sizing:border-box;margin:0;padding:0}
  body{font-family:sans-serif;background:#1a1a2e;color:#eee;display:flex;
       justify-content:center;align-items:flex-start;min-height:100vh;padding:1.5rem}
  .card{background:#16213e;padding:2rem;border-radius:12px;width:100%;max-width:340px}
  h2{margin:0 0 1.5rem;color:#4fc3f7}
  h3{font-size:.9rem;color:#78909c;margin:1.2rem 0 .6rem;text-transform:uppercase;letter-spacing:.05em}
  label{display:block;margin:.8rem 0 .3rem;font-size:.9rem;color:#aaa}
  input[type=text],input[type=password],select{width:100%;padding:.6rem;border-radius:6px;
    border:1px solid #333;background:#0f3460;color:#eee;font-size:1rem}
  .row{display:flex;align-items:center;gap:.6rem;margin-top:.8rem}
  .row input[type=checkbox]{width:1.1rem;height:1.1rem;accent-color:#4fc3f7;flex-shrink:0}
  .row span{font-size:.9rem;color:#aaa}
  .btn{width:100%;padding:.75rem;border:none;border-radius:6px;font-size:1rem;
       font-weight:bold;cursor:pointer;margin-top:.75rem;display:block}
  .btn-scan{background:#0f3460;color:#4fc3f7;border:1px solid #4fc3f7}
  .btn-scan:hover{background:#163a5e}
  .btn-connect{background:#4fc3f7;color:#000}
  .btn-connect:hover{background:#81d4fa}
  hr{border:none;border-top:1px solid #2a3a5e;margin:1.2rem 0}
  #netlist{margin-top:.75rem;display:none;max-height:200px;overflow-y:auto;border-radius:6px}
  .net{display:flex;justify-content:space-between;align-items:center;padding:.5rem .7rem;
       cursor:pointer;background:#0f3460;border-bottom:1px solid #1a3a60}
  .net:first-child{border-radius:6px 6px 0 0}
  .net:last-child{border-bottom:none;border-radius:0 0 6px 6px}
  .net:hover,.net.sel{background:#1a4a80}
  .net-name{font-size:.9rem}
  .sig{font-size:.75rem;color:#78909c;letter-spacing:-.05em}
  .msg{padding:.5rem;text-align:center;font-size:.85rem;color:#78909c}
  #oled_opts{margin-top:.6rem}
</style>
</head>
<body>
<div class="card">
  <h2>HP DPS Control Setup</h2>
  <button class="btn btn-scan" onclick="scan(this)">Scan for Networks</button>
  <div id="netlist"></div>
  <form method="POST" action="/save">
    <label>Network (SSID)</label>
    <input type="text" id="ssid" name="ssid" placeholder="Network name" required>
    <label>Password</label>
    <input type="password" name="pass" placeholder="Leave blank if open">
    <hr>
    <h3>Display</h3>
    <div class="row">
      <input type="checkbox" id="oled_chk" name="oled_en" value="1" onchange="tog()" checked>
      <span>Enable 0.42&quot; OLED (SSD1306)</span>
    </div>
    <div id="oled_opts">
      <label>I2C Address</label>
      <select name="oled_addr">
        <option value="60">0x3C (default)</option>
        <option value="61">0x3D</option>
      </select>
    </div>
    <button type="submit" class="btn btn-connect">Save &amp; Connect</button>
  </form>
</div>
<script>
function bars(r){return r>-50?'▂▄▆█':r>-60?'▂▄▆·':r>-70?'▂▄··':'▂···';}
function tog(){document.getElementById('oled_opts').style.display=
  document.getElementById('oled_chk').checked?'block':'none';}
function scan(btn){
  var list=document.getElementById('netlist');
  list.style.display='block';
  list.innerHTML='<div class="msg">Scanning...</div>';
  btn.disabled=true;
  fetch('/scan').then(function(r){return r.json();}).then(function(nets){
    btn.disabled=false;
    if(!nets.length){list.innerHTML='<div class="msg">No networks found</div>';return;}
    list.innerHTML='';
    nets.forEach(function(n){
      var el=document.createElement('div');
      el.className='net';
      el.innerHTML='<span class="net-name">'+n.ssid+'</span>'+
                   '<span class="sig">'+bars(n.rssi)+(n.secure?' &#128274;':'')+'</span>';
      el.onclick=function(){
        document.querySelectorAll('.net').forEach(function(e){e.classList.remove('sel');});
        el.classList.add('sel');
        document.getElementById('ssid').value=n.ssid;
      };
      list.appendChild(el);
    });
  }).catch(function(){btn.disabled=false;list.innerHTML='<div class="msg">Scan failed</div>';});
}
</script>
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

bool provisioning_oled_enabled() {
    Preferences prefs;
    prefs.begin("disp", true);
    bool en = prefs.getBool("en", true);
    prefs.end();
    return en;
}

uint8_t provisioning_oled_addr() {
    Preferences prefs;
    prefs.begin("disp", true);
    uint8_t addr = prefs.getUChar("addr", 0x3C);
    prefs.end();
    return addr;
}

void provisioning_run_ap() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(
        IPAddress(192, 168, 4, 1),
        IPAddress(192, 168, 4, 1),
        IPAddress(255, 255, 255, 0)
    );
    WiFi.softAP("HP-DPS-Setup");
    Serial.println("AP started: HP-DPS-Setup @ 192.168.4.1");

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
            // Save display settings
            Preferences disp;
            disp.begin("disp", false);
            disp.putBool("en", portal.hasArg("oled_en"));
            disp.putUChar("addr", (uint8_t)portal.arg("oled_addr").toInt());
            disp.end();
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

    auto handleScan = [&]() {
        int n = WiFi.scanNetworks();
        String json = "[";
        for (int i = 0; i < n; i++) {
            String ssid = WiFi.SSID(i);
            // basic JSON escaping for SSID
            ssid.replace("\\", "\\\\");
            ssid.replace("\"", "\\\"");
            if (i > 0) json += ",";
            json += "{\"ssid\":\"" + ssid + "\",\"rssi\":" + String(WiFi.RSSI(i)) +
                    ",\"secure\":" + (WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false") + "}";
        }
        json += "]";
        WiFi.scanDelete();
        portal.send(200, "application/json", json);
    };

    portal.on("/", HTTP_GET, sendPortal);
    portal.on("/scan", HTTP_GET, handleScan);
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
