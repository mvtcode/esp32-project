#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "config_manager.h"

// Non-blocking WiFi initialization
void initWiFi(const ConfigData& config);

// Non-blocking WiFi maintenance (checks and reconnects periodically)
void maintainWiFiConnection(const ConfigData& config);

// Connect to WiFi using stored configuration (non-blocking initialization)
void connectWiFi(const ConfigData& config);

#endif
