#include "time_service.h"
#include "log.h"

bool TimeService::synced = false;
uint32_t TimeService::lastSyncAttempt = 0;

static const char* VI_DAY_NAMES[] = {
    "Thứ Hai", "Thứ Ba", "Thứ Tư", "Thứ Năm", "Thứ Sáu", "Thứ Bảy", "Chủ Nhật"
};

void TimeService::init() {
    // Múi giờ GMT+7 (Asia/Ho_Chi_Minh)
    configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
    synced = false;
    lastSyncAttempt = 0;
    LOG_I("Time", "NTP Client configured with GMT+7.");
}

void TimeService::update(bool wifiConnected) {
    if (!wifiConnected) return;

    uint32_t now = millis();
    if (!synced && (now - lastSyncAttempt >= 5000 || lastSyncAttempt == 0)) {
        lastSyncAttempt = now;
        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 200)) {
            if (timeinfo.tm_year > (2020 - 1900)) {
                synced = true;
                LOG_I("Time", "NTP Sync SUCCESS: %02d:%02d:%02d %02d/%02d/%04d",
                      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                      timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
            }
        }
    }
}

bool TimeService::isSynced() {
    return synced;
}

TimeInfo TimeService::getTimeInfo() {
    TimeInfo info;
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo, 10) || timeinfo.tm_year < (2024 - 1900)) {
        // Trạng thái đang tải / chưa đồng bộ NTP
        info.hour = 0;
        info.minute = 0;
        info.second = 0;
        info.isAm = false;
        info.day = 1;
        info.month = 1;
        info.year = 2026;
        info.dayOfWeek = 0;
        info.activeDayIndex = 0;
        info.isSynced = false;
        snprintf(info.timeStr, sizeof(info.timeStr), "-- : -- : --");
        snprintf(info.secStr, sizeof(info.secStr), "--");
        snprintf(info.dateStr, sizeof(info.dateStr), "Đang đồng bộ NTP...");
        for (int i = 0; i < 7; i++) info.weekDayNumbers[i] = i + 1;
        info.lunar.day = 0;
        info.lunar.month = 0;
        info.lunar.year = 0;
        info.lunar.dayName = "Đang tải";
        info.lunar.yearName = "Đang tải";
        info.lunar.holiday = nullptr;
        return info;
    }

    info.hour = timeinfo.tm_hour;
    info.minute = timeinfo.tm_min;
    info.second = timeinfo.tm_sec;
    info.isAm = (info.hour < 12);
    info.day = timeinfo.tm_mday;
    info.month = timeinfo.tm_mon + 1;
    info.year = timeinfo.tm_year + 1900;
    info.isSynced = true;

    // Chuyển đổi tm_wday (0=Chủ nhật, 1=Thứ hai... 6=Thứ bảy) sang dải 0=Thứ hai ... 6=Chủ nhật
    int monSunIdx = (timeinfo.tm_wday == 0) ? 6 : (timeinfo.tm_wday - 1);
    info.dayOfWeek = monSunIdx;
    info.activeDayIndex = monSunIdx;

    snprintf(info.timeStr, sizeof(info.timeStr), "%02d : %02d : %02d", info.hour, info.minute, info.second);
    snprintf(info.secStr, sizeof(info.secStr), ":%02d", info.second);
    snprintf(info.dateStr, sizeof(info.dateStr), "%s, %02d/%02d", 
             VI_DAY_NAMES[monSunIdx], info.day, info.month);

    // Tính toán ngày của 7 ngày trong tuần từ Thứ Hai đến Chủ Nhật
    time_t nowSec = mktime(&timeinfo);
    time_t mondaySec = nowSec - (monSunIdx * 86400);

    for (int i = 0; i < 7; i++) {
        time_t daySec = mondaySec + (i * 86400);
        struct tm dInfo;
        localtime_r(&daySec, &dInfo);
        info.weekDayNumbers[i] = dInfo.tm_mday;
    }

    // Tính toán Lịch Âm Hồ Ngọc Đức
    info.lunar = getDetailedLunarDate(info.year, info.month, info.day);

    return info;
}
