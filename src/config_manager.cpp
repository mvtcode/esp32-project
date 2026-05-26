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
  
  // Load brightness settings
  config.brightness = preferences.getUChar("brightness", 100); // Default: 100%
  
  // Load sleep mode settings
  config.sleepEnabled = preferences.getBool("sleepEnabled", false); // Default: disabled
  config.sleepStartMinute = preferences.getUShort("sleepStart", 1320); // Default: 22:00 (1320 minutes)
  config.sleepEndMinute = preferences.getUShort("sleepEnd", 420); // Default: 07:00 (420 minutes)
  config.sleepBrightness = preferences.getUChar("sleepBright", 0); // Default: 0 (off)
  
  // Load marquee running text
  String marquee = preferences.getString("marquee", "Chào mừng bạn đến với ESP32 LED Matrix Clock!");
  marquee.toCharArray(config.marqueeText, sizeof(config.marqueeText));
  
  preferences.end();
  
  // Validate configuration
  config.isValid = isConfigValid(config);
  
  Serial.println("=== Configuration Loaded ===");
  Serial.printf("SSID: %s\n", config.ssid);
  Serial.printf("Password: %s\n", config.password[0] ? "***" : "(empty)");
  Serial.printf("Latitude: %.4f\n", config.latitude);
  Serial.printf("Longitude: %.4f\n", config.longitude);
  Serial.printf("Brightness: %d%%\n", config.brightness);
  Serial.printf("Sleep Enabled: %s\n", config.sleepEnabled ? "YES" : "NO");
  if (config.sleepEnabled) {
    Serial.printf("  Sleep Start: %02d:%02d (%d minutes from midnight)\n", 
      config.sleepStartMinute / 60, config.sleepStartMinute % 60, config.sleepStartMinute);
    Serial.printf("  Sleep End: %02d:%02d (%d minutes from midnight)\n",
      config.sleepEndMinute / 60, config.sleepEndMinute % 60, config.sleepEndMinute);
    Serial.printf("  Sleep Brightness: %d%%\n", config.sleepBrightness);
  }
  Serial.printf("Marquee: %s\n", config.marqueeText);
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
  
  // Save brightness settings
  preferences.putUChar("brightness", config.brightness);
  
  // Save sleep mode settings
  preferences.putBool("sleepEnabled", config.sleepEnabled);
  preferences.putUShort("sleepStart", config.sleepStartMinute);
  preferences.putUShort("sleepEnd", config.sleepEndMinute);
  preferences.putUChar("sleepBright", config.sleepBrightness);
  
  // Save marquee running text
  preferences.putString("marquee", String(config.marqueeText));
  
  preferences.end();
  
  Serial.println("=== Configuration Saved ===");
  Serial.printf("SSID: %s\n", config.ssid);
  Serial.printf("Latitude: %.4f\n", config.latitude);
  Serial.printf("Longitude: %.4f\n", config.longitude);
  Serial.printf("Brightness: %d%%\n", config.brightness);
  Serial.printf("Sleep Enabled: %s\n", config.sleepEnabled ? "YES" : "NO");
  if (config.sleepEnabled) {
    Serial.printf("Sleep Time: %02d:%02d - %02d:%02d\n", 
      config.sleepStartMinute / 60, config.sleepStartMinute % 60,
      config.sleepEndMinute / 60, config.sleepEndMinute % 60);
    Serial.printf("Sleep Brightness: %d%%\n", config.sleepBrightness);
  }
  Serial.printf("Marquee: %s\n", config.marqueeText);
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
