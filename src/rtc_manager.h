#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#include <time.h>

// Initialize DS1302 RTC module
void initRTC();

// Sync RTC time from NTP (unconditional)
void syncRTCFromNTP(const struct tm& timeinfo);

// Sync RTC from NTP only if drift >= thresholdSeconds. Returns true if updated.
bool syncRTCFromNTPIfDrifted(const struct tm& ntpTime, int thresholdSeconds);

// Get time from RTC (fallback when WiFi is down)
bool getRTCTime(struct tm& timeinfo);

// Check if RTC is valid and running
bool isRTCValid();

// Check if RTC is physically present
bool isRTCPresent();

// Set RTC time using epoch timestamp (seconds since 1970)
void setRTCTime(time_t epochSeconds);

#endif
