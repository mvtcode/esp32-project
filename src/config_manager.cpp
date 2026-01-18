#include "config_manager.h"
#include <Preferences.h>

Preferences preferences;

void loadConfig(ConfigData &config) {
  preferences.begin("esp32config", true); // Read-only mode
  
  // Load SSID
  String ssid = preferences.getString("ssid", "");
  ssid.toCharArray(config.ssid, sizeof(config.ssid));
  
  // Load password
  String password = preferences.getString("password", "");
  password.toCharArray(config.password, sizeof(config.password));
  
  // Load latitude and longitude
  config.latitude = preferences.getFloat("latitude", 0.0);
  config.longitude = preferences.getFloat("longitude", 0.0);
  
  preferences.end();
  
  // Validate configuration
  config.isValid = isConfigValid(config);
  
  Serial.println("=== Configuration Loaded ===");
  Serial.printf("SSID: %s\n", config.ssid);
  Serial.printf("Password: %s\n", config.password[0] ? "***" : "(empty)");
  Serial.printf("Latitude: %.4f\n", config.latitude);
  Serial.printf("Longitude: %.4f\n", config.longitude);
  Serial.printf("Valid: %s\n", config.isValid ? "YES" : "NO");
  Serial.println("===========================");
}

bool saveConfig(const ConfigData &config) {
  if (!isConfigValid(config)) {
    Serial.println("ERROR: Cannot save invalid configuration");
    return false;
  }
  
  preferences.begin("esp32config", false); // Read-write mode
  
  preferences.putString("ssid", String(config.ssid));
  preferences.putString("password", String(config.password));
  preferences.putFloat("latitude", config.latitude);
  preferences.putFloat("longitude", config.longitude);
  
  preferences.end();
  
  Serial.println("=== Configuration Saved ===");
  Serial.printf("SSID: %s\n", config.ssid);
  Serial.printf("Latitude: %.4f\n", config.latitude);
  Serial.printf("Longitude: %.4f\n", config.longitude);
  Serial.println("==========================");
  
  return true;
}

void clearConfig() {
  preferences.begin("esp32config", false); // Read-write mode
  preferences.clear();
  preferences.end();
  
  Serial.println("=== Configuration Cleared ===");
}

bool isConfigValid(const ConfigData &config) {
  // Check if SSID is not empty
  if (strlen(config.ssid) == 0) {
    return false;
  }
  
  // Check if coordinates are valid (not zero)
  if (config.latitude == 0.0 && config.longitude == 0.0) {
    return false;
  }
  
  // Check if coordinates are in valid range
  if (config.latitude < -90.0 || config.latitude > 90.0) {
    return false;
  }
  
  if (config.longitude < -180.0 || config.longitude > 180.0) {
    return false;
  }
  
  return true;
}
