#include "config_manager.h"
#include "log.h"

static const char* TAG = "Config";

Preferences ConfigManager::prefs;
bool ConfigManager::initialized = false;

uint8_t ConfigManager::s_cachedBrightness = 80;
bool ConfigManager::s_brightnessLoaded = false;
bool ConfigManager::s_brightnessDirty = false;
unsigned long ConfigManager::s_brightnessDebounceTime = 0;

uint8_t ConfigManager::s_cachedVolume = 30;
bool ConfigManager::s_volumeLoaded = false;
bool ConfigManager::s_volumeDirty = false;
unsigned long ConfigManager::s_volumeDebounceTime = 0;

#define PREF_NAMESPACE "cyd_cfg"
#define DEBOUNCE_DELAY_MS 600 // Trì hoãn 600ms chống mòn Flash NVS (Rule 7)

void ConfigManager::init() {
    if (initialized) return;
    prefs.begin(PREF_NAMESPACE, false);
    initialized = true;
}

void ConfigManager::update() {
    if (!initialized) return;
    unsigned long now = millis();

    // Debounce ghi NVS cho độ sáng màn hình
    if (s_brightnessDirty && (now - s_brightnessDebounceTime >= DEBOUNCE_DELAY_MS)) {
        s_brightnessDirty = false;
        prefs.putInt("bright", s_cachedBrightness);
        LOG_D(TAG, "NVS flushed: Brightness = %d%%", s_cachedBrightness);
    }

    // Debounce ghi NVS cho âm lượng mặc định
    if (s_volumeDirty && (now - s_volumeDebounceTime >= DEBOUNCE_DELAY_MS)) {
        s_volumeDirty = false;
        prefs.putInt("def_vol", s_cachedVolume);
        LOG_D(TAG, "NVS flushed: Volume = %d%%", s_cachedVolume);
    }
}

void ConfigManager::flush() {
    if (!initialized) return;
    if (s_brightnessDirty) {
        s_brightnessDirty = false;
        prefs.putInt("bright", s_cachedBrightness);
    }
    if (s_volumeDirty) {
        s_volumeDirty = false;
        prefs.putInt("def_vol", s_cachedVolume);
    }
}

String ConfigManager::getWifiSSID() {
    init();
    return prefs.getString("wifi_ssid", "");
}

String ConfigManager::getWifiPassword() {
    init();
    return prefs.getString("wifi_pass", "");
}

void ConfigManager::setWifiCredentials(const String& ssid, const String& password) {
    init();
    if (getWifiSSID() != ssid) {
        prefs.putString("wifi_ssid", ssid);
    }
    if (getWifiPassword() != password) {
        prefs.putString("wifi_pass", password);
    }
}

bool ConfigManager::hasWifiCredentials() {
    return getWifiSSID().length() > 0;
}

int ConfigManager::getCityIndex() {
    init();
    int idx = prefs.getInt("city_idx", 0);
    if (idx < 0 || (size_t)idx >= VIETNAM_CITIES_COUNT) idx = 0;
    return idx;
}

void ConfigManager::setCityIndex(int index) {
    init();
    if (index >= 0 && (size_t)index < VIETNAM_CITIES_COUNT) {
        if (getCityIndex() != index) {
            prefs.putInt("city_idx", index);
        }
    }
}

const CityLocation& ConfigManager::getCurrentCity() {
    int idx = getCityIndex();
    return VIETNAM_CITIES[idx];
}

int ConfigManager::getSyncIntervalMinutes() {
    init();
    return prefs.getInt("sync_int", 30);
}

void ConfigManager::setSyncIntervalMinutes(int minutes) {
    init();
    if (getSyncIntervalMinutes() != minutes) {
        prefs.putInt("sync_int", minutes);
    }
}

uint8_t ConfigManager::getBrightness() {
    init();
    if (!s_brightnessLoaded) {
        int val = prefs.getInt("bright", 80);
        if (val < 10) val = 10;
        if (val > 100) val = 100;
        s_cachedBrightness = (uint8_t)val;
        s_brightnessLoaded = true;
    }
    return s_cachedBrightness;
}

void ConfigManager::setBrightness(uint8_t val) {
    init();
    if (val < 10) val = 10;
    if (val > 100) val = 100;

    // Dirty check: nếu không đổi và không dirty thì không cần làm gì
    if (s_brightnessLoaded && s_cachedBrightness == val && !s_brightnessDirty) {
        return;
    }

    s_cachedBrightness = val;
    s_brightnessLoaded = true;
    s_brightnessDirty = true;
    s_brightnessDebounceTime = millis();
}

int ConfigManager::getSleepTimeoutSeconds() {
    init();
    return prefs.getInt("sleep_to", 60); // default 60s
}

void ConfigManager::setSleepTimeoutSeconds(int seconds) {
    init();
    if (getSleepTimeoutSeconds() != seconds) {
        prefs.putInt("sleep_to", seconds);
    }
}

bool ConfigManager::isAutoBrightnessEnabled() {
    init();
    return prefs.getBool("auto_br", false);
}

void ConfigManager::setAutoBrightnessEnabled(bool enabled) {
    init();
    if (isAutoBrightnessEnabled() != enabled) {
        prefs.putBool("auto_br", enabled);
    }
}

uint8_t ConfigManager::getDefaultVolume() {
    init();
    if (!s_volumeLoaded) {
        int val = prefs.getInt("def_vol", 30);
        if (val < 0) val = 0;
        if (val > 100) val = 100;
        s_cachedVolume = (uint8_t)val;
        s_volumeLoaded = true;
    }
    return s_cachedVolume;
}

void ConfigManager::setDefaultVolume(uint8_t vol) {
    init();
    if (vol > 100) vol = 100;

    // Dirty check: nếu không đổi và không dirty thì bỏ qua
    if (s_volumeLoaded && s_cachedVolume == vol && !s_volumeDirty) {
        return;
    }

    s_cachedVolume = vol;
    s_volumeLoaded = true;
    s_volumeDirty = true;
    s_volumeDebounceTime = millis();
}

bool ConfigManager::isTouchBeepEnabled() {
    init();
    return prefs.getBool("touch_bp", false);
}

void ConfigManager::setTouchBeepEnabled(bool enabled) {
    init();
    if (isTouchBeepEnabled() != enabled) {
        prefs.putBool("touch_bp", enabled);
    }
}

String ConfigManager::getLastAudioTrackPath() {
    init();
    return prefs.getString("last_track", "");
}

void ConfigManager::setLastAudioTrackPath(const String& path) {
    init();
    if (path.length() > 0 && getLastAudioTrackPath() != path) {
        prefs.putString("last_track", path);
    }
}

int ConfigManager::getLastAudioTrackIndex() {
    init();
    return prefs.getInt("last_idx", 0);
}

void ConfigManager::setLastAudioTrackIndex(int index) {
    init();
    if (index >= 0 && getLastAudioTrackIndex() != index) {
        prefs.putInt("last_idx", index);
    }
}

bool ConfigManager::isDevModeEnabled() {
    init();
    return prefs.getBool("dev_mode", false);
}

void ConfigManager::setDevModeEnabled(bool enabled) {
    init();
    if (isDevModeEnabled() != enabled) {
        prefs.putBool("dev_mode", enabled);
    }
}

void ConfigManager::resetToDefaults() {
    init();
    s_brightnessDirty = false;
    s_volumeDirty = false;
    s_brightnessLoaded = false;
    s_volumeLoaded = false;
    prefs.clear();
}

