#include "wifi_app.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ElegantOTA.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include "display.h"

static WebServer     *s_server = nullptr;
static DNSServer      s_dns_server;
static Preferences    s_prefs;

enum WifiState {
    WIFI_STATE_IDLE,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_AP_MODE
};

static WifiState      s_wifi_state = WIFI_STATE_IDLE;
static uint32_t       s_connect_start_ts = 0;
static bool           s_wifi_connected = false;
static WeatherData    s_cached_weather = { 28.5f, 65, "Partly Cloudy", true };
static TaskHandle_t   s_weather_task_handle = nullptr;
static volatile bool  s_weather_task_exit   = false;

// Persistent Config
static char           s_config_ssid[64] = "";
static char           s_config_pwd[64] = "";
static float          s_config_lat = 21.0285f;
static float          s_config_lon = 105.8542f;
static uint8_t        s_config_brightness = 100;

// Scan State
static bool           s_scan_in_progress = false;
static bool           s_scan_complete = false;
static int            s_scan_count = 0;

// Embedded HTML Web Setup Portal from Clock branch (without sleep mode)
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="vi"><head><meta charset="UTF-8"/><meta name="viewport" content="width=device-width,initial-scale=1.0"/><title>ESP32 Setup</title>
<style>
:root{--p:#007bff;--bg:#f4f4f9;--txt:#333}body{font-family:-apple-system,sans-serif;background:var(--bg);color:var(--txt);display:flex;justify-content:center;padding:15px;margin:0}
.card{background:#fff;padding:20px;border-radius:12px;box-shadow:0 4px 16px rgba(0,0,0,0.1);width:100%;max-width:400px}
h2{margin-top:0;color:var(--p);text-align:center;font-size:1.4rem}.item{margin-bottom:12px}
label{display:block;margin-bottom:4px;font-weight:bold;font-size:.85rem}
input,select{width:100%;padding:10px;border:1px solid #ddd;border-radius:6px;box-sizing:border-box;font-size:.95rem}
button{width:100%;padding:12px;background:var(--p);color:#fff;border:none;border-radius:6px;font-size:1rem;cursor:pointer;font-weight:bold}
button:disabled{background:#ccc;cursor:not-allowed}button.secondary{background:#6c757d}
#msg{margin-top:12px;padding:10px;border-radius:6px;display:none;font-size:.9rem;text-align:center}
.success{background:#d4edda;color:#155724}.error{background:#f8d7da;color:#721c24}.info{background:#d1ecf1;color:#0c5460}
.wifi-list{max-height:160px;overflow-y:auto;border:1px solid #ddd;border-radius:6px;margin-bottom:10px}
.wifi-item{padding:10px;border-bottom:1px solid #eee;cursor:pointer;display:flex;justify-content:space-between;align-items:center}
.wifi-item.selected{background:#e7f3ff;border-left:4px solid var(--p)}
.spinner{border:2px solid #f3f3f3;border-top:2px solid var(--p);border-radius:50%;width:14px;height:14px;animation:s 1s linear infinite;display:inline-block;margin-right:6px;vertical-align:middle}
@keyframes s{0%{transform:rotate(0)}100%{transform:rotate(360deg)}}.hidden{display:none}
</style></head>
<body>
  <div class="card">
    <h2>Cấu Hình Hệ Thống</h2>

    <div class="item">
      <button id="scanBtn" class="secondary" onclick="scanWiFi()">
        <span id="scanText">🔍 Quét WiFi</span>
      </button>
    </div>

    <div id="wifiListContainer" class="hidden">
      <div class="item">
        <label>Chọn mạng WiFi</label>
        <div class="wifi-list" id="wifiList"></div>
      </div>
    </div>

    <div class="item">
      <label>Mật khẩu WiFi</label>
      <input type="password" id="pw" placeholder="Chọn WiFi trước hoặc nhập mật khẩu" disabled />
    </div>

    <div class="item">
      <label>Thành phố để lấy thời tiết</label>
      <select id="city" onchange="updateCoords()"></select>
    </div>

    <div style="display: flex; gap: 10px; margin-bottom: 15px">
      <div style="flex: 1">
        <label>Vĩ độ (Lat)</label>
        <input type="text" id="lat" readonly style="background: #eee" />
      </div>
      <div style="flex: 1">
        <label>Kinh độ (Long)</label>
        <input type="text" id="lon" readonly style="background: #eee" />
      </div>
    </div>

    <!-- Brightness Control Section -->
    <div class="item">
      <label>Độ sáng màn hình</label>
      <div style="display: flex; align-items: center; gap: 10px">
        <input
          type="range"
          id="brightness"
          min="10"
          max="100"
          value="100"
          style="flex: 1"
          oninput="updateBrightnessLabel()"
        />
        <span
          id="brightnessValue"
          style="min-width: 45px; font-weight: bold; color: var(--p)"
          >100%</span
        >
      </div>
    </div>

    <button id="btn" onclick="sendData()">Lưu Cấu Hình</button>
    <div id="msg"></div>
  </div>

  <script>
    const cities=[{name:"Hà Nội",lat:21.0285,lon:105.8542},{name:"Hải Phòng",lat:20.8449,lon:106.6881},{name:"Đà Nẵng",lat:16.0544,lon:108.2022},{name:"TP.HCM",lat:10.7626,lon:106.6601},{name:"Cần Thơ",lat:10.0333,lon:105.7833}];
    const citySelect=document.getElementById("city"),latInp=document.getElementById("lat"),lonInp=document.getElementById("lon"),pwInp=document.getElementById("pw");
    let selectedWiFi=null,wifiNetworks=[];
    cities.forEach((c,i)=>{let o=document.createElement("option");o.value=i;o.innerHTML=c.name;citySelect.appendChild(o);});
    function updateCoords(){const c=cities[citySelect.value];latInp.value=c.lat;lonInp.value=c.lon;}
    updateCoords();
    function updateBrightnessLabel(){document.getElementById('brightnessValue').textContent=document.getElementById('brightness').value+'%';}
    function scanWiFi(){
      const b=document.getElementById("scanBtn"),t=document.getElementById("scanText");
      b.disabled=true;t.innerHTML='<span class="spinner"></span>Quét...';
      fetch('/api/wifi/scan').then(r=>r.json()).then(()=>setTimeout(pollScanResults,1000)).catch(()=>{b.disabled=false;t.innerHTML="Quét lại"});
    }
    function pollScanResults(){
      fetch('/api/wifi/results').then(r=>r.json()).then(d=>{
        if(d.status==="scanning")setTimeout(pollScanResults,800);
        else{document.getElementById("scanBtn").disabled=false;document.getElementById("scanText").innerHTML="Quét lại";wifiNetworks=d.networks||[];displayWiFiList(wifiNetworks);}
      }).catch(()=>{document.getElementById("scanBtn").disabled=false;document.getElementById("scanText").innerHTML="Quét lại";});
    }
    function displayWiFiList(nets){
      const l=document.getElementById("wifiList"),c=document.getElementById("wifiListContainer");
      l.innerHTML="";
      if(!nets.length){l.innerHTML='<div style="padding:10px;text-align:center;color:#999">Không tìm thấy WiFi</div>';c.classList.remove("hidden");return;}
      nets.forEach((n,i)=>{
        const it=document.createElement("div");it.className="wifi-item";it.onclick=()=>selectWiFi(i);
        it.innerHTML=`<div><b>${n.ssid}</b><div style="font-size:.8rem;color:#666">${n.rssi} dBm</div></div>${n.encryption!=="OPEN"?'<span>🔒</span>':''}`;
        l.appendChild(it);
      });
      c.classList.remove("hidden");
    }
    function selectWiFi(i){
      selectedWiFi=wifiNetworks[i];
      document.querySelectorAll(".wifi-item").forEach((it,idx)=>it.classList.toggle("selected",idx===i));
      if(selectedWiFi.encryption==="OPEN"){pwInp.disabled=true;pwInp.value="";pwInp.placeholder="Không cần mật khẩu";}
      else{pwInp.disabled=false;pwInp.placeholder="Nhập mật khẩu WiFi";pwInp.focus();}
    }
    function sendData(){
      const btn=document.getElementById("btn"),msg=document.getElementById("msg");
      if(!selectedWiFi){msg.innerHTML="Vui lòng chọn WiFi!";msg.className="error";msg.style.display="block";return;}
      const pw=pwInp.value;
      if(selectedWiFi.encryption!=="OPEN"&&!pw){msg.innerHTML="Vui lòng nhập mật khẩu!";msg.className="error";msg.style.display="block";return;}
      btn.disabled=true;msg.style.display="block";msg.innerHTML='<span class="spinner"></span>Đang lưu & kết nối...';msg.className="info";
      fetch('/api/save',{
        method:'POST',
        headers:{'Content-Type':'application/json'},
        body:JSON.stringify({ssid:selectedWiFi.ssid,password:pw,latitude:parseFloat(latInp.value),longitude:parseFloat(lonInp.value),brightness:parseInt(document.getElementById('brightness').value)})
      }).then(r=>r.json()).then(()=>{msg.innerHTML="✅ Lưu thành công!";msg.className="success";})
      .catch(()=>{msg.innerHTML="Lỗi lưu cấu hình!";msg.className="error";btn.disabled=false;});
    }
  </script>
</body></html>
)rawliteral";

// FreeRTOS Weather Task on Core 0
// Uses exit flag so it can finish current HTTP request cleanly before dying,
// preventing TCP socket leak when wifi_app_stop() is called mid-fetch.
static void weather_task(void *pv) {
    vTaskDelay(pdMS_TO_TICKS(3000));
    while (!s_weather_task_exit) {
        if (s_wifi_connected && s_cached_weather.valid == false || (millis() % 600000 < 5000)) {
            // Skip weather fetch if in XiaoZhi AI mode to dedicate all network and TLS memory to AI chat
            if (display_get_audio_mode() == AUDIO_MODE_XIAOZHI) {
                for (int i = 0; i < 300 && !s_weather_task_exit; i++) {
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                continue;
            }

            char url[160];
            snprintf(url, sizeof(url),
                     "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current=temperature_2m,relative_humidity_2m",
                     s_config_lat, s_config_lon);

            Serial.printf("[Weather Task] Fetching from Open-Meteo (Lat: %.4f, Lon: %.4f)...\n", s_config_lat, s_config_lon);
            WiFiClientSecure client;
            client.setInsecure();
            HTTPClient http;
            http.begin(client, url);
            http.setTimeout(5000);  // 5s timeout — task checks exit flag after this returns
            int code = http.GET();
            if (!s_weather_task_exit && code == 200) {
                String payload = http.getString();
                JsonDocument doc;
                DeserializationError err = deserializeJson(doc, payload);
                if (!err) {
                    s_cached_weather.temp_c = doc["current"]["temperature_2m"];
                    s_cached_weather.humidity = doc["current"]["relative_humidity_2m"];
                    s_cached_weather.valid = true;
                    Serial.printf("[Weather Task] Updated: %.1f C, %d%% Hum\n", s_cached_weather.temp_c, s_cached_weather.humidity);
                }
            } else if (code != 200) {
                Serial.printf("[Weather Task] HTTP failed: %d\n", code);
            }
            http.end();  // Always close connection to free socket
        }

        // Sleep in short intervals to check exit flag quickly (100ms slices × 6000 = 10 min)
        for (int i = 0; i < 6000 && !s_weather_task_exit; i++) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    // Clean self-delete
    s_weather_task_handle = nullptr;
    vTaskDelete(NULL);
}

static void load_stored_config() {
    s_prefs.begin("esp32config", true);
    String ssid = s_prefs.getString("ssid", "");
    String pwd  = s_prefs.getString("password", "");
    ssid.toCharArray(s_config_ssid, sizeof(s_config_ssid));
    pwd.toCharArray(s_config_pwd, sizeof(s_config_pwd));

    s_config_lat = s_prefs.getFloat("latitude", 21.0285f);
    s_config_lon = s_prefs.getFloat("longitude", 105.8542f);
    s_config_brightness = s_prefs.getUChar("brightness", 100);
    s_prefs.end();

    display_set_brightness(s_config_brightness);
    Serial.printf("[Config] Loaded: SSID='%s', Lat=%.4f, Lon=%.4f, Brightness=%d%%\n",
                  s_config_ssid, s_config_lat, s_config_lon, s_config_brightness);
}

static void save_stored_config(const char *ssid, const char *pwd, float lat, float lon, uint8_t brightness) {
    s_prefs.begin("esp32config", false);
    s_prefs.putString("ssid", ssid);
    s_prefs.putString("password", pwd);
    s_prefs.putFloat("latitude", lat);
    s_prefs.putFloat("longitude", lon);
    s_prefs.putUChar("brightness", brightness);
    s_prefs.end();

    strncpy(s_config_ssid, ssid, sizeof(s_config_ssid) - 1);
    strncpy(s_config_pwd, pwd, sizeof(s_config_pwd) - 1);
    s_config_lat = lat;
    s_config_lon = lon;
    s_config_brightness = brightness;

    display_set_brightness(brightness);
    Serial.printf("[Config] Saved: SSID='%s', Lat=%.4f, Lon=%.4f, Brightness=%d%%\n",
                  ssid, lat, lon, brightness);
}

static void setup_web_endpoints() {
    if (!s_server) return;
    // 1. Root page -> serve embedded HTML from PROGMEM
    s_server->on("/", HTTP_GET, []() {
        if (s_server) s_server->send_P(200, "text/html", INDEX_HTML);
    });

    // Captive Portal probes for iOS / Android / Windows
    s_server->on("/generate_204", HTTP_GET, []() { if (s_server) s_server->send_P(200, "text/html", INDEX_HTML); });
    s_server->on("/gen_204", HTTP_GET, []() { if (s_server) s_server->send_P(200, "text/html", INDEX_HTML); });
    s_server->on("/hotspot-detect.html", HTTP_GET, []() { if (s_server) s_server->send_P(200, "text/html", INDEX_HTML); });
    s_server->on("/connecttest.txt", HTTP_GET, []() { if (s_server) s_server->send(200, "text/plain", "Microsoft Connect Test"); });
    s_server->on("/ncsi.txt", HTTP_GET, []() { if (s_server) s_server->send(200, "text/plain", "Microsoft NCSI"); });

    // 2. Status API
    s_server->on("/status", HTTP_GET, []() {
        if (!s_server) return;
        bool conn = (WiFi.status() == WL_CONNECTED);
        String ip = conn ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
        String ssid = conn ? WiFi.SSID() : AP_SSID_NAME;
        String json = "{\"connected\":" + String(conn ? "true" : "false") +
                      ",\"ssid\":\"" + ssid +
                      "\",\"ip\":\"" + ip + "\"}";
        s_server->send(200, "application/json", json);
    });

    // 3. Trigger WiFi scan (compatible with Clock branch API)
    s_server->on("/api/wifi/scan", HTTP_GET, []() {
        if (!s_server) return;
        Serial.println("[WiFi API] Starting WiFi Scan...");
        s_scan_in_progress = true;
        s_scan_complete = false;
        WiFi.scanNetworks(true); // Asynchronous scan
        s_server->send(200, "application/json", "{\"status\":\"started\",\"message\":\"Scan started\"}");
    });

    // 4. Get WiFi scan results
    s_server->on("/api/wifi/results", HTTP_GET, []() {
        if (!s_server) return;
        int16_t scan_res = WiFi.scanComplete();
        if (scan_res == WIFI_SCAN_RUNNING) {
            s_server->send(200, "application/json", "{\"status\":\"scanning\",\"message\":\"Scan in progress\"}");
            return;
        }

        s_scan_in_progress = false;
        s_scan_complete = true;
        int n = (scan_res >= 0) ? scan_res : 0;

        JsonDocument doc;
        doc["status"] = "complete";
        JsonArray networks = doc["networks"].to<JsonArray>();

        for (int i = 0; i < n && i < 20; i++) {
            JsonObject net = networks.add<JsonObject>();
            net["ssid"] = WiFi.SSID(i);
            net["rssi"] = WiFi.RSSI(i);
            net["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "OPEN" : "ENCRYPTED";
        }

        String response;
        serializeJson(doc, response);
        s_server->send(200, "application/json", response);
        WiFi.scanDelete();
    });

    // Legacy direct /scan endpoint
    s_server->on("/scan", HTTP_GET, []() {
        if (!s_server) return;
        int n = WiFi.scanNetworks();
        String json = "[";
        for (int i = 0; i < n; ++i) {
            if (i > 0) json += ",";
            json += "{\"ssid\":\"" + WiFi.SSID(i) +
                    "\",\"rssi\":" + String(WiFi.RSSI(i)) +
                    ",\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false") + "}";
        }
        json += "]";
        s_server->send(200, "application/json", json);
    });

    // 5. Save credentials API (Clock branch compatible POST /api/save)
    auto handle_save = []() {
        if (!s_server) return;
        String ssid = "";
        String pwd = "";
        float lat = 21.0285f;
        float lon = 105.8542f;
        uint8_t brightness = 100;

        if (s_server->hasArg("plain")) {
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, s_server->arg("plain"));
            if (!err) {
                if (doc["ssid"].is<const char*>()) ssid = doc["ssid"].as<String>();
                if (doc["password"].is<const char*>()) pwd = doc["password"].as<String>();
                if (doc["latitude"].is<float>()) lat = doc["latitude"].as<float>();
                if (doc["longitude"].is<float>()) lon = doc["longitude"].as<float>();
                if (doc["brightness"].is<uint8_t>()) brightness = doc["brightness"].as<uint8_t>();
            }
        } else if (s_server->hasArg("ssid")) {
            ssid = s_server->arg("ssid");
            pwd  = s_server->hasArg("password") ? s_server->arg("password") : "";
        }

        if (ssid.length() > 0) {
            save_stored_config(ssid.c_str(), pwd.c_str(), lat, lon, brightness);
            s_server->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Configuration saved\"}");

            delay(500);
            s_dns_server.stop();
            WiFi.mode(WIFI_AP_STA);
            WiFi.begin(ssid.c_str(), pwd.c_str());
            s_wifi_state = WIFI_STATE_CONNECTING;
            s_connect_start_ts = millis();
        } else {
            s_server->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing SSID\"}");
        }
    };

    s_server->on("/api/save", HTTP_POST, handle_save);
    s_server->on("/save", HTTP_POST, handle_save);

    // 6. Restart Device API
    s_server->on("/restart", HTTP_POST, []() {
        if (s_server) s_server->send(200, "application/json", "{\"status\":\"restarting\"}");
        delay(800);
        ESP.restart();
    });

    // Captive portal fallback redirect
    s_server->onNotFound([]() {
        if (s_server) {
            s_server->sendHeader("Location", "http://192.168.4.1/", true);
            s_server->send(302, "text/plain", "");
        }
    });
}

void wifi_app_init() {
    load_stored_config();

    Serial.println("[WiFi] Starting non-blocking WiFi connect...");
    WiFi.mode(WIFI_STA);

    if (strlen(s_config_ssid) > 0) {
        Serial.printf("[WiFi] Connecting to saved SSID: %s\n", s_config_ssid);
        WiFi.begin(s_config_ssid, s_config_pwd);
        s_wifi_state = WIFI_STATE_CONNECTING;
        s_connect_start_ts = millis();
    } else {
        Serial.println("[WiFi] No saved credentials found. Launching AP Captive Portal...");
        wifi_app_start_ap_portal();
    }
    s_wifi_connected = false;

    // Configure SNTP background auto-sync
    configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");

    // Spawn Core 0 Weather background task
    s_weather_task_exit = false;  // Ensure flag is clear before spawning
    if (!s_weather_task_handle) {
        xTaskCreatePinnedToCore(weather_task, "weather_task", 6144, nullptr, 1, &s_weather_task_handle, 0);
    }
}

void wifi_app_start_ap_portal() {
    Serial.println("[WiFi] Launching AP Captive Portal...");
    WiFi.disconnect();
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID_NAME);

    // Start DNS Server on port 53 to redirect all domain lookups to 192.168.4.1 (Captive Portal)
    s_dns_server.setErrorReplyCode(DNSReplyCode::NoError);
    s_dns_server.start(53, "*", WiFi.softAPIP());

    if (!s_server) s_server = new WebServer(80);
    setup_web_endpoints();
    ElegantOTA.begin(s_server);
    s_server->begin();
    s_wifi_state = WIFI_STATE_AP_MODE;
    Serial.printf("[WiFi] AP Captive Portal active at: %s (IP: %s)\n", AP_SSID_NAME, WiFi.softAPIP().toString().c_str());
    display_toast("AP: " AP_SSID_NAME);
}

void wifi_app_stop() {
    s_dns_server.stop();

    // Signal weather_task to exit cleanly (it will finish current HTTP call, close socket, then self-delete)
    if (s_weather_task_handle) {
        s_weather_task_exit = true;
        // Give it up to 6s to finish any ongoing HTTP request (timeout is 5s)
        uint32_t wait_start = millis();
        while (s_weather_task_handle && (millis() - wait_start < 6000)) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        // If still running after 6s (shouldn't happen), force kill as last resort
        if (s_weather_task_handle) {
            Serial.println("[WiFi] Weather task did not exit in time — force killing");
            vTaskDelete(s_weather_task_handle);
            s_weather_task_handle = nullptr;
        }
        s_weather_task_exit = false;  // Reset for potential re-init
    }

    if (s_wifi_state != WIFI_STATE_IDLE || WiFi.getMode() != WIFI_OFF) {
        Serial.println("[WiFi] Stopping WiFi subsystem & freeing 2.4GHz RF...");
        if (s_server) {
            s_server->close();
            delete s_server;
            s_server = nullptr;
        }
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        s_wifi_connected = false;
        s_wifi_state = WIFI_STATE_IDLE;
        Serial.println("[WiFi] Subsystem stopped. RF OFF.");
    }
}

void wifi_app_loop(bool is_bt_streaming) {
    if (s_wifi_state == WIFI_STATE_IDLE || is_bt_streaming) return;

    static uint32_t last_wifi_tick = 0;
    uint32_t now = millis();
    if (now - last_wifi_tick < 100) return;
    last_wifi_tick = now;

    if (s_wifi_state == WIFI_STATE_CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            s_dns_server.stop();
            s_wifi_state = WIFI_STATE_CONNECTED;
            s_wifi_connected = true;
            Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());

            configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");

            if (!s_server) s_server = new WebServer(80);
            setup_web_endpoints();
            ElegantOTA.begin(s_server);
            s_server->begin();
            Serial.println("[HTTP] Web Server & ElegantOTA active on port 80");
            display_toast("WIFI CONNECTED");
        } else if (now - s_connect_start_ts > 15000) {
            // Timeout 15s -> start AP mode non-blockingly
            wifi_app_start_ap_portal();
        }
    } else if (s_wifi_state == WIFI_STATE_CONNECTED) {
        if (WiFi.status() != WL_CONNECTED) {
            s_wifi_connected = false;
            s_wifi_state = WIFI_STATE_CONNECTING;
            s_connect_start_ts = now;
            if (strlen(s_config_ssid) > 0) {
                WiFi.begin(s_config_ssid, s_config_pwd);
            }
            return;
        }

        if (s_server) s_server->handleClient();
        ElegantOTA.loop();

        // Periodic NTP time re-sync: 1 hour / time (3600000 ms)
        static uint32_t s_last_time_sync = 0;
        if (s_last_time_sync == 0) s_last_time_sync = now;
        if (now - s_last_time_sync >= 3600000) {
            s_last_time_sync = now;
            configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
            Serial.println("[NTP] 1-Hour Time Sync re-triggered");
        }
    } else if (s_wifi_state == WIFI_STATE_AP_MODE) {
        s_dns_server.processNextRequest();

        if (WiFi.status() == WL_CONNECTED) {
            s_dns_server.stop();
            s_wifi_state = WIFI_STATE_CONNECTED;
            s_wifi_connected = true;
            Serial.printf("[WiFi] AP Mode -> Transitioned to Connected! IP: %s\n", WiFi.localIP().toString().c_str());
            display_toast("WIFI CONNECTED");
        }
        if (s_server) s_server->handleClient();
        ElegantOTA.loop();
    }
}

void wifi_app_reset_settings() {
    Serial.println("[WiFi] Resetting WiFi settings...");
    s_prefs.begin("esp32config", false);
    s_prefs.clear();
    s_prefs.end();
    s_config_ssid[0] = '\0';
    s_config_pwd[0] = '\0';
    wifi_app_start_ap_portal();
}

bool wifi_app_is_connected() {
    return (WiFi.status() == WL_CONNECTED);
}

bool wifi_app_is_connecting() {
    return (s_wifi_state == WIFI_STATE_CONNECTING);
}

bool wifi_app_is_ap_mode() {
    return (s_wifi_state == WIFI_STATE_AP_MODE);
}

void wifi_app_get_time_str(char *out, size_t max_len) {
    if (!out || max_len == 0) return;

    time_t now = time(nullptr);
    struct tm ti;
    localtime_r(&now, &ti);

    if (ti.tm_year >= (2025 - 1900)) {
        snprintf(out, max_len, "%02d:%02d:%02d", ti.tm_hour, ti.tm_min, ti.tm_sec);
        return;
    }

    bool blink = (millis() / 500) % 2;
    if (blink) {
        snprintf(out, max_len, "--:--:--");
    } else {
        snprintf(out, max_len, "-- -- --");
    }
}

WeatherData wifi_app_get_weather() {
    return s_cached_weather;
}

SolarDate wifi_app_get_solar_date() {
    SolarDate d = { 0, 0, 0, 0, false };
    time_t now = time(nullptr);
    struct tm ti;
    localtime_r(&now, &ti);

    if (ti.tm_year >= (2025 - 1900)) {
        d.year = ti.tm_year + 1900;
        d.month = ti.tm_mon + 1;
        d.day = ti.tm_mday;
        d.day_of_week = ti.tm_wday;
        d.valid = true;
    }
    return d;
}
