#include "wifi_manager.h"
#include "display.h"
#include "reset_button.h"
#include <Arduino.h>
#include <WiFi.h>

static unsigned long lastReconnectAttempt = 0;
static bool isWiFiConnected = false;

// Initialize WiFi connection in background (non-blocking)
void initWiFi(const ConfigData& config) {
  Serial.println("Initializing non-blocking WiFi STA mode...");
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  
  // Configure NTP time servers
  configTime(7 * 3600, 0, "vn.pool.ntp.org", "pool.ntp.org", "time.google.com");
  
  Serial.print("Connecting to SSID: ");
  Serial.println(config.ssid);
  WiFi.begin(config.ssid, config.password);
  
  lastReconnectAttempt = millis();
}

// Periodically maintain WiFi connection (non-blocking)
void maintainWiFiConnection(const ConfigData& config) {
  wl_status_t status = WiFi.status();
  
  if (status == WL_CONNECTED) {
    if (!isWiFiConnected) {
      Serial.println("\n>>> WiFi Connected! <<<");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      isWiFiConnected = true;
    }
  } else {
    if (isWiFiConnected) {
      Serial.println("\n>>> WiFi Disconnected! Will attempt reconnection periodically <<<");
      isWiFiConnected = false;
    }
    
    // Attempt reconnection every 30 seconds if disconnected
    unsigned long now = millis();
    if (now - lastReconnectAttempt >= 30000) {
      lastReconnectAttempt = now;
      Serial.println("Periodic WiFi reconnect attempt...");
      WiFi.disconnect();
      WiFi.begin(config.ssid, config.password);
    }
  }
}

// Connect WiFi wrapper (non-blocking)
void connectWiFi(const ConfigData& config) {
  initWiFi(config);
}
