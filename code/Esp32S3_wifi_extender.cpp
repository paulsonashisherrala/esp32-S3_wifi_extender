/**
 * @file ESP32_Extender.cpp
 * @brief Professional Wi-Fi Range Extender with NAPT and Real-time Dashboard
 * @author Paulsonashish Errala

 */

#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <esp_wifi.h> 

// ==========================================
//              CONFIGURATION
// ==========================================

// --- Network Settings ---
#define STA_SSID        "OptimusLogic" // replace your wifi credentials 
#define STA_PASS        "password"    // enter wifi password
#define AP_SSID         "No Internet"
#define AP_PASS         "idontknow"

// --- Hardware/System Limits ---
#define MAX_CLIENTS     12      // Max devices to track
#define JSON_BUFFER     4096    // Size for JSON serialization
#define HTTP_PORT       80
#define DNS_PORT        53

// --- IP Configuration ---
const IPAddress ap_ip(192, 168, 4, 1);
const IPAddress ap_mask(255, 255, 255, 0);
const IPAddress ap_lease_start(192, 168, 4, 2);
const IPAddress ap_dns(8, 8, 4, 4);

// ==========================================
//           FORWARD DECLARATIONS
// ==========================================
// This tells the compiler that this function exists later in the file
void onEvent(arduino_event_id_t event, arduino_event_info_t info);

// ==========================================
//           GLOBAL OBJECTS
// ==========================================

DNSServer dnsServer;
AsyncWebServer server(HTTP_PORT);

// Structure for tracking connected client uptime
struct ConnectedDevice {
    String mac;
    unsigned long connectStart;
    bool active;
};

ConnectedDevice devices[MAX_CLIENTS];
unsigned long wifiConnectTime = 0; // Timestamp when Extender gets Internet

