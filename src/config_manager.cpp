#include "config_manager.h"
#include "debug.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

ClockConfig systemConfig = {
  "", // ssid
  7,  // timezone
  3,  // brightness
  true // is24h
};

bool initConfig() {
  if (!LittleFS.begin()) {
    DEBUG_PRINTLN("[FS] Failed to mount LittleFS!");
    return false;
  }
  DEBUG_PRINTLN("[FS] LittleFS mounted successfully.");
  return true;
}

bool loadConfig() {
  if (!LittleFS.exists("/config.json")) {
    DEBUG_PRINTLN("[Config] File does not exist, using defaults.");
    return false;
  }
  
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    DEBUG_PRINTLN("[Config] Failed to open config for reading.");
    return false;
  }
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();
  
  if (error) {
    DEBUG_PRINTLN("[Config] Failed to parse config JSON.");
    return false;
  }
  
  if (doc.containsKey("ssid")) {
    strlcpy(systemConfig.ssid, doc["ssid"] | "", sizeof(systemConfig.ssid));
  }
  systemConfig.timezone = doc["timezone"] | 7;
  systemConfig.brightness = doc["brightness"] | 3;
  systemConfig.is24h = doc["is24h"] | true;
  
  DEBUG_PRINTLN("[Config] Config loaded successfully:");
  DEBUG_PRINTF("  SSID: %s\n", systemConfig.ssid);
  DEBUG_PRINTF("  Timezone: %d\n", systemConfig.timezone);
  DEBUG_PRINTF("  Brightness: %d\n", systemConfig.brightness);
  DEBUG_PRINTF("  24h format: %s\n", systemConfig.is24h ? "true" : "false");
  
  return true;
}

bool saveConfig() {
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    DEBUG_PRINTLN("[Config] Failed to open config for writing.");
    return false;
  }
  
  JsonDocument doc;
  doc["ssid"] = WiFi.SSID();
  doc["timezone"] = systemConfig.timezone;
  doc["brightness"] = systemConfig.brightness;
  doc["is24h"] = systemConfig.is24h;
  
  if (serializeJson(doc, configFile) == 0) {
    DEBUG_PRINTLN("[Config] Failed to write config to file.");
    configFile.close();
    return false;
  }
  
  configFile.close();
  DEBUG_PRINTLN("[Config] Config saved successfully.");
  return true;
}
