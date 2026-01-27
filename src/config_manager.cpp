#include "config_manager.h"
#include <Preferences.h>

Preferences preferences;

void loadConfig(ConfigData &config) {
  preferences.begin("esp32config", true); // Read-only mode

  // Load WiFi settings
  String ssid = preferences.getString("ssid", "");
  ssid.toCharArray(config.ssid, sizeof(config.ssid));

  String password = preferences.getString("password", "");
  password.toCharArray(config.password, sizeof(config.password));

  // Load location
  config.latitude = preferences.getFloat("latitude", 0.0);
  config.longitude = preferences.getFloat("longitude", 0.0);

  // Load admin password
  String adminPassword = preferences.getString("adminPassword", "");
  adminPassword.toCharArray(config.adminPassword, sizeof(config.adminPassword));

  // Load display settings
  config.brightness = preferences.getUChar("brightness", 100);

  // Load sleep mode settings
  config.sleepEnabled = preferences.getBool("sleepEnabled", false);
  config.sleepHour = preferences.getUChar("sleepHour", 23);
  config.sleepMinute = preferences.getUChar("sleepMinute", 0);
  config.wakeHour = preferences.getUChar("wakeHour", 6);
  config.wakeMinute = preferences.getUChar("wakeMinute", 0);
  config.sleepBrightness = preferences.getUChar("sleepBrightness", 10);

  // Load MQTT settings (future)
  String mqttBroker = preferences.getString("mqttBroker", "");
  mqttBroker.toCharArray(config.mqttBroker, sizeof(config.mqttBroker));
  config.mqttPort = preferences.getUShort("mqttPort", 1883);
  String mqttUsername = preferences.getString("mqttUsername", "");
  mqttUsername.toCharArray(config.mqttUsername, sizeof(config.mqttUsername));
  String mqttPassword = preferences.getString("mqttPassword", "");
  mqttPassword.toCharArray(config.mqttPassword, sizeof(config.mqttPassword));
  String mqttTopic = preferences.getString("mqttTopic", "");
  mqttTopic.toCharArray(config.mqttTopic, sizeof(config.mqttTopic));

  // Load WebSocket settings (future)
  String wsServer = preferences.getString("wsServer", "");
  wsServer.toCharArray(config.wsServer, sizeof(config.wsServer));
  config.wsEnabled = preferences.getBool("wsEnabled", false);

  preferences.end();

  // Validate configuration
  config.isValid = isConfigValid(config);

  Serial.println("=== Configuration Loaded ===");
  Serial.printf("SSID: %s\n", config.ssid);
  Serial.printf("Password: %s\n", config.password[0] ? "***" : "(empty)");
  Serial.printf("Latitude: %.4f\n", config.latitude);
  Serial.printf("Longitude: %.4f\n", config.longitude);
  Serial.printf("Admin Password: %s\n",
                config.adminPassword[0] ? "***" : "(not set)");
  Serial.printf("Brightness: %d\n", config.brightness);
  Serial.printf("Sleep Mode: %s\n",
                config.sleepEnabled ? "Enabled" : "Disabled");
  if (config.sleepEnabled) {
    Serial.printf("Sleep Time: %02d:%02d - %02d:%02d (brightness: %d)\n",
                  config.sleepHour, config.sleepMinute, config.wakeHour,
                  config.wakeMinute, config.sleepBrightness);
  }
  Serial.printf("Valid: %s\n", config.isValid ? "YES" : "NO");
  Serial.println("===========================");
}

bool saveConfig(const ConfigData &config) {
  if (!isConfigValid(config)) {
    Serial.println("ERROR: Cannot save invalid configuration");
    return false;
  }

  preferences.begin("esp32config", false); // Read-write mode

  // Save WiFi settings
  preferences.putString("ssid", String(config.ssid));
  preferences.putString("password", String(config.password));

  // Save location
  preferences.putFloat("latitude", config.latitude);
  preferences.putFloat("longitude", config.longitude);

  // Save admin password
  preferences.putString("adminPassword", String(config.adminPassword));

  // Save display settings
  preferences.putUChar("brightness", config.brightness);

  // Save sleep mode settings
  preferences.putBool("sleepEnabled", config.sleepEnabled);
  preferences.putUChar("sleepHour", config.sleepHour);
  preferences.putUChar("sleepMinute", config.sleepMinute);
  preferences.putUChar("wakeHour", config.wakeHour);
  preferences.putUChar("wakeMinute", config.wakeMinute);
  preferences.putUChar("sleepBrightness", config.sleepBrightness);

  // Save MQTT settings (future)
  preferences.putString("mqttBroker", String(config.mqttBroker));
  preferences.putUShort("mqttPort", config.mqttPort);
  preferences.putString("mqttUsername", String(config.mqttUsername));
  preferences.putString("mqttPassword", String(config.mqttPassword));
  preferences.putString("mqttTopic", String(config.mqttTopic));

  // Save WebSocket settings (future)
  preferences.putString("wsServer", String(config.wsServer));
  preferences.putBool("wsEnabled", config.wsEnabled);

  preferences.end();

  Serial.println("=== Configuration Saved ===");
  Serial.printf("SSID: %s\n", config.ssid);
  Serial.printf("Latitude: %.4f\n", config.latitude);
  Serial.printf("Longitude: %.4f\n", config.longitude);
  Serial.printf("Brightness: %d\n", config.brightness);
  Serial.printf("Sleep Mode: %s\n",
                config.sleepEnabled ? "Enabled" : "Disabled");
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
