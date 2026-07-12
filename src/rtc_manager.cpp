#include "rtc_manager.h"
#include <Arduino.h>
#include <RtcDS1302.h>
#include <ThreeWire.h>

// DS1302 pins (explicitly configured to avoid conflicts on ESP32-S3-N16R8)
#define RTC_CLK_PIN 11
#define RTC_DAT_PIN 12
#define RTC_RST_PIN 13

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
  } else {
    Serial.printf("RTC initialized. Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
                  now.Year(), now.Month(), now.Day(),
                  now.Hour(), now.Minute(), now.Second());
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
  Serial.printf("RTC synced from NTP: %04d-%02d-%02d %02d:%02d:%02d\n",
                compiled.Year(), compiled.Month(), compiled.Day(),
                compiled.Hour(), compiled.Minute(), compiled.Second());
}

// Sync RTC from NTP only if the time differs by more than 'thresholdSeconds'
// Returns true if RTC was updated
bool syncRTCFromNTPIfDrifted(const struct tm& ntpTime, int thresholdSeconds) {
  RtcDateTime rtcNow = Rtc.GetDateTime();
  if (!rtcNow.IsValid()) {
    Serial.println("RTC not valid, syncing unconditionally from NTP.");
    syncRTCFromNTP(ntpTime);
    return true;
  }

  // Build a unix-like timestamp for NTP time
  // Use mktime on the ntpTime struct to get epoch seconds
  struct tm ntpCopy = ntpTime;
  time_t ntpEpoch = mktime(&ntpCopy);

  // Build epoch for RTC time
  struct tm rtcTm;
  rtcTm.tm_year = rtcNow.Year() - 1900;
  rtcTm.tm_mon  = rtcNow.Month() - 1;
  rtcTm.tm_mday = rtcNow.Day();
  rtcTm.tm_hour = rtcNow.Hour();
  rtcTm.tm_min  = rtcNow.Minute();
  rtcTm.tm_sec  = rtcNow.Second();
  rtcTm.tm_isdst = 0;
  time_t rtcEpoch = mktime(&rtcTm);

  long drift = (long)(ntpEpoch - rtcEpoch);
  Serial.printf("NTP vs RTC drift: %ld seconds\n", drift);

  if (abs(drift) >= thresholdSeconds) {
    Serial.printf("Drift >= %d s, updating RTC from NTP.\n", thresholdSeconds);
    syncRTCFromNTP(ntpTime);
    return true;
  }

  Serial.println("RTC is within tolerance, no update needed.");
  return false;
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

// Check if RTC is physically present (by checking if we can write and read back a control register/ram or time)
bool isRTCPresent() {
  RtcDateTime now = Rtc.GetDateTime();
  if (!now.IsValid()) {
    if (!Rtc.GetIsRunning()) {
      Rtc.SetIsRunning(true);
      if (!Rtc.GetIsRunning()) {
        return false;
      }
    }
  }
  return true;
}

// Set RTC time using epoch timestamp (seconds since 1970)
void setRTCTime(time_t epochSeconds) {
  if (!isRTCPresent()) {
    Serial.println("RTC module not found, skipping setRTCTime.");
    return;
  }
  
  // Convert UTC epoch to local time (GMT+7 for Vietnam)
  time_t localEpoch = epochSeconds + (7 * 3600);
  struct tm *timeinfo = gmtime(&localEpoch);
  
  if (timeinfo == nullptr) {
    Serial.println("Failed to convert epoch to local time structure.");
    return;
  }

  RtcDateTime dt = RtcDateTime(
    timeinfo->tm_year + 1900,
    timeinfo->tm_mon + 1,
    timeinfo->tm_mday,
    timeinfo->tm_hour,
    timeinfo->tm_min,
    timeinfo->tm_sec
  );

  Rtc.SetIsWriteProtected(false);
  Rtc.SetDateTime(dt);
  Serial.printf("RTC time manually synchronized to: %04d-%02d-%02d %02d:%02d:%02d\n",
                dt.Year(), dt.Month(), dt.Day(),
                dt.Hour(), dt.Minute(), dt.Second());
}
