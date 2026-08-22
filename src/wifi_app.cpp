#include "wifi_app.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ElegantOTA.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <time.h>
#include <esp_wifi.h>
#include "display.h"

static WebServer      s_server(80);
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
<html lang="vi">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>ESP32 Audio & Clock Config</title>
  <style>
    :root {
      --p: #007bff;
      --bg: #f4f4f9;
      --txt: #333;
    }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      background: var(--bg);
      color: var(--txt);
      display: flex;
      justify-content: center;
      padding: 20px;
      padding-bottom: 70px;
      margin: 0;
    }
    .card {
      background: #fff;
      padding: 24px;
      border-radius: 12px;
      box-shadow: 0 4px 16px rgba(0, 0, 0, 0.1);
      width: 100%;
      max-width: 400px;
    }
    h2 {
      margin-top: 0;
      color: var(--p);
      text-align: center;
      font-size: 1.5rem;
    }
    .item {
      margin-bottom: 15px;
    }
    label {
      display: block;
      margin-bottom: 5px;
      font-weight: bold;
      font-size: 0.9rem;
    }
    input, select {
      width: 100%;
      padding: 12px;
      border: 1px solid #ddd;
      border-radius: 8px;
      box-sizing: border-box;
      font-size: 1rem;
    }
    input:disabled {
      background: #f0f0f0;
      cursor: not-allowed;
    }
    input[type="range"] {
      -webkit-appearance: none;
      appearance: none;
      background: transparent;
      cursor: pointer;
      padding: 0;
    }
    input[type="range"]::-webkit-slider-runnable-track {
      background: #ddd;
      height: 8px;
      border-radius: 4px;
    }
    input[type="range"]::-moz-range-track {
      background: #ddd;
      height: 8px;
      border-radius: 4px;
    }
    input[type="range"]::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: var(--p);
      cursor: pointer;
      margin-top: -6px;
    }
    input[type="range"]::-moz-range-thumb {
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: var(--p);
      cursor: pointer;
      border: none;
    }
    button {
      width: 100%;
      padding: 14px;
      background: var(--p);
      color: #fff;
      border: none;
      border-radius: 8px;
      font-size: 1.1rem;
      cursor: pointer;
      transition: 0.3s;
      margin-top: 10px;
      font-weight: bold;
    }
    button:disabled {
      background: #ccc;
      cursor: not-allowed;
    }
    button:hover:not(:disabled) {
      background: #0056b3;
    }
    button.secondary {
      background: #6c757d;
    }
    button.secondary:hover:not(:disabled) {
      background: #545b62;
    }
    #msg {
      margin-top: 15px;
      padding: 12px;
      border-radius: 6px;
      text-align: center;
      display: none;
      font-size: 0.95rem;
    }
    .success {
      background: #d4edda;
      color: #155724;
      border: 1px solid #c3e6cb;
    }
    .error {
      background: #f8d7da;
      color: #721c24;
      border: 1px solid #f5c6cb;
    }
    .info {
      background: #d1ecf1;
      color: #0c5460;
      border: 1px solid #bee5eb;
    }
    footer {
      position: fixed;
      bottom: 0;
      left: 0;
      width: 100%;
      background: #fff;
      text-align: center;
      padding: 12px 0;
      box-shadow: 0 -2px 8px rgba(0, 0, 0, 0.1);
      font-size: 0.9rem;
      color: var(--txt);
    }
    footer a {
      color: var(--p);
      text-decoration: none;
      font-weight: bold;
    }
    footer a:hover {
      text-decoration: underline;
    }
    .wifi-list {
      max-height: 200px;
      overflow-y: auto;
      border: 1px solid #ddd;
      border-radius: 8px;
      margin-bottom: 15px;
    }
    .wifi-item {
      padding: 12px;
      border-bottom: 1px solid #eee;
      cursor: pointer;
      transition: background 0.2s;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .wifi-item:last-child {
      border-bottom: none;
    }
    .wifi-item:hover {
      background: #f8f9fa;
    }
    .wifi-item.selected {
      background: #e7f3ff;
      border-left: 4px solid var(--p);
    }
    .wifi-info {
      flex: 1;
    }
    .wifi-ssid {
      font-weight: bold;
      margin-bottom: 2px;
    }
    .wifi-signal {
      font-size: 0.85rem;
      color: #666;
    }
    .wifi-lock {
      color: #666;
      font-size: 1.2rem;
    }
    .spinner {
      border: 3px solid #f3f3f3;
      border-top: 3px solid var(--p);
      border-radius: 50%;
      width: 18px;
      height: 18px;
      animation: spin 1s linear infinite;
      display: inline-block;
      margin-right: 8px;
      vertical-align: middle;
    }
    @keyframes spin {
      0% { transform: rotate(0deg); }
      100% { transform: rotate(360deg); }
    }
    .hidden {
      display: none;
    }
  </style>
</head>
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
    const cities = [
      { name: "Hà Nội", lat: 21.0285, lon: 105.8542 },
      { name: "Vĩnh Hưng - HN", lat: 20.986104, lon: 105.876657 },
      { name: "Hải Phòng", lat: 20.8449, lon: 106.6881 },
      { name: "Thủy Nguyên", lat: 20.950479, lon: 106.653507 },
      { name: "An Sơn - TN", lat: 20.992789, lon: 106.560116 },
      { name: "Đà Nẵng", lat: 16.0544, lon: 108.2022 },
      { name: "TP. Hồ Chí Minh", lat: 10.7626, lon: 106.6601 },
      { name: "Cần Thơ", lat: 10.0333, lon: 105.7833 },
      { name: "Điện Biên", lat: 21.3833, lon: 103.0167 },
    ];

    const citySelect = document.getElementById("city");
    const latInp = document.getElementById("lat");
    const lonInp = document.getElementById("lon");
    const pwInp = document.getElementById("pw");

    let selectedWiFi = null;
    let wifiNetworks = [];

    cities.forEach((c, index) => {
      let opt = document.createElement("option");
      opt.value = index;
      opt.innerHTML = c.name;
      citySelect.appendChild(opt);
    });

    function updateCoords() {
      const c = cities[citySelect.value];
      latInp.value = c.lat;
      lonInp.value = c.lon;
    }
    updateCoords();

    function updateBrightnessLabel() {
      const brightness = document.getElementById('brightness').value;
      document.getElementById('brightnessValue').textContent = brightness + '%';
    }

    function getSignalStrength(rssi) {
      if (rssi >= -55) return "Rất mạnh";
      if (rssi >= -65) return "Mạnh";
      if (rssi >= -75) return "Trung bình";
      if (rssi >= -85) return "Yếu";
      return "Rất yếu";
    }

    function scanWiFi() {
      const scanBtn = document.getElementById("scanBtn");
      const scanText = document.getElementById("scanText");
      const msg = document.getElementById("msg");

      scanBtn.disabled = true;
      scanText.innerHTML = '<span class="spinner"></span>Đang quét WiFi...';
      msg.style.display = "none";

      fetch('/api/wifi/scan')
        .then(r => r.json())
        .then(data => {
          setTimeout(pollScanResults, 1000);
        })
        .catch(err => {
          scanBtn.disabled = false;
          scanText.innerHTML = "🔍 Quét lại WiFi";
          msg.innerHTML = "Lỗi khởi động quét WiFi";
          msg.className = "error";
          msg.style.display = "block";
        });
    }

    function pollScanResults() {
      const scanBtn = document.getElementById("scanBtn");
      const scanText = document.getElementById("scanText");
      const msg = document.getElementById("msg");

      fetch('/api/wifi/results')
        .then(r => r.json())
        .then(data => {
          if (data.status === "scanning") {
            setTimeout(pollScanResults, 800);
          } else if (data.status === "complete") {
            scanBtn.disabled = false;
            scanText.innerHTML = "🔍 Quét lại WiFi";
            wifiNetworks = data.networks || [];
            displayWiFiList(wifiNetworks);
          } else {
            scanBtn.disabled = false;
            scanText.innerHTML = "🔍 Quét lại WiFi";
          }
        })
        .catch(err => {
          scanBtn.disabled = false;
          scanText.innerHTML = "🔍 Quét lại WiFi";
        });
    }

    function displayWiFiList(networks) {
      const wifiList = document.getElementById("wifiList");
      const wifiListContainer = document.getElementById("wifiListContainer");
      wifiList.innerHTML = "";

      if (networks.length === 0) {
        wifiList.innerHTML = '<div style="padding:12px; text-align:center; color:#999;">Không tìm thấy WiFi nào</div>';
        wifiListContainer.classList.remove("hidden");
        return;
      }

      networks.forEach((network, index) => {
        const item = document.createElement("div");
        item.className = "wifi-item";
        item.onclick = () => selectWiFi(index);

        const info = document.createElement("div");
        info.className = "wifi-info";

        const ssid = document.createElement("div");
        ssid.className = "wifi-ssid";
        ssid.textContent = network.ssid;

        const signal = document.createElement("div");
        signal.className = "wifi-signal";
        signal.textContent = `${getSignalStrength(network.rssi)} (${network.rssi} dBm)`;

        info.appendChild(ssid);
        info.appendChild(signal);
        item.appendChild(info);

        if (network.encryption !== "OPEN") {
          const lock = document.createElement("div");
          lock.className = "wifi-lock";
          lock.innerHTML = "🔒";
          item.appendChild(lock);
        }

        wifiList.appendChild(item);
      });

      wifiListContainer.classList.remove("hidden");
    }

    function selectWiFi(index) {
      selectedWiFi = wifiNetworks[index];
      const items = document.querySelectorAll(".wifi-item");
      items.forEach((item, i) => {
        if (i === index) item.classList.add("selected");
        else item.classList.remove("selected");
      });

      if (selectedWiFi.encryption === "OPEN") {
        pwInp.disabled = true;
        pwInp.value = "";
        pwInp.placeholder = "WiFi không yêu cầu mật khẩu";
      } else {
        pwInp.disabled = false;
        pwInp.placeholder = "Nhập mật khẩu WiFi";
        pwInp.focus();
      }
    }

    function sendData() {
      const btn = document.getElementById("btn");
      const msg = document.getElementById("msg");

      if (!selectedWiFi) {
        msg.innerHTML = "Vui lòng bấm 'Quét WiFi' và chọn một mạng!";
        msg.className = "error";
        msg.style.display = "block";
        return;
      }

      const password = pwInp.value;
      if (selectedWiFi.encryption !== "OPEN" && !password) {
        msg.innerHTML = "Vui lòng nhập mật khẩu WiFi";
        msg.className = "error";
        msg.style.display = "block";
        pwInp.focus();
        return;
      }

      const latitude = parseFloat(latInp.value);
      const longitude = parseFloat(lonInp.value);
      const brightness = parseInt(document.getElementById('brightness').value);

      const payload = {
        ssid: selectedWiFi.ssid,
        password: password,
        latitude: latitude,
        longitude: longitude,
        brightness: brightness
      };

      btn.disabled = true;
      msg.style.display = "block";
      msg.innerHTML = '<span class="spinner"></span>Đang lưu cài đặt & kết nối WiFi...';
      msg.className = "info";

      fetch('/api/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      })
      .then(r => r.json())
      .then(res => {
        msg.innerHTML = "✅ Lưu thành công! ESP32 đang kết nối WiFi và chuyển sang chế độ Đồng Hồ...";
        msg.className = "success";
      })
      .catch(err => {
        msg.innerHTML = "Lỗi khi lưu cấu hình!";
        msg.className = "error";
        btn.disabled = false;
      });
    }
  </script>

  <footer>
    Power by <a href="https://www.facebook.com/mvt.hp.star/" target="_blank">Mạc Tân</a> | Mobile: <a href="tel:0964335688">0964 335 688</a>
  </footer>
</body>
</html>
)rawliteral";

// FreeRTOS Weather Task on Core 0
// Uses exit flag so it can finish current HTTP request cleanly before dying,
// preventing TCP socket leak when wifi_app_stop() is called mid-fetch.
static void weather_task(void *pv) {
    vTaskDelay(pdMS_TO_TICKS(3000));
    while (!s_weather_task_exit) {
        if (WiFi.status() == WL_CONNECTED) {
            char url[160];
            snprintf(url, sizeof(url),
                     "http://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current=temperature_2m,relative_humidity_2m",
                     s_config_lat, s_config_lon);

            Serial.printf("[Weather Task] Fetching from Open-Meteo (Lat: %.4f, Lon: %.4f)...\n", s_config_lat, s_config_lon);
            WiFiClient client;
            HTTPClient http;
            http.begin(client, url);
            http.setTimeout(4000);  // 4s timeout
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
    // 1. Root page -> serve embedded HTML from PROGMEM
    s_server.on("/", HTTP_GET, []() {
        s_server.send_P(200, "text/html", INDEX_HTML);
    });

    // Captive Portal probes for iOS / Android / Windows
    s_server.on("/generate_204", HTTP_GET, []() { s_server.send_P(200, "text/html", INDEX_HTML); });
    s_server.on("/gen_204", HTTP_GET, []() { s_server.send_P(200, "text/html", INDEX_HTML); });
    s_server.on("/hotspot-detect.html", HTTP_GET, []() { s_server.send_P(200, "text/html", INDEX_HTML); });
    s_server.on("/connecttest.txt", HTTP_GET, []() { s_server.send(200, "text/plain", "Microsoft Connect Test"); });
    s_server.on("/ncsi.txt", HTTP_GET, []() { s_server.send(200, "text/plain", "Microsoft NCSI"); });

    // 2. Status API
    s_server.on("/status", HTTP_GET, []() {
        bool conn = (WiFi.status() == WL_CONNECTED);
        String ip = conn ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
        String ssid = conn ? WiFi.SSID() : AP_SSID_NAME;
        String json = "{\"connected\":" + String(conn ? "true" : "false") +
                      ",\"ssid\":\"" + ssid +
                      "\",\"ip\":\"" + ip + "\"}";
        s_server.send(200, "application/json", json);
    });

    // 3. Trigger WiFi scan (compatible with Clock branch API)
    s_server.on("/api/wifi/scan", HTTP_GET, []() {
        Serial.println("[WiFi API] Starting WiFi Scan...");
        s_scan_in_progress = true;
        s_scan_complete = false;
        WiFi.scanNetworks(true); // Asynchronous scan
        s_server.send(200, "application/json", "{\"status\":\"started\",\"message\":\"Scan started\"}");
    });

    // 4. Get WiFi scan results
    s_server.on("/api/wifi/results", HTTP_GET, []() {
        int16_t scan_res = WiFi.scanComplete();
        if (scan_res == WIFI_SCAN_RUNNING) {
            s_server.send(200, "application/json", "{\"status\":\"scanning\",\"message\":\"Scan in progress\"}");
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
        s_server.send(200, "application/json", response);
        WiFi.scanDelete();
    });

    // Legacy direct /scan endpoint
    s_server.on("/scan", HTTP_GET, []() {
        int n = WiFi.scanNetworks();
        String json = "[";
        for (int i = 0; i < n; ++i) {
            if (i > 0) json += ",";
            json += "{\"ssid\":\"" + WiFi.SSID(i) +
                    "\",\"rssi\":" + String(WiFi.RSSI(i)) +
                    ",\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false") + "}";
        }
        json += "]";
        s_server.send(200, "application/json", json);
    });

    // 5. Save credentials API (Clock branch compatible POST /api/save)
    auto handle_save = []() {
        String ssid = "";
        String pwd = "";
        float lat = 21.0285f;
        float lon = 105.8542f;
        uint8_t brightness = 100;

        if (s_server.hasArg("plain")) {
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, s_server.arg("plain"));
            if (!err) {
                if (doc["ssid"].is<const char*>()) ssid = doc["ssid"].as<String>();
                if (doc["password"].is<const char*>()) pwd = doc["password"].as<String>();
                if (doc["latitude"].is<float>()) lat = doc["latitude"].as<float>();
                if (doc["longitude"].is<float>()) lon = doc["longitude"].as<float>();
                if (doc["brightness"].is<uint8_t>()) brightness = doc["brightness"].as<uint8_t>();
            }
        } else if (s_server.hasArg("ssid")) {
            ssid = s_server.arg("ssid");
            pwd  = s_server.hasArg("password") ? s_server.arg("password") : "";
        }

        if (ssid.length() > 0) {
            save_stored_config(ssid.c_str(), pwd.c_str(), lat, lon, brightness);
            s_server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Configuration saved\"}");

            delay(500);
            s_dns_server.stop();
            WiFi.mode(WIFI_AP_STA);
            WiFi.begin(ssid.c_str(), pwd.c_str());
            s_wifi_state = WIFI_STATE_CONNECTING;
            s_connect_start_ts = millis();
        } else {
            s_server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing SSID\"}");
        }
    };

    s_server.on("/api/save", HTTP_POST, handle_save);
    s_server.on("/save", HTTP_POST, handle_save);

    // 6. Restart Device API
    s_server.on("/restart", HTTP_POST, []() {
        s_server.send(200, "application/json", "{\"status\":\"restarting\"}");
        delay(800);
        ESP.restart();
    });

    // Captive portal fallback redirect
    s_server.onNotFound([]() {
        s_server.sendHeader("Location", "http://192.168.4.1/", true);
        s_server.send(302, "text/plain", "");
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

    setup_web_endpoints();
    ElegantOTA.begin(&s_server);
    s_server.begin();
    s_wifi_state = WIFI_STATE_AP_MODE;
    Serial.printf("[WiFi] AP Captive Portal active at: %s (IP: %s)\n", AP_SSID_NAME, WiFi.softAPIP().toString().c_str());
    display_toast("AP: " AP_SSID_NAME);
}

void wifi_app_stop() {
    s_dns_server.stop();

    if (s_wifi_state != WIFI_STATE_IDLE || WiFi.getMode() != WIFI_OFF) {
        Serial.println("[WiFi] Stopping WiFi subsystem with verified state polling...");
        s_server.close();
        s_server.stop();
        vTaskDelay(pdMS_TO_TICKS(50));

        WiFi.disconnect(true, true);
        WiFi.mode(WIFI_OFF);

        uint32_t wait_start = millis();
        while (WiFi.getMode() != WIFI_OFF && (millis() - wait_start < 1500)) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        s_wifi_connected = false;
        s_wifi_state = WIFI_STATE_IDLE;
        Serial.printf("[WiFi] WiFi fully STOPPED & VERIFIED (Mode: %d, FreeHeap=%u, MaxBlock=%u)\n",
                      (int)WiFi.getMode(), (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());
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

            setup_web_endpoints();
            ElegantOTA.begin(&s_server);
            s_server.begin();
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

        s_server.handleClient();
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
        s_server.handleClient();
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
