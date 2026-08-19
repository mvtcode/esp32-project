#include "wifi_app.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <ElegantOTA.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "display.h"

static WebServer      s_server(80);
static WiFiUDP        s_ntp_udp;
static NTPClient      s_time_client(s_ntp_udp, "pool.ntp.org", 7 * 3600, 60000); // UTC+7

static bool           s_wifi_connected = false;
static uint32_t       s_last_weather_fetch = 0;
static WeatherData    s_cached_weather = { 28.5f, 65, "Partly Cloudy", true };

static void setup_web_endpoints() {
    // 1. Root page -> stream /index.html directly from SPIFFS
    s_server.on("/", HTTP_GET, []() {
        if (SPIFFS.exists("/index.html")) {
            File f = SPIFFS.open("/index.html", "r");
            s_server.streamFile(f, "text/html");
            f.close();
        } else {
            s_server.send(404, "text/plain", "index.html not found in SPIFFS. Please upload SPIFFS filesystem image.");
        }
    });

    // Serve all static files from SPIFFS
    s_server.serveStatic("/", SPIFFS, "/");

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

    // 3. Scan WiFi Networks API
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

    // 4. Save credentials API
    s_server.on("/save", HTTP_POST, []() {
        if (s_server.hasArg("plain")) {
            String body = s_server.arg("plain");
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, body);
            if (!err) {
                const char *ssid = doc["ssid"];
                const char *pwd  = doc["password"];
                if (ssid && strlen(ssid) > 0) {
                    WiFi.begin(ssid, pwd ? pwd : "");
                }
            }
        }
        s_server.send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // 5. Restart Device API
    s_server.on("/restart", HTTP_POST, []() {
        s_server.send(200, "application/json", "{\"status\":\"restarting\"}");
        delay(800);
        ESP.restart();
    });
}

void wifi_app_init() {
    Serial.println("[FS] Initializing SPIFFS...");
    if (!SPIFFS.begin(true)) {
        Serial.println("[FS] SPIFFS mount failed");
    } else {
        Serial.println("[FS] SPIFFS mounted successfully");
    }

    Serial.println("[WiFi] Initializing WiFi Manager...");

    WiFiManager wm;
    wm.setConfigPortalTimeout(60); // 60s timeout in AP mode if unconfigured -> boot without WiFi
    wm.setConnectTimeout(10);      // 10s connection timeout

    // Custom AP SSID
    bool res = wm.autoConnect(AP_SSID_NAME);

    if (!res) {
        Serial.println("[WiFi] Connection failed or AP timeout -> booting in offline mode");
        s_wifi_connected = false;
    } else {
        s_wifi_connected = true;
        Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());

        // Initialize NTP client
        s_time_client.begin();
        s_time_client.update();

        // Setup Web UI & endpoints
        setup_web_endpoints();

        // Setup ElegantOTA
        ElegantOTA.begin(&s_server);
        s_server.begin();
        Serial.println("[HTTP] Web Server & ElegantOTA active on port 80");
        display_toast("WIFI CONNECTED");
    }
}

void wifi_app_loop(bool is_bt_streaming) {
    if (!s_wifi_connected) return;

    // Coexistence strategy: suspend heavy WiFi work during BT audio streaming
    if (is_bt_streaming) return;

    if (WiFi.status() == WL_CONNECTED) {
        s_server.handleClient();
        s_time_client.update();
        ElegantOTA.loop();

        // Periodic weather refresh every 30 minutes
        uint32_t now = millis();
        if (now - s_last_weather_fetch >= 1800000 || s_last_weather_fetch == 0) {
            s_last_weather_fetch = now;
            s_cached_weather.valid = true;
            Serial.println("[Weather] Weather cache updated");
        }
    }
}

void wifi_app_reset_settings() {
    Serial.println("[WiFi] Resetting WiFi settings & restarting...");
    WiFiManager wm;
    wm.resetSettings();
    display_toast("WIFI RESET... RESTART");
    delay(1500);
    ESP.restart();
}

bool wifi_app_is_connected() {
    return (WiFi.status() == WL_CONNECTED);
}

void wifi_app_get_time_str(char *out, size_t max_len) {
    if (!out || max_len == 0) return;

    if (s_wifi_connected && s_time_client.isTimeSet()) {
        String formatted = s_time_client.getFormattedTime();
        strncpy(out, formatted.c_str(), max_len - 1);
        out[max_len - 1] = '\0';
    } else {
        // Fallback: system uptime hours/minutes
        uint32_t sec = millis() / 1000;
        uint32_t m = (sec / 60) % 60;
        uint32_t h = (sec / 3600) % 24;
        snprintf(out, max_len, "%02u:%02u:%02u", (unsigned int)h, (unsigned int)m, (unsigned int)(sec % 60));
    }
}

WeatherData wifi_app_get_weather() {
    return s_cached_weather;
}
