#ifndef DASHBOARD_UI_H
#define DASHBOARD_UI_H

#include <lvgl.h>
#include <string.h>
#include "screens/home_screen.h"
#include "screens/calendar_screen.h"
#include "screens/player_screen.h"
#include "screens/settings_screen.h"
#include "dev_hud.h"

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
    void updateWeather(int temp, const char* condition, int feelsLike, int humidity, int windSpeed, int uvIndex, const char* cityName = nullptr);
    void updateGoldPrices(const char* buySJC, const char* sellSJC);
    void updateFuelPrices(int ron95, int ron92, int diesel, int mazut, int ron95Delta, int ron92Delta, int dieselDelta, int mazutDelta);

    // CalendarScreen Setters
    void setCalendarToday(int year, int month, int day);

    // PlayerScreen Setters
    void updateTrackInfo(const char* title, const char* artist, const char* album, const char* qualityStr);
    void updatePlaybackProgress(int currentTimeSecs, int totalTimeSecs);
    void setPlayState(bool isPlaying);
    void updatePlaybackMode(bool shuffleActive, int repeatMode);
    void updateVolume(int volumePercent);
    void updateEQ(const char* eqMode);
    void clearPlaylist();
    void addPlaylistItem(const PlaylistItem& item, int trackIndex = -1);

    // SettingsScreen Setters
    void updateDeviceInfo(const SettingsDeviceInfo& info);
    void updateSettingsTelemetry(uint32_t freeHeapBytes, const char* uptimeStr, const char* ipStr, const char* macStr);
    void updateWifiSettings(const char* statusStr, const char* connectedSSID, const char* ipStr, int rssi);
    void setActiveMenuItem(int index);

    // Common Animations & Clock tick
    void tick();

    // Dev HUD toggling & update
    void setDevHudVisible(bool visible);
    void updateDevHud(float fps, uint32_t freeHeapKb, float memUsagePercent, uint8_t cpuPercent, int32_t rssi, const char* ip);


    lv_obj_t* getRoot() { return masterContainer; }

private:
    // UI Master View Containers
    lv_obj_t* masterContainer;
    lv_obj_t* activeViewArea;
    lv_obj_t* bottomNavBar;

    // Screens (Dynamically allocated/deallocated based on active tab)
    HomeScreen* homeScreen;
    CalendarScreen* calendarScreen;
    PlayerScreen* playerScreen;
    SettingsScreen* settingsScreen;

    // Developer Mode HUD
    DevHud* devHud;

    // Tabs navigation elements
    lv_obj_t* tabContainers[4];
    lv_obj_t* tabIndicators[4];
    lv_obj_t* tabIcons[4];
    lv_obj_t* tabLabels[4];

    int activeTabIndex;
    int lastCheckedTrackIdx;

    // Cache to restore screens state on demand
    struct HomeCache {
        char timeStr[16] = "-- : -- : --";
        char secStr[6] = "--";
        char dateStr[48] = "Đang đồng bộ NTP...";
        bool isAm = false;
        char lunarDayStr[32] = "ÂL: Đang đồng bộ...";
        char lunarInfoStr[48] = "";
        int activeDayIndex = 0;
        int dayNumbers[7] = {1, 2, 3, 4, 5, 6, 7};
        int temp = 28;
        char condition[64] = "Đang tải thời tiết...";
        int feelsLike = 28;
        int humidity = 70;
        int windSpeed = 10;
        int uvIndex = 5;
        char goldBuy[16] = "145,7";
        char goldSell[16] = "148,7";
        int fuelRon95 = 22600;
        int fuelRon92 = 21760;
        int fuelDiesel = 28080;
        int fuelMazut = 18140;
        int fuelRon95Delta = -60;
        int fuelRon92Delta = -70;
        int fuelDieselDelta = -460;
        int fuelMazutDelta = 460;
    } homeCache;

    struct CalendarCache {
        int year = 2026;
        int month = 8;
        int day = 30;
    } calCache;

    struct PlayerCache {
        char title[64] = "Nơi Này Có Anh";
        char artist[64] = "Sơn Tùng M-TP";
        char album[64] = "Album 2017";
        char qualityStr[32] = "MP3 320kbps";
        int currentSec = 84;
        int totalSec = 275;
        bool isPlaying = true;
        bool shuffleActive = false;
        int repeatMode = 1; // 0=Off, 1=All, 2=One
        int volume = 50;
        char eqMode[16] = "POP";
        
        PlaylistItem playlist[6];
        int playlistCount = 0;
    } playerCache;

    struct SettingsCache {

        SettingsDeviceInfo info = {"ESP32 Dashboard", "CYD 3.5 ST7796", "v2.5.0", "19/05/2026", "FreeRTOS", "CYD-35-ESP32"};
        uint32_t freeHeap = 160000;
        char uptimeStr[32] = "00:00:00";
        char ipStr[32] = "0.0.0.0";
        char macStr[32] = "--:--:--:--:--:--";
        char wifiState[32] = "Chưa kết nối";
        char wifiSsid[32] = "";
        int wifiRssi = -100;
        int activeMenuItem = 0;
    } settingsCache;

    // Helper functions
    void initTabs();
    static void tab_click_event_cb(lv_event_t* e);
};

#endif // DASHBOARD_UI_H
