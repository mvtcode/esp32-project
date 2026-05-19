#ifndef DASHBOARD_UI_H
#define DASHBOARD_UI_H

#include <lvgl.h>
#include <string.h>
#include "screens/home_screen.h"
#include "screens/calendar_screen.h"
#include "screens/player_screen.h"
#include "screens/settings_screen.h"

class DashboardUI {
public:
    DashboardUI();
    ~DashboardUI();

    // Screen navigation
    void setTabActive(int tabIndex);
    int getActiveTab() { return activeTabIndex; }

    // Screen getters (Only return valid if that screen is currently active/visible)
    HomeScreen* getHomeScreen() { return homeScreen; }
    CalendarScreen* getCalendarScreen() { return calendarScreen; }
    PlayerScreen* getPlayerScreen() { return playerScreen; }
    SettingsScreen* getSettingsScreen() { return settingsScreen; }

    // --- High-Performance MVC Telemetry Setters ---
    // HomeScreen Setters
    void updateTime(const char* timeStr, const char* secondsStr, const char* dateStr, bool isAm);
    void updateLunarCalendar(const char* lunarDayStr, const char* lunarInfoStr);
    void updateCalendarRibbon(int activeDayIndex, const int* dayNumbers);
    void updateWeather(int temp, const char* condition, int feelsLike, int humidity, int windSpeed, int uvIndex);
    void updateGoldPrices(int buySJC, int sellSJC, int buyDelta, int sellDelta);
    void updateFuelPrices(int ron95, int ron92, int diesel, int kerosene, int ron95Delta, int ron92Delta, int dieselDelta, int keroseneDelta);

    // CalendarScreen Setters
    void updateMonthYearHeader(const char* monthYearStr);
    void updateCalendarDays(int startDayOfWeek, int daysInMonth, int activeDay, const uint32_t* dotColorsMatrix);
    void updateSyncStatus(bool googleConnected, bool appleConnected);
    void clearEvents();
    void addEvent(const CalendarEvent& event);

    // PlayerScreen Setters
    void updateTrackInfo(const char* title, const char* artist, const char* album, const char* qualityStr);
    void updatePlaybackProgress(int currentTimeSecs, int totalTimeSecs);
    void setPlayState(bool isPlaying);
    void updatePlaybackMode(bool shuffleActive, bool repeatActive);
    void updateVolume(int volume);
    void updateEQ(const char* eqMode);
    void clearPlaylist();
    void addPlaylistItem(const PlaylistItem& item);

    // SettingsScreen Setters
    void updateDeviceInfo(const SettingsDeviceInfo& info);
    void updateMemoryUsage(float usedGB, float totalGB);
    void updateBatteryStatus(int percent, const char* durationStr);
    void updateWiFiConnection(const char* ssid, const char* ip);
    void updateLanguage(const char* langStr);
    void setActiveMenuItem(int index);

    // Tick update to run micro-animations (like music spectrum)
    void tick();

    lv_obj_t* getRoot() { return masterContainer; }

private:
    lv_obj_t* masterContainer;
    lv_obj_t* activeViewArea;
    lv_obj_t* bottomNavBar;

    // Screens (Dynamically allocated/deallocated based on active tab)
    HomeScreen* homeScreen;
    CalendarScreen* calendarScreen;
    PlayerScreen* playerScreen;
    SettingsScreen* settingsScreen;

    // Tabs navigation elements
    lv_obj_t* tabContainers[4];
    lv_obj_t* tabIndicators[4];
    lv_obj_t* tabIcons[4];
    lv_obj_t* tabLabels[4];

    int activeTabIndex;

    // --- State Cache (For restoring screen telemetry upon tab recreation) ---
    struct HomeCache {
        char timeStr[12] = "15:45";
        char secStr[6] = "00";
        char dateStr[48] = "Thứ Năm, 15/05/2025";
        bool isAm = false;
        char lunarDayStr[32] = "15/4 Lịch âm";
        char lunarInfoStr[48] = "Năm Ất Tỵ";
        int activeDayIndex = 3;
        int dayNumbers[7] = {12, 13, 14, 15, 16, 17, 18};
        int temp = 28;
        char condition[64] = "Nhiều mây";
        int feelsLike = 27;
        int humidity = 84;
        int windSpeed = 12;
        int uvIndex = 6;
        int goldBuy = 82500;
        int goldSell = 85000;
        int goldBuyDelta = 500;
        int goldSellDelta = -200;
        int fuelRon95 = 23780;
        int fuelRon92 = 22620;
        int fuelDiesel = 19850;
        int fuelKerosene = 16420;
        int fuelRon95Delta = 150;
        int fuelRon92Delta = -80;
        int fuelDieselDelta = 20;
        int fuelKeroseneDelta = 0;
    } homeCache;

    struct CalendarCache {
        char monthYearStr[32] = "Tháng 05, 2026";
        int startDayOfWeek = 4;
        int daysInMonth = 31;
        int activeDay = 15;
        uint32_t dotColors[42] = {0};
        bool googleConnected = true;
        bool appleConnected = true;
        
        CalendarEvent events[4];
        int eventCount = 0;
    } calCache;

    struct PlayerCache {
        char title[64] = "Nơi Này Có Anh";
        char artist[64] = "Sơn Tùng M-TP";
        char album[64] = "Album 2017";
        char qualityStr[32] = "MP3 320kbps";
        int currentSec = 84;
        int totalSec = 275;
        bool isPlaying = true;
        bool shuffleActive = true;
        bool repeatActive = false;
        int volume = 20;
        char eqMode[16] = "POP";
        
        PlaylistItem playlist[6];
        int playlistCount = 0;
    } playerCache;

    struct SettingsCache {
        SettingsDeviceInfo info = {"CYD 3.5 Controller", "ESP32-3248S035", "v1.0.0", "19/05/2026", "FreeRTOS", "SN-9842A1"};
        float usedGB = 8.6f;
        float totalGB = 16.0f;
        int batPercent = 95;
        char batDuration[32] = "1 giờ 15 phút";
        char wifiSsid[32] = "Wifi_Home";
        char wifiIp[32] = "192.168.1.15";
        char language[32] = "Tiếng Việt";
        int activeMenuItem = 0;
    } settingsCache;

    // Helper functions
    void initTabs();
    static void tab_click_event_cb(lv_event_t* e);
};

#endif // DASHBOARD_UI_H
