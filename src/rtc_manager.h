#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#include <time.h>

// Initialize DS1302 RTC module
void initRTC();

// Sync RTC time from NTP
void syncRTCFromNTP(const struct tm& timeinfo);

// Get time from RTC (fallback when WiFi is down)
bool getRTCTime(struct tm& timeinfo);

// Check if RTC is valid and running
bool isRTCValid();

#endif
