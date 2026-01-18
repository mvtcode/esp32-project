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
};

// Configuration management functions
void loadConfig(ConfigData &config);
bool saveConfig(const ConfigData &config);
void clearConfig();
bool isConfigValid(const ConfigData &config);

#endif // CONFIG_MANAGER_H
