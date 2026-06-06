#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>

struct ClockConfig {
  char ssid[33];
  int timezone;
  int brightness;
  bool is24h;
};

extern ClockConfig systemConfig;

bool initConfig();
bool loadConfig();
bool saveConfig();

#endif
