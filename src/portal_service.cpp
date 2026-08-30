#include "portal_service.h"
#include "weather_service.h"

WebServer PortalService::server(80);
DNSServer PortalService::dnsServer;
Preferences PortalService::prefs;
bool PortalService::is_portal_active = false;
bool PortalService::is_server_started = false;
bool PortalService::is_scanning = false;

void PortalService::scanTaskWorker(void *param) {
    Serial.println("[PortalService] Scanning WiFi in background task...");
    WiFi.scanDelete();
    WiFi.scanNetworks(false, true);
    is_scanning = false;
    Serial.printf("[PortalService] Scan completed! Found %d networks.\n", WiFi.scanComplete());
    vTaskDelete(NULL);
}

AppConfig PortalService::loadConfig() {
    AppConfig cfg;
    prefs.begin("app_cfg", false);
    
    cfg.wifi_ssid = prefs.getString("wifi_ssid", "");
    cfg.wifi_pass = prefs.getString("wifi_pass", "");
    cfg.google_script_url = prefs.getString("script_url", "");
    cfg.upload_cooldown = prefs.getInt("cooldown", 5);
    cfg.res_mode = prefs.getInt("res_mode", 0);
    cfg.upload_enabled = prefs.getBool("upload_en", false);
    cfg.led_enabled = prefs.getBool("led_en", true);
    cfg.audio_enabled = prefs.getBool("audio_en", true);
    cfg.audio_volume = prefs.getInt("audio_vol", 40);
    cfg.weather_city = prefs.getString("w_city", "Hà Nội");
    cfg.weather_lat = prefs.getFloat("w_lat", 21.0285f);
    cfg.weather_lon = prefs.getFloat("w_lon", 105.8542f);
    cfg.standby_timeout = prefs.getInt("sb_time", 15);

    prefs.end();
    return cfg;
}

void PortalService::saveConfig(const AppConfig &cfg) {
    prefs.begin("app_cfg", false);
    prefs.putString("wifi_ssid", cfg.wifi_ssid);
    prefs.putString("wifi_pass", cfg.wifi_pass);
    prefs.putString("script_url", cfg.google_script_url);
    prefs.putInt("cooldown", cfg.upload_cooldown);
    prefs.putInt("res_mode", cfg.res_mode);
    prefs.putBool("upload_en", cfg.upload_enabled);
    prefs.putBool("led_en", cfg.led_enabled);
    prefs.putBool("audio_en", cfg.audio_enabled);
    prefs.putInt("audio_vol", cfg.audio_volume);
    prefs.putString("w_city", cfg.weather_city);
    prefs.putFloat("w_lat", cfg.weather_lat);
    prefs.putFloat("w_lon", cfg.weather_lon);
    prefs.putInt("sb_time", cfg.standby_timeout);
    prefs.end();
    Serial.println("[PortalService] Config saved to NVS successfully");
}

String PortalService::getContentType(const String &filename) {
    if (filename.endsWith(".html") || filename.endsWith(".htm")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".json")) return "application/json";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".gif")) return "image/gif";
    else if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) return "image/jpeg";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".svg")) return "image/svg+xml";
    else if (filename.endsWith(".xml")) return "text/xml";
    else if (filename.endsWith(".pdf")) return "application/x-pdf";
    else if (filename.endsWith(".zip")) return "application/x-zip";
    else if (filename.endsWith(".gz")) return "application/x-gzip";
    else if (filename.endsWith(".txt")) return "text/plain";
    return "application/octet-stream";
}

bool PortalService::handleFileRead(String path) {
    if (path.endsWith("/")) path += "index.html";
    String contentType = getContentType(path);
    String pathWithGz = path + ".gz";

    if (LittleFS.exists(pathWithGz) || LittleFS.exists(path)) {
        if (LittleFS.exists(pathWithGz)) path += ".gz";
        File file = LittleFS.open(path, "r");
        server.streamFile(file, contentType);
        file.close();
        return true;
    }
    return false;
}

