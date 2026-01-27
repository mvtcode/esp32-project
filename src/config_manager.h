#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>

// Configuration data structure
struct ConfigData {
  // WiFi settings
  char ssid[64];
  char password[64];

  // Location for weather
  float latitude;
  float longitude;

  // Admin authentication (for STA mode web config)
  char adminPassword[64];

  // Display settings
  uint8_t brightness; // LED brightness 0-255, default 100

  // Sleep mode settings
  bool sleepEnabled;       // Enable/disable sleep mode
  uint8_t sleepHour;       // Sleep start hour (0-23)
  uint8_t sleepMinute;     // Sleep start minute (0-59)
  uint8_t wakeHour;        // Wake hour (0-23)
  uint8_t wakeMinute;      // Wake minute (0-59)
  uint8_t sleepBrightness; // Brightness during sleep (0=off, default 10)

  // MQTT settings (future support - v2.3.x)
  char mqttBroker[64];
  uint16_t mqttPort;
  char mqttUsername[32];
  char mqttPassword[64];
  char mqttTopic[64];

  // WebSocket settings (future support - v2.3.x)
  char wsServer[128];
  bool wsEnabled;

  // Validation flag
  bool isValid;
};

// Configuration management functions
void loadConfig(ConfigData &config);
bool saveConfig(const ConfigData &config);
void clearConfig();
bool isConfigValid(const ConfigData &config);

#endif // CONFIG_MANAGER_H
