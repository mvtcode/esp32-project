#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

struct CityLocation {
    const char* name;
    float latitude;
    float longitude;
};

// Danh sách các Tỉnh/Thành phố lớn tại Việt Nam
static const CityLocation VIETNAM_CITIES[] = {
    {"Hà Nội", 21.0285f, 105.8542f},
    {"TP. Hồ Chí Minh", 10.8231f, 106.6297f},
    {"Đà Nẵng", 16.0544f, 108.2022f},
    {"Hải Phòng", 20.8449f, 106.6881f},
    {"Cần Thơ", 10.0452f, 105.7469f},
    {"Nha Trang", 12.2388f, 109.1967f},
    {"Đà Lạt", 11.9404f, 108.4583f},
    {"Huế", 16.4637f, 107.5909f},
    {"Vũng Tàu", 10.3460f, 107.0843f},
    {"Quy Nhơn", 13.7820f, 109.2197f},
    {"Hạ Long (Quảng Ninh)", 20.9505f, 107.0734f},
    {"Phan Thiết", 10.9274f, 108.1021f},
    {"Buôn Ma Thuột", 12.6667f, 108.0500f},
    {"Pleiku", 13.9833f, 108.0000f},
    {"Vinh (Nghệ An)", 18.6734f, 105.6813f},
    {"Thanh Hóa", 19.8067f, 105.7852f},
    {"Nam Định", 20.4200f, 106.1683f},
    {"Thái Nguyên", 21.5928f, 105.8442f},
    {"Việt Trì (Phú Thọ)", 21.3228f, 105.4019f},
    {"Lào Cai", 22.4856f, 103.9707f}
};

static const size_t VIETNAM_CITIES_COUNT = sizeof(VIETNAM_CITIES) / sizeof(VIETNAM_CITIES[0]);

class ConfigManager {
public:
    static void init();

    // WiFi Config
    static String getWifiSSID();
    static String getWifiPassword();
    static void setWifiCredentials(const String& ssid, const String& password);
    static bool hasWifiCredentials();

    // Location & Weather Config
    static int getCityIndex();
    static void setCityIndex(int index);
    static const CityLocation& getCurrentCity();

    // Data Sync Interval (in minutes: 15, 30, 60, 120)
    static int getSyncIntervalMinutes();
    static void setSyncIntervalMinutes(int minutes);

    // Display & Power Config
    static uint8_t getBrightness();         // 10 - 100%
    static void setBrightness(uint8_t val);
    static int getSleepTimeoutSeconds();    // 0 = never, 30, 60, 180, 300
    static void setSleepTimeoutSeconds(int seconds);
    static bool isAutoBrightnessEnabled();
    static void setAutoBrightnessEnabled(bool enabled);

    // Audio & Feedback Config
    static uint8_t getDefaultVolume();      // 0 - 100%
    static void setDefaultVolume(uint8_t vol);
    static bool isTouchBeepEnabled();
    static void setTouchBeepEnabled(bool enabled);
    static String getLastAudioTrackPath();
    static void setLastAudioTrackPath(const String& path);
    static int getLastAudioTrackIndex();
    static void setLastAudioTrackIndex(int index);

    // Developer Mode Config
    static bool isDevModeEnabled();
    static void setDevModeEnabled(bool enabled);

    // Factory Reset
    static void resetToDefaults();

private:
    static Preferences prefs;
    static bool initialized;
};

#endif // CONFIG_MANAGER_H