void PortalService::handleRoot() {
    if (!handleFileRead("/index.html")) {
        Serial.println("[PortalService] Error: /index.html not found in LittleFS!");
        server.send(404, "text/plain", "Error: /index.html not found in LittleFS! Please upload filesystem using: pio run --target uploadfs");
    }
}

void PortalService::handleNotFound() {
    if (handleFileRead(server.uri())) {
        return;
    }

    String uri = server.uri();
    if (uri == "/generate_204" || uri == "/fwlink" || uri == "/hotspot-detect.html" || 
        uri == "/ncsi.txt" || uri == "/connecttest.txt" || uri == "/canonical.html") {
        if (handleFileRead("/index.html")) return;
    }

    if (is_portal_active) {
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.send(302, "text/plain", "");
    } else {
        server.send(404, "text/plain", "Not Found");
    }
}

void PortalService::handleScan() {
    if (!is_scanning) {
        is_scanning = true;
        BaseType_t res = xTaskCreatePinnedToCore(
            scanTaskWorker,
            "WiFiScanTask",
            4096,
            NULL,
            1,
            NULL,
            0
        );
        if (res != pdPASS) {
            Serial.println("[PortalService] WiFi Scan task creation failed");
            is_scanning = false;
        }
    }
    server.send(200, "application/json", "{\"status\":\"started\",\"message\":\"Scan started\"}");
}

void PortalService::handleScanResults() {
    if (is_scanning) {
        server.send(200, "application/json", "{\"status\":\"scanning\"}");
        return;
    }

    int n = WiFi.scanComplete();
    if (n < 0) {
        server.send(200, "application/json", "{\"status\":\"not_started\"}");
        return;
    }

    String json = "{\"status\":\"complete\",\"networks\":[";
    for (int i = 0; i < n && i < 20; ++i) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"encryption\":\"" + String(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "OPEN" : "ENCRYPTED") + "\"";
        json += "}";
    }
    json += "]}";
    server.send(200, "application/json", json);
}

void PortalService::handleGetConfig() {
    AppConfig cfg = loadConfig();
    String json = "{";
    json += "\"ssid\":\"" + cfg.wifi_ssid + "\",";
    json += "\"password\":\"" + cfg.wifi_pass + "\",";
    json += "\"scriptUrl\":\"" + cfg.google_script_url + "\",";
    json += "\"cooldown\":" + String(cfg.upload_cooldown) + ",";
    json += "\"resMode\":" + String(cfg.res_mode) + ",";
    json += "\"ledEnabled\":" + String(cfg.led_enabled ? "true" : "false") + ",";
    json += "\"audioEnabled\":" + String(cfg.audio_enabled ? "true" : "false") + ",";
    json += "\"audioVolume\":" + String(cfg.audio_volume) + ",";
    json += "\"weatherCity\":\"" + cfg.weather_city + "\",";
    json += "\"lat\":" + String(cfg.weather_lat, 4) + ",";
    json += "\"lon\":" + String(cfg.weather_lon, 4) + ",";
    json += "\"standbyTimeout\":" + String(cfg.standby_timeout);
    json += "}";
    server.send(200, "application/json", json);
}

