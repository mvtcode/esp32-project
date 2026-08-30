#include "config_manager.h"

Preferences ConfigManager::prefs;
bool ConfigManager::initialized = false;

#define PREF_NAMESPACE "cyd_cfg"

void ConfigManager::init() {
    if (initialized) return;
    prefs.begin(PREF_NAMESPACE, false);
    initialized = true;
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
    prefs.putString("wifi_ssid", ssid);
    prefs.putString("wifi_pass", password);
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
        prefs.putInt("city_idx", index);
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
    prefs.putInt("sync_int", minutes);
}

uint8_t ConfigManager::getBrightness() {
    init();
    int val = prefs.getInt("bright", 80);
    if (val < 10) val = 10;
    if (val > 100) val = 100;
    return (uint8_t)val;
}

void ConfigManager::setBrightness(uint8_t val) {
    init();
    if (val < 10) val = 10;
    if (val > 100) val = 100;
    prefs.putInt("bright", val);
}

int ConfigManager::getSleepTimeoutSeconds() {
    init();
    return prefs.getInt("sleep_to", 60); // default 60s
}

void ConfigManager::setSleepTimeoutSeconds(int seconds) {
    init();
    prefs.putInt("sleep_to", seconds);
}

bool ConfigManager::isAutoBrightnessEnabled() {
    init();
    return prefs.getBool("auto_br", false);
}

void ConfigManager::setAutoBrightnessEnabled(bool enabled) {
    init();
    prefs.putBool("auto_br", enabled);
}

uint8_t ConfigManager::getDefaultVolume() {
    init();
    int val = prefs.getInt("def_vol", 30);
    if (val < 0) val = 0;
    if (val > 100) val = 100;
    return (uint8_t)val;
}

void ConfigManager::setDefaultVolume(uint8_t vol) {
    init();
    if (vol > 100) vol = 100;
    prefs.putInt("def_vol", vol);
}

bool ConfigManager::isTouchBeepEnabled() {
    init();
    return prefs.getBool("touch_bp", false);
}

void ConfigManager::setTouchBeepEnabled(bool enabled) {
    init();
    prefs.putBool("touch_bp", enabled);
}

String ConfigManager::getLastAudioTrackPath() {
    init();
    return prefs.getString("last_track", "");
}

void ConfigManager::setLastAudioTrackPath(const String& path) {
    init();
    if (path.length() > 0) {
        prefs.putString("last_track", path);
    }
}

int ConfigManager::getLastAudioTrackIndex() {
    init();
    return prefs.getInt("last_idx", 0);
}

void ConfigManager::setLastAudioTrackIndex(int index) {
    init();
    if (index >= 0) {
        prefs.putInt("last_idx", index);
    }
}

bool ConfigManager::isDevModeEnabled() {
    init();
    return prefs.getBool("dev_mode", false);
}

void ConfigManager::setDevModeEnabled(bool enabled) {
    init();
    prefs.putBool("dev_mode", enabled);
}

void ConfigManager::resetToDefaults() {
    init();
    prefs.clear();
}
