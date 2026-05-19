#ifndef SETTINGS_SCREEN_H
#define SETTINGS_SCREEN_H

#include <lvgl.h>

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
    ~SettingsScreen() {
        if (rootContainer) lv_obj_del(rootContainer);
    }

    // Setters for dynamic updates
    void updateDeviceInfo(const SettingsDeviceInfo& info);
    void updateMemoryUsage(float usedGB, float totalGB);
    void updateBatteryStatus(int percent, const char* durationStr);
    void updateWiFiConnection(const char* ssid, const char* ip);
    void updateLanguage(const char* langStr);
    
    void setActiveMenuItem(int index);

    lv_obj_t* getRoot() { return rootContainer; }

private:
    lv_obj_t* rootContainer;

    // Sidebar items
    lv_obj_t* sidebarMenu;
    lv_obj_t* sidebarItems[8];

    // Device details
    lv_obj_t* lblDevName;
    lv_obj_t* lblDevModel;
    lv_obj_t* lblDevFw;
    lv_obj_t* lblDevBuild;
    lv_obj_t* lblDevOs;
    lv_obj_t* lblDevSerial;
    lv_obj_t* imgNightSky; // simulated custom graphics

    // System Widgets
    lv_obj_t* barMemory;
    lv_obj_t* lblMemoryDetails;
    
    lv_obj_t* lblBatteryDetails;
    lv_obj_t* lblBatteryRemaining;

    lv_obj_t* lblWiFiSsid;
    lv_obj_t* lblWiFiIp;

    lv_obj_t* lblLanguageVal;

    // Helper functions
    void createSidebar(lv_obj_t* parent);
    void createContentPane(lv_obj_t* parent);
};

#endif // SETTINGS_SCREEN_H
