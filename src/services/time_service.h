#ifndef TIME_SERVICE_H
#define TIME_SERVICE_H

#include <Arduino.h>
#include <time.h>
#include "lunar_calendar.h"

struct TimeInfo {
    int hour;
    int minute;
    int second;
    bool isAm;
    int day;
    int month;
    int year;
    int dayOfWeek; // 0 = Mon, 1 = Tue, ..., 6 = Sun
    char timeStr[16];
    char secStr[8];
    char dateStr[64];
    int weekDayNumbers[7];
    int activeDayIndex;
    LunarDate lunar;
    bool isSynced;
};

class TimeService {
public:
    static void init();
    static void update(bool wifiConnected);
    static bool isSynced();
    static TimeInfo getTimeInfo();

private:
    static bool synced;
    static uint32_t lastSyncAttempt;
};

#endif // TIME_SERVICE_H
