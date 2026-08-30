#ifndef SETTINGS_SCREEN_H
#define SETTINGS_SCREEN_H

#include <lvgl.h>
#include <vector>
#include <Arduino.h>

struct SettingsDeviceInfo {
    const char* deviceName;
    const char* model;
    const char* fwVersion;
    const char* buildDate;
    const char* osName;
    const char* serialNumber;
};

class SettingsScreen {
public:
    SettingsScreen(lv_obj_t* parent);
    ~SettingsScreen();

    // Menu Navigation
    void setActiveMenuItem(int index);
    int getActiveMenuItem() const { return currentMenuIndex; }

    // Dynamic Updates from Main Loop
    void updateDeviceInfo(const SettingsDeviceInfo& info);
    void updateTelemetry(uint32_t freeHeap, const char* uptimeStr, const char* ipStr, const char* macStr);
    void updateWifiStatus(const char* stateStr, const char* ssid, const char* ip, int rssi);
    void updateWifiScanList();

    // Dialog / Modal helpers
    void showWifiPasswordModal(const char* ssid);
    void hideWifiPasswordModal();

    lv_obj_t* getRoot() { return rootContainer; }

private:
    lv_obj_t* rootContainer;

    // Sidebar items (6 tabs)
    lv_obj_t* sidebarMenu;
    lv_obj_t* sidebarItems[6];
    int currentMenuIndex;

    // Right Content Host
    lv_obj_t* rightPane;

    // --- Tab 0: Device Widgets ---
    lv_obj_t* lblDevName;
    lv_obj_t* lblDevModel;
    lv_obj_t* lblDevFw;
    lv_obj_t* lblDevBuild;
    lv_obj_t* lblDevOs;
    lv_obj_t* lblDevSerial;
    lv_obj_t* lblDevUptime;
    lv_obj_t* lblDevRam;
    lv_obj_t* lblDevIp;
    lv_obj_t* lblDevMac;
    lv_obj_t* btnRestart;
    lv_obj_t* btnFactoryReset;

    // --- Tab 1: WiFi Widgets ---
    lv_obj_t* lblWifiCurrentState;
    lv_obj_t* lblWifiCurrentInfo;
    lv_obj_t* btnWifiScan;
    lv_obj_t* lblBtnScan;
    lv_obj_t* wifiListContainer;

    // WiFi Password Modal & Keyboard
    lv_obj_t* modalBackdrop;
    lv_obj_t* modalCard;
    lv_obj_t* modalSsidLabel;
    lv_obj_t* taPassword;
    lv_obj_t* keyboard;
    char selectedSsid[64];

    // --- Tab 2: SD Card Widgets ---
    lv_obj_t* lblSdStatus;
    lv_obj_t* lblSdBusInfo;
    lv_obj_t* barSdUsage;
    lv_obj_t* lblSdCapacity;
    lv_obj_t* lblSdPercent;
    lv_obj_t* btnRefreshSd;
    lv_obj_t* btnFormatSd;
    lv_obj_t* lblSdActionMsg;

    // --- Tab 3: Sync Widgets ---
    lv_obj_t* ddCity;
    lv_obj_t* ddSyncInterval;
    lv_obj_t* btnSyncNow;
    lv_obj_t* lblSyncStatus;

    // --- Tab 4: Display Widgets ---
    lv_obj_t* sliderBrightness;
    lv_obj_t* lblBrightnessVal;
    lv_obj_t* ddSleepTimeout;
    lv_obj_t* swAutoBrightness;

    // --- Tab 5: System Widgets ---
    lv_obj_t* swDevMode;
    lv_obj_t* sliderVolume;
    lv_obj_t* lblVolumeVal;
    lv_obj_t* swTouchBeep;

    // Cached telemetry for on-demand rendering
    uint32_t cachedFreeHeap;
    char cachedUptime[32];
    char cachedIp[32];
    char cachedMac[32];
    char cachedWifiState[32];
    char cachedWifiSsid[32];
    int cachedWifiRssi;

    // Sub-pane Builders & Cleaners
    void createSidebar(lv_obj_t* parent);
    void destroyCurrentPane();
    void buildDevicePane();
    void buildWifiPane();
    void buildSdCardPane();
    void buildSyncPane();
    void buildDisplayPane();
    void buildSystemPane();
    void createWifiPasswordModal();

    // Event callbacks
    static void sidebar_click_event_cb(lv_event_t* e);
    static void wifi_scan_click_cb(lv_event_t* e);
    static void wifi_item_click_cb(lv_event_t* e);
    static void wifi_connect_submit_cb(lv_event_t* e);
    static void wifi_modal_cancel_cb(lv_event_t* e);
    static void refresh_sd_click_cb(lv_event_t* e);
    static void format_sd_click_cb(lv_event_t* e);
    static void city_changed_cb(lv_event_t* e);
    static void sync_interval_changed_cb(lv_event_t* e);
    static void sync_now_click_cb(lv_event_t* e);
    static void brightness_changed_cb(lv_event_t* e);
    static void sleep_timeout_changed_cb(lv_event_t* e);
    static void dev_mode_toggle_cb(lv_event_t* e);
    static void volume_changed_cb(lv_event_t* e);
    static void restart_click_cb(lv_event_t* e);
    static void factory_reset_click_cb(lv_event_t* e);
};

#endif // SETTINGS_SCREEN_H
