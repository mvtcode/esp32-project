#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "config_manager.h"

// Connect to WiFi using stored configuration with retry mechanism
void connectWiFi(const ConfigData& config);

#endif
