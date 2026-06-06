#include "ntp_manager.h"
#include "config_manager.h"
#include "debug.h"
#include <time.h>

void initNTP() {
  DEBUG_PRINTF("[NTP] Initializing NTP (TZ: %d)\n", systemConfig.timezone);
  configTime(systemConfig.timezone * 3600, 0, "pool.ntp.org", "time.google.com");
}

bool isTimeSynced() {
  time_t now = time(nullptr);
  return now > 1577836800; // After Jan 1 2020
}

void printLocalTime() {
  char buf[64];
  getLocalTimeStr(buf, sizeof(buf));
  DEBUG_PRINTF("[Time] %s\n", buf);
}

void getLocalTimeStr(char* buffer, size_t maxLen) {
  time_t now = time(nullptr);
  struct tm* timeInfo = localtime(&now);
  strftime(buffer, maxLen, "%Y-%m-%d %H:%M:%S", timeInfo);
}
