#include "web_server.h"
#include <WiFi.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

AsyncWebServer* server = nullptr;

void setupAPMode() {
  Serial.println("=== Setting up Access Point ===");
  
  // Disconnect from any existing WiFi connection
  WiFi.disconnect();
  delay(100);
  
  // Configure AP mode
  WiFi.mode(WIFI_AP);
  
  // Configure static IP for AP
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  // Set AP configuration
  WiFi.softAPConfig(local_IP, gateway, subnet);
  
  // Start AP with better compatibility settings
  // Parameters: SSID, password, channel, hidden, max_connection
  bool result = WiFi.softAP(
    "Clock-2026",  // SSID
    "",                     // No password (empty string)
    1,                      // Channel 1 (most compatible)
    false,                  // Not hidden
    4                       // Max 4 connections
  );
  
  if (result) {
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    Serial.println("SSID: Clock-2026");
    Serial.println("Password: (none)");
    Serial.println("Channel: 1");
    Serial.println("Gateway: " + gateway.toString());
    Serial.println("Subnet: " + subnet.toString());
    Serial.printf("Connected clients: %d\n", WiFi.softAPgetStationNum());
    Serial.println("==============================");
  } else {
    Serial.println("ERROR: Failed to start AP!");
  }
}

void setupWebServer() {
  // Initialize SPIFFS
  Serial.println("Initializing SPIFFS...");
  if (!SPIFFS.begin(true)) {
    Serial.println("ERROR: Failed to mount SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully!");
  
  // List files in SPIFFS for debugging
  Serial.println("Files in SPIFFS:");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.print("  - ");
    Serial.print(file.name());
    Serial.print(" (");
    Serial.print(file.size());
    Serial.println(" bytes)");
    file = root.openNextFile();
  }
  
  // Check if index.html exists
  if (SPIFFS.exists("/index.html")) {
    Serial.println("✓ /index.html found in SPIFFS");
  } else {
    Serial.println("✗ /index.html NOT FOUND in SPIFFS!");
    Serial.println("Please run: pio run --target uploadfs");
  }
  
  // Create web server instance
  server = new AsyncWebServer(80);
  
  // Serve the configuration page at root
  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("HTTP GET / - Serving index.html");
    
    // Read file content
    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
      Serial.println("ERROR: Failed to open /index.html");
      request->send(500, "text/plain", "Failed to open index.html");
      return;
    }
    
    String html = file.readString();
    file.close();
    
    Serial.printf("Sending HTML (%d bytes)\n", html.length());
    
    // Send with explicit headers
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=UTF-8", html);
    response->addHeader("Cache-Control", "no-cache");
    response->addHeader("Connection", "close");
    request->send(response);
    
    Serial.println("Response sent!");
  });
  
  // Also serve at /index.html explicitly
  server->on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("HTTP GET /index.html - Serving index.html");
    
    // Read file content
    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
      Serial.println("ERROR: Failed to open /index.html");
      request->send(500, "text/plain", "Failed to open index.html");
      return;
    }
    
    String html = file.readString();
    file.close();
    
    Serial.printf("Sending HTML (%d bytes)\n", html.length());
    
    // Send with explicit headers
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=UTF-8", html);
    response->addHeader("Cache-Control", "no-cache");
    response->addHeader("Connection", "close");
    request->send(response);
    
    Serial.println("Response sent!");
  });
  
  // API endpoint to save configuration
  server->on("/api/save", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
    // Handle preflight CORS request
    AsyncWebServerResponse *response = request->beginResponse(200);
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
    request->send(response);
  });
  
  server->on("/api/save", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      Serial.printf("API /api/save called - Received %d bytes\n", len);
      
      // Parse JSON payload
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);
      
      if (error) {
        Serial.print("JSON parse error: ");
        Serial.println(error.c_str());
        
        AsyncWebServerResponse *response = request->beginResponse(400, "application/json", 
          "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
        return;
      }
      
      // Extract configuration data
      ConfigData newConfig;
      
      if (doc.containsKey("ssid")) {
        String ssid = doc["ssid"].as<String>();
        ssid.toCharArray(newConfig.ssid, sizeof(newConfig.ssid));
        Serial.printf("SSID: %s\n", newConfig.ssid);
      } else {
        newConfig.ssid[0] = '\0';
      }
      
      if (doc.containsKey("password")) {
        String password = doc["password"].as<String>();
        password.toCharArray(newConfig.password, sizeof(newConfig.password));
        Serial.println("Password: ***");
      } else {
        newConfig.password[0] = '\0';
      }
      
      newConfig.latitude = doc["latitude"] | 0.0;
      newConfig.longitude = doc["longitude"] | 0.0;
      Serial.printf("Coordinates: %.4f, %.4f\n", newConfig.latitude, newConfig.longitude);
      
      // Validate and save
      if (!isConfigValid(newConfig)) {
        Serial.println("ERROR: Invalid configuration received");
        
        AsyncWebServerResponse *response = request->beginResponse(400, "application/json", 
          "{\"status\":\"error\",\"message\":\"Invalid configuration\"}");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
        return;
      }
      
      if (saveConfig(newConfig)) {
        Serial.println("Configuration saved successfully!");
        
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", 
          "{\"status\":\"success\",\"message\":\"Configuration saved\"}");
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Connection", "close");
        request->send(response);
        
        Serial.println("Restarting in 2 seconds...");
        // Restart ESP32 after 2 seconds
        delay(2000);
        ESP.restart();
      } else {
        request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to save\"}");
      }
    });
  
  // Start the server
  server->begin();
  Serial.println("Web server started on port 80");
}

void stopWebServer() {
  if (server) {
    server->end();
    delete server;
    server = nullptr;
    Serial.println("Web server stopped");
  }
}
