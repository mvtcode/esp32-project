#include "rtc_manager.h"
#include <Arduino.h>
#include <RtcDS1302.h>
#include <ThreeWire.h>

// DS1302 pins
#define RTC_CLK_PIN 32
#define RTC_DAT_PIN 33
#define RTC_RST_PIN 2   // Changed from 34 (input-only) to 2

// DS1302 connection
ThreeWire myWire(RTC_DAT_PIN, RTC_CLK_PIN, RTC_RST_PIN);
RtcDS1302<ThreeWire> Rtc(myWire);

// Initialize DS1302 RTC module
void initRTC() {
  Serial.println("Initializing DS1302 RTC...");
  
  Rtc.Begin();
  
  // Check if RTC is write protected
  if (Rtc.GetIsWriteProtected()) {
    Serial.println("RTC was write protected, enabling writing now");
    Rtc.SetIsWriteProtected(false);
  }
  
  // Check if RTC is running
  if (!Rtc.GetIsRunning()) {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }
  
  // Get current RTC time
  RtcDateTime now = Rtc.GetDateTime();
  
  if (!now.IsValid()) {
    Serial.println("RTC lost confidence in the DateTime!");
  }
}

// Sync RTC time from NTP
void syncRTCFromNTP(const struct tm& timeinfo) {
  // Convert tm struct to RtcDateTime
  RtcDateTime compiled = RtcDateTime(
    timeinfo.tm_year + 1900,
    timeinfo.tm_mon + 1,
    timeinfo.tm_mday,
    timeinfo.tm_hour,
    timeinfo.tm_min,
    timeinfo.tm_sec
  );
  
  Rtc.SetDateTime(compiled);
}

// Get time from RTC (fallback when WiFi is down)
bool getRTCTime(struct tm& timeinfo) {
  RtcDateTime now = Rtc.GetDateTime();
  
  if (!now.IsValid()) {
    Serial.println("RTC DateTime is not valid!");
    return false;
  }
  
  // Convert RtcDateTime to tm struct
  timeinfo.tm_year = now.Year() - 1900;
  timeinfo.tm_mon = now.Month() - 1;
  timeinfo.tm_mday = now.Day();
  timeinfo.tm_hour = now.Hour();
  timeinfo.tm_min = now.Minute();
  timeinfo.tm_sec = now.Second();
  timeinfo.tm_wday = now.DayOfWeek(); // 0 = Sunday
  timeinfo.tm_yday = -1; // Not available from RTC
  timeinfo.tm_isdst = 0;
  
  return true;
}

// Check if RTC is valid and running
bool isRTCValid() {
  RtcDateTime now = Rtc.GetDateTime();
  return now.IsValid() && Rtc.GetIsRunning();
}
