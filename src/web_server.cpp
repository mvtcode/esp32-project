#include "web_server.h"
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <SPIFFS.h>
#include <WiFi.h>


AsyncWebServer *server = nullptr;
DNSServer dnsServer;
ConfigData globalConfig; // Global reference to config for authentication

// Helper function: HTTP Basic Auth
bool authenticateRequest(AsyncWebServerRequest *request,
                         const String &adminPassword) {
  // Skip auth if admin password not set yet
  if (adminPassword.length() == 0) {
    return true;
  }

  // Use AsyncWebServer's built-in authentication
  return request->authenticate("admin", adminPassword.c_str());
}
// Setup APSTA Mode (AP + STA simultaneously)
void setupAPSTAMode() {
  Serial.println("=== Setting up AP+STA Mode ===");

  // Set mode to AP+STA
  WiFi.mode(WIFI_AP_STA);
  delay(100);

  // Configure AP mode
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);

  // Start AP
  bool apResult = WiFi.softAP("Clock-2026", "", 1, false, 4);

  if (apResult) {
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    Serial.println("SSID: Clock-2026");
    Serial.println("Password: (none)");
  } else {
    Serial.println("ERROR: Failed to start AP!");
  }

  // If we have WiFi config, try to connect to STA
  if (globalConfig.isValid && strlen(globalConfig.ssid) > 0) {
    Serial.println("Attempting to connect to WiFi as STA...");
    WiFi.begin(globalConfig.ssid, globalConfig.password);

    // Non-blocking: will auto-reconnect in background
    Serial.printf("Connecting to SSID: %s\n", globalConfig.ssid);
  } else {
    Serial.println("No WiFi config found, STA mode not started");
  }

  Serial.println("==============================");
}

void setupCaptivePortal() {
  const byte DNS_PORT = 53;
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  Serial.println("DNS Server started for Captive Portal");
}

void handleDNS() { dnsServer.processNextRequest(); }

