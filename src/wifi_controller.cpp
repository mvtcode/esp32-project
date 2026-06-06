#include "wifi_controller.h"
#include "config_manager.h"
#include "led_indicator.h"
#include "debug.h"
#include <ESP8266WiFi.h>
#include <WiFiManager.h>

// Hook to update LED Matrix display in Phase 2
#ifndef STATUS_LED_PIN
extern void updateDisplayMessage(const char* msg);
#endif

static bool shouldSaveWiFiConfig = false;

static void saveConfigCallback() {
  DEBUG_PRINTLN("[WiFi] Should save config triggered");
  shouldSaveWiFiConfig = true;
}

static void configModeCallback(WiFiManager *myWiFiManager) {
  DEBUG_PRINTLN("[WiFi] Entered config mode");
  DEBUG_PRINTF("  AP SSID: %s\n", myWiFiManager->getConfigPortalSSID().c_str());
  DEBUG_PRINTF("  AP IP: %s\n", WiFi.softAPIP().toString().c_str());
  
  #ifdef STATUS_LED_PIN
    setLedMode(LED_MODE_FAST);
  #else
    updateDisplayMessage("SET");
  #endif
}

void initWiFi() {
  WiFiManager wm;
  
  wm.setAPCallback(configModeCallback);
  wm.setSaveConfigCallback(saveConfigCallback);
  
  // Custom portal parameters
  char tzStr[6];
  snprintf(tzStr, sizeof(tzStr), "%d", systemConfig.timezone);
  WiFiManagerParameter custom_timezone("timezone", "Timezone (GMT Offset)", tzStr, 5);
  
  char brightStr[4];
  snprintf(brightStr, sizeof(brightStr), "%d", systemConfig.brightness);
  WiFiManagerParameter custom_brightness("brightness", "Brightness (0-15)", brightStr, 3);
  
  WiFiManagerParameter custom_is24h("is24h", "24h Format (1=Yes, 0=No)", systemConfig.is24h ? "1" : "0", 2);
  
  wm.addParameter(&custom_timezone);
  wm.addParameter(&custom_brightness);
  wm.addParameter(&custom_is24h);
  
  DEBUG_PRINTLN("[WiFi] Attempting to connect...");
  
  #ifdef STATUS_LED_PIN
    setLedMode(LED_MODE_SLOW);
  #else
    updateDisplayMessage("CONN");
  #endif
  
  wm.setConfigPortalTimeout(180);
  
  if (!wm.autoConnect("ESPClock-Setup")) {
    DEBUG_PRINTLN("[WiFi] Failed to connect and hit timeout. Restarting...");
    #ifndef STATUS_LED_PIN
      updateDisplayMessage("FAIL");
    #endif
    delay(3000);
    ESP.restart();
  }
  
  DEBUG_PRINTLN("[WiFi] Connected successfully!");
  DEBUG_PRINTF("  IP Address: %s\n", WiFi.localIP().toString().c_str());
  
  if (shouldSaveWiFiConfig) {
    systemConfig.timezone = atoi(custom_timezone.getValue());
    systemConfig.brightness = atoi(custom_brightness.getValue());
    systemConfig.is24h = (atoi(custom_is24h.getValue()) == 1);
    saveConfig();
  }
  
  #ifdef STATUS_LED_PIN
    setLedMode(LED_MODE_ON);
  #else
    updateDisplayMessage("OK");
  #endif
}

static unsigned long lastWiFiCheckTime = 0;

void handleWiFiConnection() {
  if (millis() - lastWiFiCheckTime > 10000) {
    lastWiFiCheckTime = millis();
    if (WiFi.status() != WL_CONNECTED) {
      DEBUG_PRINTLN("[WiFi] Connection lost! Reconnecting...");
      #ifdef STATUS_LED_PIN
        setLedMode(LED_MODE_SLOW);
      #else
        updateDisplayMessage("CONN");
      #endif
      WiFi.reconnect();
    } 
#ifdef STATUS_LED_PIN
    else if (digitalRead(STATUS_LED_PIN) != LOW && !isLedActive()) {
      setLedMode(LED_MODE_ON);
    }
#endif
  }
}
