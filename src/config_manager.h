#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>

// Configuration data structure
struct ConfigData {
  char ssid[64];
  char password[64];
  float latitude;
  float longitude;
  bool isValid;
  
  // Display settings
  uint8_t brightness;           // Screen brightness: 10-100% (default: 100)
  
  // Sleep mode settings
  bool sleepEnabled;            // Enable/disable sleep mode (default: false)
  uint16_t sleepStartMinute;    // Sleep start time in minutes from midnight 0-1439 (default: 1320 = 22:00)
  uint16_t sleepEndMinute;      // Sleep end time in minutes from midnight 0-1439 (default: 420 = 07:00)
  uint8_t sleepBrightness;      // Brightness during sleep 0-100%, 0=off (default: 0)
  char marqueeText[256];        // Custom marquee running text
};

// Configuration management functions
void loadConfig(ConfigData &config);
bool saveConfig(const ConfigData &config);
void clearConfig();
bool isConfigValid(const ConfigData &config);

#endif // CONFIG_MANAGER_H