// ==========================================
//           WEB DASHBOARD (HTML)
// ==========================================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Extender Pro</title>
  <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;600;700&display=swap" rel="stylesheet">
  <style>
    :root { --primary: #3b82f6; --bg: #0f172a; --card-bg: rgba(30, 41, 59, 0.7); --text: #f8fafc; --text-dim: #94a3b8; }
    * { margin:0; padding:0; box-sizing:border-box; }
    body { font-family: 'Inter', sans-serif; background: var(--bg); color: var(--text); min-height: 100vh; background-image: radial-gradient(circle at top right, #1e3a8a 0%, transparent 40%), radial-gradient(circle at bottom left, #0f172a 0%, transparent 40%); }
    .header { background: rgba(15, 23, 42, 0.8); backdrop-filter: blur(12px); padding: 1rem 1.5rem; position: sticky; top: 0; z-index: 100; border-bottom: 1px solid rgba(255,255,255,0.1); display: flex; align-items: center; justify-content: space-between; }
    .brand { display: flex; align-items: center; gap: 10px; font-weight: 700; font-size: 1.2rem; letter-spacing: -0.5px; }
    .status-dot { width: 10px; height: 10px; border-radius: 50%; background: #10b981; box-shadow: 0 0 10px #10b981; animation: pulse 2s infinite; }
    .container { max-width: 1200px; margin: 2rem auto; padding: 0 1.5rem; }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(350px, 1fr)); gap: 1.5rem; margin-bottom: 1.5rem; }
    .card { background: var(--card-bg); border-radius: 20px; padding: 1.5rem; border: 1px solid rgba(255,255,255,0.05); backdrop-filter: blur(10px); box-shadow: 0 10px 30px -10px rgba(0,0,0,0.5); }
    .card-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 1.5rem; }
    .card-title { font-size: 1.1rem; font-weight: 600; color: var(--text); display: flex; align-items: center; gap: 8px; }
    .metric-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 1rem; }
    .metric-item { background: rgba(255,255,255,0.03); padding: 1rem; border-radius: 12px; display: flex; flex-direction: column; align-items: center; justify-content: center; }
    .metric-val { font-size: 1.5rem; font-weight: 700; margin: 5px 0; }
    .metric-label { font-size: 0.8rem; color: var(--text-dim); text-transform: uppercase; letter-spacing: 0.5px; }
    .text-green { color: #34d399; } .text-red { color: #f87171; }
    .btn-ookla { background: linear-gradient(135deg, #00b0f0, #005eb8); color: white; border: none; padding: 1rem 2rem; border-radius: 50px; font-size: 1.1rem; font-weight: 700; cursor: pointer; width: 100%; text-decoration: none; display: inline-block; margin-bottom: 1rem; text-align: center; }
    .device-list { max-height: 400px; overflow-y: auto; }
    .device-item { display: flex; align-items: center; justify-content: space-between; padding: 1rem; border-bottom: 1px solid rgba(255,255,255,0.05); transition: background 0.3s; }
    .device-item:hover { background: rgba(255,255,255,0.05); }
    .device-name { font-weight: 600; font-size: 1rem; color: #fff; margin-bottom: 4px; }
    .device-meta { font-size: 0.8rem; color: var(--text-dim); font-family: monospace; }
    .uptime-badge { background: rgba(16, 185, 129, 0.2); color: #34d399; padding: 4px 10px; border-radius: 12px; font-size: 0.75rem; font-weight: 600; display: inline-block; margin-top: 5px; }
    .bars { display: flex; gap: 3px; align-items: flex-end; height: 20px; }
    .bar { width: 4px; background: #334155; border-radius: 2px; }
    .bar.fill { background: #34d399; }
    @keyframes pulse { 0% {opacity:1} 50% {opacity:0.5} 100% {opacity:1} }
    ::-webkit-scrollbar { width: 6px; }
    ::-webkit-scrollbar-thumb { background: rgba(255,255,255,0.1); border-radius: 3px; }
  </style>
</head>
<body>
  <div class="header">
    <div class="brand"><span class="status-dot"></span> ESP32-S3 Wifi Extender</div>
    <div style="font-size:0.8rem; color:var(--text-dim)">Connected</div>
  </div>
  <div class="container">
    <div class="grid">
      <div class="card">
        <div class="card-header"><div class="card-title">📊 System Vitality</div></div>
        <div class="metric-grid">
          <div class="metric-item">
            <div class="metric-label">Internet</div>
            <div class="metric-val" id="netStatus">--</div>
          </div>
          <div class="metric-item">
            <div class="metric-label">Signal</div>
            <div class="bars" id="signalBars">
                <div class="bar"></div><div class="bar"></div><div class="bar"></div><div class="bar"></div>
            </div>
            <div style="font-size:0.8rem; color:#94a3b8" id="rssiVal">-- dBm</div>
          </div>
          <div class="metric-item">
            <div class="metric-label">Clients</div>
            <div class="metric-val" id="clientCount">0</div>
          </div>
          <div class="metric-item">
            <div class="metric-label">System Uptime</div>
            <div class="metric-val" id="uptime" style="font-size:1rem; color:#34d399">00:00:00</div>
          </div>
        </div>
      </div>
      <div class="card">
        <div class="card-header"><div class="card-title">🚀 Speed Test</div></div>
        <div style="text-align:center">
            <a href="https://www.speedtest.net/" target="_blank" class="btn-ookla">GO (Ookla)</a>
            <div style="margin-top:1rem; border-top:1px solid rgba(255,255,255,0.1); padding-top:1rem;">
                <p style="font-size:0.8rem; color:#64748b; margin-bottom:0.5rem">Quick Inline Test</p>
                <iframe src="https://openspeedtest.com/Get-widget.php" width="100%" height="200px" frameborder="0" style="border-radius:12px; background:#fff"></iframe>
            </div>
        </div>
      </div>
    </div>
    <div class="grid" style="grid-template-columns: 1fr;">
      <div class="card">
        <div class="card-header">
          <div class="card-title">📱 Connected Devices</div>
          <div style="font-size:0.8rem; color:#94a3b8">Real-time Updates</div>
        </div>
        <div class="device-list" id="deviceList">
          <div style="text-align:center; padding:2rem; color:var(--text-dim)">Scanning Network...</div>
        </div>
      </div>
    </div>
  </div>
  <script>
    function fmtTime(seconds) {
      if(seconds < 0) return "Connecting...";
      const h = Math.floor(seconds/3600).toString().padStart(2,'0');
      const m = Math.floor((seconds%3600)/60).toString().padStart(2,'0');
      const s = Math.floor(seconds%60).toString().padStart(2,'0');
      return `${h}:${m}:${s}`;
    }
    function updateSignal(rssi) {
        const bars = document.querySelectorAll('.bar');
        const strength = rssi >= -50 ? 4 : rssi >= -60 ? 3 : rssi >= -70 ? 2 : 1;
        bars.forEach((bar, idx) => {
            bar.className = idx < strength ? 'bar fill' : 'bar';
            bar.style.height = (40 + (idx * 20)) + '%';
        });
        document.getElementById('rssiVal').innerText = rssi + " dBm";
    }
    async function fetchData() {
      try {
        const res = await fetch('/status.json');
        const data = await res.json();
        const netEl = document.getElementById('netStatus');
        netEl.textContent = data.online ? 'Online' : 'Offline';
        netEl.className = data.online ? 'metric-val text-green' : 'metric-val text-red';
        updateSignal(data.sys_rssi);
        document.getElementById('clientCount').textContent = data.devices.length;
        document.getElementById('uptime').textContent = fmtTime(data.uptime);
        const list = document.getElementById('deviceList');
        if(data.devices.length === 0) {
            list.innerHTML = '<div style="text-align:center; padding:2rem; color:var(--text-dim)">No devices connected</div>';
        } else {
            list.innerHTML = data.devices.map((c, i) => `
                <div class="device-item">
                    <div>
                        <div class="device-name">Device-${c.mac.substring(9,11)}${c.mac.substring(12,14)}${c.mac.substring(15,17)}</div>
                        <div class="device-meta">IP: 192.168.4.${2+i} &bull; MAC: ${c.mac}</div>
                        <div class="uptime-badge">⏱ Connected: ${fmtTime(c.uptime)}</div>
                    </div>
                    <div style="text-align:right">
                        <div style="font-weight:700; color:#fff">${c.rssi} dBm</div>
                        <div style="font-size:0.75rem; color:#94a3b8">Signal</div>
                    </div>
                </div>
            `).join('');
        }
      } catch(e) { console.log(e); }
      setTimeout(fetchData, 1000);
    }
    fetchData();
  </script>
</body>
</html>
)rawliteral";

// ==========================================
//             SETUP & LOOP
// ==========================================

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    
    // Init device struct
    for(int i = 0; i < MAX_CLIENTS; i++) {
        devices[i].mac = "";
        devices[i].active = false;
    }

    // Attach Event Handler
    WiFi.onEvent(onEvent); // Using WiFi.onEvent for broader compatibility

    // 1. Initialize AP Mode (Extender Output)
    WiFi.AP.begin();
    WiFi.AP.config(ap_ip, ap_ip, ap_mask, ap_lease_start, ap_dns);
    WiFi.AP.create(AP_SSID, AP_PASS, 1); // Channel 1

    if (!WiFi.AP.waitStatusBits(ESP_NETIF_STARTED_BIT, 1000)) {
        Serial.println("Error: Failed to start AP!");
        return;
    }

    // 2. Initialize STA Mode (Input from Router)
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.begin(STA_SSID, STA_PASS);

    // 3. Start DNS Server (Captive Portal)
    dnsServer.start(DNS_PORT, "*", ap_ip);

    // 4. Configure Web Routes
    
    // Route: Main Dashboard
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("text/html");
        response->setCode(200);
        response->addHeader("Cache-Control", "no-cache");
        response->print(INDEX_HTML);
        request->send(response);
    });

    // Route: JSON API (Real-time data)
    server.on("/status.json", HTTP_GET, [](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(JSON_BUFFER);
        
        bool isOnline = WiFi.isConnected();
        doc["online"] = isOnline;
        doc["sys_rssi"] = WiFi.RSSI();
        doc["uptime"] = (isOnline && wifiConnectTime > 0) ? (millis() - wifiConnectTime) / 1000 : 0;

        // Get Low-Level Client List
        wifi_sta_list_t wifi_sta_list;
        esp_wifi_ap_get_sta_list(&wifi_sta_list);

        JsonArray arr = doc.createNestedArray("devices");
        unsigned long currentMillis = millis();

        for(int i = 0; i < wifi_sta_list.num; i++) {
            wifi_sta_info_t station = wifi_sta_list.sta[i];
            
            // Optimized MAC to String conversion
            char macCStr[18];
            snprintf(macCStr, sizeof(macCStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                station.mac[0], station.mac[1], station.mac[2],
                station.mac[3], station.mac[4], station.mac[5]);
            String sMac = String(macCStr);

            // Device Tracking Logic
            int trackedIdx = -1;
            
            // Find existing
            for(int k = 0; k < MAX_CLIENTS; k++) {
                if(devices[k].active && devices[k].mac == sMac) {
                    trackedIdx = k;
                    break;
                }
            }
            
            // Or Add new
            if(trackedIdx == -1) {
                for(int k = 0; k < MAX_CLIENTS; k++) {
                    if(!devices[k].active) {
                        devices[k].mac = sMac;
                        devices[k].connectStart = currentMillis;
                        devices[k].active = true;
                        trackedIdx = k;
                        break;
                    }
                }
            }

            // Populate JSON
            if(trackedIdx != -1) {
                JsonObject obj = arr.createNestedObject();
                obj["mac"] = sMac;
                obj["rssi"] = station.rssi;
                obj["uptime"] = (currentMillis - devices[trackedIdx].connectStart) / 1000;
            }
        }

        // Cleanup stale devices
        for(int k = 0; k < MAX_CLIENTS; k++) {
            if(devices[k].active) {
                bool stillConnected = false;
                for(int i = 0; i < wifi_sta_list.num; i++) {
                     wifi_sta_info_t s = wifi_sta_list.sta[i];
                     char m[18];
                     snprintf(m, sizeof(m), "%02X:%02X:%02X:%02X:%02X:%02X", 
                        s.mac[0], s.mac[1], s.mac[2], s.mac[3], s.mac[4], s.mac[5]);
                     if(devices[k].mac == String(m)) {
                         stillConnected = true;
                         break;
                     }
                }
                if(!stillConnected) devices[k].active = false;
            }
        }

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // Captive Portal Routes
    server.on("/continue", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("http://www.google.com");
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->redirect("http://192.168.4.1/");
    });

    server.begin();
    Serial.println("System Started: OptimusLogic Extender Pro");
}

void loop() {
    dnsServer.processNextRequest();
    delay(5); // Small yield for stability
}

// ==========================================
//           EVENT HANDLERS
// ==========================================

void onEvent(arduino_event_id_t event, arduino_event_info_t info) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.println("[Wi-Fi] Internet Connected via Router");
            wifiConnectTime = millis();
            WiFi.AP.enableNAPT(true);
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("[Wi-Fi] Internet Lost - Retrying...");
            wifiConnectTime = 0;
            WiFi.AP.enableNAPT(false);
            break;
        default: break;
    }
}
