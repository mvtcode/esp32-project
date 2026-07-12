#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "config_manager.h"
#include <WiFi.h>

// Connect to WiFi using stored configuration with retry mechanism
void connectWiFi(const ConfigData& config);

// Hiển thị IP address của đồng hồ lên màn hình matrix
void showIPAddress(IPAddress ip);

#endif