void PortalService::handleSaveConfig() {
    if (server.hasArg("plain")) {
        String body = server.arg("plain");
        Serial.printf("[PortalService] Received JSON Config: %s\n", body.c_str());

        AppConfig cfg = loadConfig();

        int idx = body.indexOf("\"ssid\":\"");
        if (idx >= 0) {
            int end = body.indexOf("\"", idx + 8);
            if (end > idx) cfg.wifi_ssid = body.substring(idx + 8, end);
        }

        idx = body.indexOf("\"password\":\"");
        if (idx >= 0) {
            int end = body.indexOf("\"", idx + 12);
            if (end > idx) cfg.wifi_pass = body.substring(idx + 12, end);
        }

        idx = body.indexOf("\"scriptUrl\":\"");
        if (idx >= 0) {
            int end = body.indexOf("\"", idx + 13);
            if (end > idx) cfg.google_script_url = body.substring(idx + 13, end);
        }

        idx = body.indexOf("\"cooldown\":");
        if (idx >= 0) {
            cfg.upload_cooldown = body.substring(idx + 11).toInt();
            if (cfg.upload_cooldown < 3) cfg.upload_cooldown = 5;
        }

        idx = body.indexOf("\"resMode\":");
        if (idx >= 0) {
            cfg.res_mode = body.substring(idx + 10).toInt();
        }

        idx = body.indexOf("\"ledEnabled\":");
        if (idx >= 0) {
            String val = body.substring(idx + 13, idx + 18);
            cfg.led_enabled = val.startsWith("true");
        }

        idx = body.indexOf("\"audioEnabled\":");
        if (idx >= 0) {
            String val = body.substring(idx + 15, idx + 20);
            cfg.audio_enabled = val.startsWith("true");
        }

        idx = body.indexOf("\"audioVolume\":");
        if (idx >= 0) {
            int vVal = body.substring(idx + 14).toInt();
            if (vVal >= 0 && vVal <= 100) cfg.audio_volume = vVal;
        }

        idx = body.indexOf("\"weatherCity\":\"");
        if (idx >= 0) {
            int end = body.indexOf("\"", idx + 15);
            if (end > idx) {
                cfg.weather_city = body.substring(idx + 15, end);
            }
        }

        idx = body.indexOf("\"lat\":");
        if (idx >= 0) {
            float latVal = body.substring(idx + 6).toFloat();
            if (latVal != 0.0f) cfg.weather_lat = latVal;
        }

        idx = body.indexOf("\"lon\":");
        if (idx >= 0) {
            float lonVal = body.substring(idx + 6).toFloat();
            if (lonVal != 0.0f) cfg.weather_lon = lonVal;
        }

        WeatherService::setLocation(cfg.weather_city.c_str(), cfg.weather_lat, cfg.weather_lon);

        idx = body.indexOf("\"standbyTimeout\":");
        if (idx >= 0) {
            int sVal = body.substring(idx + 17).toInt();
            if (sVal >= 0) cfg.standby_timeout = sVal;
        }

        saveConfig(cfg);
        server.send(200, "application/json", "{\"status\":\"ok\"}");

        delay(1000);
        ESP.restart();
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}

void PortalService::handleGetWeather() {
    WeatherInfo w = WeatherService::getWeather();
    String json = "{";
    json += "\"temp\":" + String(w.temperature, 1) + ",";
    json += "\"humidity\":" + String(w.humidity) + ",";
    json += "\"city\":\"" + w.city_name + "\",";
    json += "\"condition\":\"" + w.condition_text + "\",";
    json += "\"valid\":" + String(w.is_valid ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
}

void PortalService::setupRoutes() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/wifi/scan", HTTP_GET, handleScan);
    server.on("/api/wifi/results", HTTP_GET, handleScanResults);
    server.on("/api/config", HTTP_GET, handleGetConfig);
    server.on("/api/save", HTTP_POST, handleSaveConfig);

    // Weather API
    server.on("/api/weather", HTTP_GET, handleGetWeather);

    server.onNotFound(handleNotFound);
}

void PortalService::startCaptivePortal() {
    is_portal_active = true;

    if (!LittleFS.begin(true)) {
        Serial.println("[PortalService] Failed to mount LittleFS!");
    }

    Serial.println("\n=== [Captive Portal] Starting AP Mode ===");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("ESP32S3-CAM-AP");

    IPAddress apIP(192, 168, 4, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

    Serial.print("[Captive Portal] AP SSID: ESP32S3-CAM-AP | IP: ");
    Serial.println(WiFi.softAPIP());

    dnsServer.start(53, "*", apIP);
    setupRoutes();
    server.begin();
    is_server_started = true;
    Serial.println("[Captive Portal] Web Server & DNS Started");
}

void PortalService::startWebServer() {
    if (!LittleFS.begin(true)) {
        Serial.println("[PortalService] Failed to mount LittleFS!");
    }
    setupRoutes();
    server.begin();
    is_server_started = true;
    Serial.println("[PortalService] Web Server Started on STA WiFi");
}

void PortalService::loop() {
    if (is_portal_active) {
        dnsServer.processNextRequest();
    }
    if (is_server_started || is_portal_active) {
        server.handleClient();
    }
}