void setupWebServer() {
  // Initialize SPIFFS
  Serial.println("Initializing SPIFFS...");
  if (!SPIFFS.begin(true)) {
    Serial.println("ERROR: Failed to mount SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully!");

  // List files
  Serial.println("Files in SPIFFS:");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.printf("  - %s (%d bytes)\n", file.name(), file.size());
    file = root.openNextFile();
  }

  // Create server instance
  server = new AsyncWebServer(80);

  // ==========================================
  // ROOT ENDPOINT: Dual routing based on client IP
  // ==========================================
  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    IPAddress clientIP = request->client()->localIP();
    IPAddress apIP = WiFi.softAPIP();

    Serial.printf("HTTP GET / from %s\n", clientIP.toString().c_str());

    // Check if client connected via AP (192.168.4.x)
    if (clientIP[0] == apIP[0] && clientIP[1] == apIP[1] &&
        clientIP[2] == apIP[2]) {
      // AP Mode client → Serve index.html (first-time setup)
      Serial.println("Serving index.html (AP Mode)");

      if (!SPIFFS.exists("/index.html")) {
        request->send(500, "text/plain",
                      "index.html not found. Run: pio run --target uploadfs");
        return;
      }

      request->send(SPIFFS, "/index.html", "text/html");
    } else {
      // STA Mode client → Serve config_sta.html (with auth)
      Serial.println("Serving config_sta.html (STA Mode)");

      if (!authenticateRequest(request, String(globalConfig.adminPassword))) {
        return request->requestAuthentication("ESP32 Clock", false);
      }

      if (!SPIFFS.exists("/config_sta.html")) {
        // Fallback to index.html if config_sta.html doesn't exist yet
        Serial.println(
            "WARNING: config_sta.html not found, falling back to index.html");
        request->send(SPIFFS, "/index.html", "text/html");
        return;
      }

      request->send(SPIFFS, "/config_sta.html", "text/html");
    }
  });

  // ==========================================
  // Captive Portal Detection Endpoints
  // ==========================================
  server->on("/generate_204", HTTP_GET,
             [](AsyncWebServerRequest *request) { request->redirect("/"); });

  server->on("/hotspot-detect.html", HTTP_GET,
             [](AsyncWebServerRequest *request) { request->redirect("/"); });

  server->on("/connecttest.txt", HTTP_GET,
             [](AsyncWebServerRequest *request) { request->redirect("/"); });

  // ==========================================
  // API: WiFi Scan (available to both AP and STA)
  // ==========================================
  server->on("/api/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("API /api/wifi - Scanning WiFi networks...");

    int n = WiFi.scanNetworks();
    Serial.printf("Found %d networks\n", n);

    JsonDocument doc;
    JsonArray networks = doc["networks"].to<JsonArray>();

    for (int i = 0; i < n; i++) {
      JsonObject network = networks.add<JsonObject>();
      network["ssid"] = WiFi.SSID(i);
      network["rssi"] = WiFi.RSSI(i);
      network["encryption"] =
          (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "OPEN" : "ENCRYPTED";
    }

    String response;
    serializeJson(doc, response);

    request->send(200, "application/json", response);
  });

  // ==========================================
  // API: Save Initial Config (AP Mode only)
  // ==========================================
  server->on(
      "/api/save", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len,
         size_t index, size_t total) {
        Serial.printf("API /api/save - Received %d bytes\n", len);

        JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len");
      
      if (error) {
      Serial.printf("JSON parse error: %s\n", error.c_str());
      request->send(400, "application/json",
                    "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
      return;
      }

      // Extract config
      ConfigData newConfig;
      memset(&newConfig, 0, sizeof(ConfigData));

      if (doc.containsKey("ssid")) {
      String ssid = doc["ssid"].as<String>();
      ssid.toCharArray(newConfig.ssid, sizeof(newConfig.ssid));
      }

      if (doc.containsKey("password")) {
      String password = doc["password"].as<String>();
      password.toCharArray(newConfig.password, sizeof(newConfig.password));
      }

      if (doc.containsKey("adminPassword")) {
      String adminPassword = doc["adminPassword"].as<String>();
      adminPassword.toCharArray(newConfig.adminPassword,
                                sizeof(newConfig.adminPassword));
      }

      newConfig.latitude = doc["latitude"] | 0.0;
      newConfig.longitude = doc["longitude"] | 0.0;

      // Set defaults for new fields
      newConfig.brightness = 100;
      newConfig.sleepEnabled = false;
      newConfig.sleepHour = 23;
      newConfig.sleepMinute = 0;
      newConfig.wakeHour = 6;
      newConfig.wakeMinute = 0;
      newConfig.sleepBrightness = 10;

      // Validate
      if (!isConfigValid(newConfig)) {
      Serial.println("ERROR: Invalid configuration");
      request->send(400, "application/json",
                    "{\"status\":\"error\",\"message\":\"Invalid config\"}");
      return;
      }

      // Save and restart
      if (saveConfig(newConfig)) {
      Serial.println("Configuration saved!");
      request->send(200, "application/json", "{\"status\":\"success\"}");

      delay(2000);
      ESP.restart();
      } else {
      request->send(500, "application/json",
                    "{\"status\":\"error\",\"message\":\"Save failed\"}");
      }
      });

  // ==========================================
  // API: Get Current Config (STA Mode with auth)
  // ==========================================
  server->on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!authenticateRequest(request, String(globalConfig.adminPassword))) {
      return request->requestAuthentication("ESP32 Clock", false);
    }

    JsonDocument doc;
    doc["ssid"] = globalConfig.ssid;
    doc["latitude"] = globalConfig.latitude;
    doc["longitude"] = globalConfig.longitude;
    doc["brightness"] = globalConfig.brightness;
    doc["sleepEnabled"] = globalConfig.sleepEnabled;
    doc["sleepHour"] = globalConfig.sleepHour;
    doc["sleepMinute"] = globalConfig.sleepMinute;
    doc["wakeHour"] = globalConfig.wakeHour;
    doc["wakeMinute"] = globalConfig.wakeMinute;
    doc["sleepBrightness"] = globalConfig.sleepBrightness;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // ==========================================
  // API: Update Brightness (STA Mode with auth)
  // ==========================================
  server->on(
      "/api/brightness", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len,
         size_t index, size_t total) {
        if (!authenticateRequest(request, String(globalConfig.adminPassword))) {
          return request->requestAuthentication("ESP32 Clock", false);
        }

        JsonDocument doc;
        deserializeJson(doc, data, len);

        if (doc.containsKey("brightness")) {
          uint8_t brightness = doc["brightness"];
          globalConfig.brightness = brightness;

          // Apply immediately (will be handled in main.cpp)
          extern void updateBrightnessRuntime(uint8_t brightness);
          updateBrightnessRuntime(brightness);

          saveConfig(globalConfig);

          request->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
          request->send(400, "application/json", "{\"status\":\"error\"}");
        }
      });

  // ==========================================
  // API: Update Sleep Settings (STA Mode with auth)
  // ==========================================
  server->on(
      "/api/sleep", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len,
         size_t index, size_t total) {
        if (!authenticateRequest(request, String(globalConfig.adminPassword))) {
          return request->requestAuthentication("ESP32 Clock", false);
        }

        JsonDocument doc;
        deserializeJson(doc, data, len);

        globalConfig.sleepEnabled = doc["sleepEnabled"] | false;
        globalConfig.sleepHour = doc["sleepHour"] | 23;
        globalConfig.sleepMinute = doc["sleepMinute"] | 0;
        globalConfig.wakeHour = doc["wakeHour"] | 6;
        globalConfig.wakeMinute = doc["wakeMinute"] | 0;
        globalConfig.sleepBrightness = doc["sleepBrightness"] | 10;

        saveConfig(globalConfig);
        request->send(200, "application/json", "{\"status\":\"success\"}");
      });

  // ==========================================
  // API: Update Admin Password (STA Mode with auth)
  // ==========================================
  server->on(
      "/api/admin-password", HTTP_POST, [](AsyncWebServerRequest *request) {},
      NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len,
         size_t index, size_t total) {
        if (!authenticateRequest(request, String(globalConfig.adminPassword))) {
          return request->requestAuthentication("ESP32 Clock", false);
        }

        JsonDocument doc;
        deserializeJson(doc, data, len);

        if (doc.containsKey("newPassword")) {
          String newPassword = doc["newPassword"].as<String>();
          if (newPassword.length() >= 6) {
            newPassword.toCharArray(globalConfig.adminPassword,
                                    sizeof(globalConfig.adminPassword));
            saveConfig(globalConfig);
            request->send(200, "application/json", "{\"status\":\"success\"}");
          } else {
            request->send(
                400, "application/json",
                "{\"status\":\"error\",\"message\":\"Password too short\"}");
          }
        } else {
          request->send(400, "application/json", "{\"status\":\"error\"}");
        }
      });

  // Start server
  server->begin();
  Serial.println("Web server started on port 80");
  Serial.println("AP Mode: http://192.168.4.1/");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("STA Mode: http://%s/\n", WiFi.localIP().toString().c_str());
  }
}

void stopWebServer() {
  if (server) {
    server->end();
    delete server;
    server = nullptr;
    Serial.println("Web server stopped");
  }
}
