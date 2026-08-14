#include "dashboard.h"
#include "provisioning.h"
#include <WebServer.h>
#include <Arduino.h>

static WebServer server(80);
static DpsSensors *g_sensors;
static bool      *g_powerOn;
static uint8_t   *g_fanPct;
static char       g_model[27];
static char       g_part_num[11];

static const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>HP DPS Control</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:sans-serif;background:#1a1a2e;color:#eee;min-height:100vh;padding:1.5rem}
h1{color:#4fc3f7;margin-bottom:.3rem;font-size:1.4rem;text-align:center}
.identity{text-align:center;font-size:.8rem;color:#78909c;margin-bottom:1.2rem;min-height:1rem}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(160px,1fr));gap:1rem;margin-bottom:1.5rem}
.card{background:#16213e;border-radius:10px;padding:1.2rem;text-align:center}
.card .label{font-size:.78rem;color:#90caf9;text-transform:uppercase;letter-spacing:.05em;margin-bottom:.4rem}
.card .value{font-size:1.8rem;font-weight:bold;color:#e0f7fa}
.card .unit{font-size:.85rem;color:#78909c;margin-top:.2rem}
.controls{background:#16213e;border-radius:10px;padding:1.2rem;display:flex;gap:1rem;align-items:center;flex-wrap:wrap}
.controls label{color:#90caf9;font-size:.85rem}
#powerBtn{padding:.6rem 1.4rem;border:none;border-radius:6px;font-size:1rem;font-weight:bold;cursor:pointer;transition:background .2s}
#powerBtn.on{background:#ef5350;color:#fff}
#powerBtn.off{background:#69f0ae;color:#000}
.fan-wrap{display:flex;align-items:center;gap:.8rem;flex:1;min-width:200px}
#fanSlider{flex:1;accent-color:#4fc3f7}
#fanVal{min-width:3rem;color:#e0f7fa;font-weight:bold}
.status{text-align:center;font-size:.75rem;color:#546e7a;margin-top:1rem}
a.reset{display:block;text-align:center;margin-top:.8rem;color:#ef5350;font-size:.8rem}
</style>
</head>
<body>
<h1>HP DPS Control</h1>
<div class="identity" id="identity"></div>
<div class="grid">
  <div class="card"><div class="label">Input Voltage</div><div class="value" id="grid_v">--</div><div class="unit">V AC</div></div>
  <div class="card"><div class="label">Input Current</div><div class="value" id="grid_a">--</div><div class="unit">A</div></div>
  <div class="card"><div class="label">Input Power</div><div class="value" id="in_w">--</div><div class="unit">W</div></div>
  <div class="card"><div class="label">Output Voltage</div><div class="value" id="out_v">--</div><div class="unit">V DC</div></div>
  <div class="card"><div class="label">Output Current</div><div class="value" id="out_a">--</div><div class="unit">A</div></div>
  <div class="card"><div class="label">Output Power</div><div class="value" id="out_w">--</div><div class="unit">W</div></div>
  <div class="card"><div class="label">Efficiency</div><div class="value" id="efficiency">--</div><div class="unit">%</div></div>
  <div class="card"><div class="label">Temperature</div><div class="value" id="temp_f">--</div><div class="unit">&deg;F</div></div>
  <div class="card"><div class="label">Fan Speed</div><div class="value" id="fan_rpm">--</div><div class="unit">RPM</div></div>
</div>
<div class="controls">
  <button id="powerBtn" class="off">Turn ON</button>
  <div class="fan-wrap">
    <label>Fan</label>
    <input type="range" id="fanSlider" min="0" max="100" value="0">
    <span id="fanVal">0%</span>
  </div>
</div>
<div class="status" id="status">Connecting...</div>
<a class="reset" href="/reset" onclick="return confirm('Reset WiFi settings?')">Reset WiFi</a>
<script>
var powerOn = false;
var fanDebounce = null;

function update(d) {
  document.getElementById('grid_v').textContent    = d.grid_v.toFixed(1);
  document.getElementById('grid_a').textContent    = d.grid_a.toFixed(2);
  document.getElementById('in_w').textContent      = d.in_w.toFixed(0);
  document.getElementById('out_v').textContent     = d.out_v.toFixed(2);
  document.getElementById('out_a').textContent     = d.out_a.toFixed(1);
  document.getElementById('out_w').textContent     = d.out_w.toFixed(0);
  document.getElementById('efficiency').textContent = d.efficiency.toFixed(1);
  document.getElementById('temp_f').textContent    = d.temp_f.toFixed(1);
  document.getElementById('fan_rpm').textContent   = d.fan_rpm;
  if (d.model) {
    var id = d.model + (d.part ? ' — ' + d.part : '');
    document.getElementById('identity').textContent = id;
  }
  powerOn = d.power_on;
  var btn = document.getElementById('powerBtn');
  if (powerOn) { btn.textContent='Turn OFF'; btn.className='on'; }
  else          { btn.textContent='Turn ON';  btn.className='off'; }
  if (document.activeElement !== document.getElementById('fanSlider')) {
    document.getElementById('fanSlider').value = d.fan_pct;
    document.getElementById('fanVal').textContent = d.fan_pct + '%';
  }
  document.getElementById('status').textContent = 'Updated ' + new Date().toLocaleTimeString();
}

function poll() {
  fetch('/data').then(function(r){ return r.json(); }).then(update)
    .catch(function(){ document.getElementById('status').textContent = 'No data'; });
}

document.getElementById('powerBtn').addEventListener('click', function() {
  fetch('/power', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body: 'state=' + (powerOn ? '0' : '1')}).then(poll);
});

document.getElementById('fanSlider').addEventListener('input', function() {
  var v = this.value;
  document.getElementById('fanVal').textContent = v + '%';
  clearTimeout(fanDebounce);
  fanDebounce = setTimeout(function() {
    fetch('/fan', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'},
      body: 'speed=' + v});
  }, 500);
});

poll();
setInterval(poll, 2000);
</script>
</body>
</html>
)rawliteral";

void webserver_init(DpsSensors *sensors, bool *powerOn, uint8_t *fanPct,
                    const char *model, const char *part_num) {
    g_sensors = sensors;
    g_powerOn = powerOn;
    g_fanPct  = fanPct;
    strncpy(g_model,    model,    sizeof(g_model)    - 1); g_model[sizeof(g_model) - 1]       = '\0';
    strncpy(g_part_num, part_num, sizeof(g_part_num) - 1); g_part_num[sizeof(g_part_num) - 1] = '\0';

    server.on("/", HTTP_GET, []() {
        server.send_P(200, "text/html", DASHBOARD_HTML);
    });

    server.on("/data", HTTP_GET, []() {
        char buf[350];
        snprintf(buf, sizeof(buf),
            "{\"grid_v\":%.1f,\"grid_a\":%.2f,\"in_w\":%.0f,"
            "\"out_v\":%.2f,\"out_a\":%.1f,\"out_w\":%.0f,\"efficiency\":%.1f,"
            "\"temp_f\":%.1f,\"fan_rpm\":%u,\"power_on\":%s,\"fan_pct\":%u,"
            "\"model\":\"%s\",\"part\":\"%s\"}",
            g_sensors->grid_v, g_sensors->grid_a, g_sensors->in_w,
            g_sensors->out_v,  g_sensors->out_a,  g_sensors->out_w,
            g_sensors->efficiency,
            g_sensors->temp_f, g_sensors->fan_rpm,
            *g_powerOn ? "true" : "false",
            *g_fanPct,
            g_model, g_part_num);
        server.send(200, "application/json", buf);
    });

    server.on("/power", HTTP_POST, []() {
        if (server.hasArg("state")) {
            bool on = server.arg("state") == "1";
            *g_powerOn = on;
            hpdps_set_power(on);
        }
        server.send(200, "text/plain", "ok");
    });

    server.on("/fan", HTTP_POST, []() {
        if (server.hasArg("speed")) {
            uint8_t pct = (uint8_t)constrain(server.arg("speed").toInt(), 0, 100);
            *g_fanPct = pct;
            hpdps_set_fan(pct);
        }
        server.send(200, "text/plain", "ok");
    });

    server.on("/reset", HTTP_GET, []() {
        server.send(200, "text/plain", "Resetting WiFi credentials, rebooting...");
        delay(500);
        provisioning_clear();
        ESP.restart();
    });
}

void webserver_start() {
    server.begin();
}

void webserver_handle() {
    server.handleClient();
}
