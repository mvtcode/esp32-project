#include "nvs_storage.h"
#include <Preferences.h>
#include "log.h"

static Preferences s_prefs;
static const char *PREFS_NAMESPACE = "vu_meter";

void nvs_storage_init() {
    s_prefs.begin(PREFS_NAMESPACE, false);
    LOG_I("NVS", "Storage initialized");
}

void nvs_save_audio_mode(AudioMode mode) {
    s_prefs.putUChar("audio_mode", (uint8_t)mode);
    const char *name = (mode == AUDIO_MODE_BT) ? "BT" : ((mode == AUDIO_MODE_CLOCK) ? "CLOCK" : "MIC");
    LOG_D("NVS", "Saved audio_mode: %d (%s)", (int)mode, name);
}

AudioMode nvs_load_audio_mode() {
    uint8_t mode = s_prefs.getUChar("audio_mode", (uint8_t)AUDIO_MODE_MIC);
    if (mode > (uint8_t)AUDIO_MODE_CLOCK) mode = (uint8_t)AUDIO_MODE_MIC;
    const char *name = (mode == AUDIO_MODE_BT) ? "BT" : ((mode == AUDIO_MODE_CLOCK) ? "CLOCK" : "MIC");
    LOG_D("NVS", "Loaded audio_mode: %d (%s)", (int)mode, name);
    return (AudioMode)mode;
}

void nvs_save_bt_mac(const uint8_t mac[6]) {
    s_prefs.putBytes("bt_mac", mac, 6);
    LOG_D("NVS", "Saved BT MAC: %02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

bool nvs_load_bt_mac(uint8_t mac[6]) {
    size_t len = s_prefs.getBytes("bt_mac", mac, 6);
    if (len == 6) {
        // Check if not all zeros or all 0xFF
        bool valid = false;
        for (int i = 0; i < 6; i++) {
            if (mac[i] != 0 && mac[i] != 0xFF) valid = true;
        }
        if (valid) {
            LOG_D("NVS", "Loaded BT MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            return true;
        }
    }
    LOG_D("NVS", "No valid saved BT MAC found");
    return false;
}

void nvs_erase_bt_mac() {
    s_prefs.remove("bt_mac");
    LOG_I("NVS", "Erased BT MAC from storage");
}

void nvs_save_volume(uint8_t volume) {
    if (volume > 127) volume = 127;
    s_prefs.putUChar("volume", volume);
}

uint8_t nvs_load_volume() {
    // Default volume: 80 (~63%)
    return s_prefs.getUChar("volume", 80);
}

void nvs_save_display_mode(uint8_t mode) {
    s_prefs.putUChar("disp_mode", mode);
}

uint8_t nvs_load_display_mode() {
    return s_prefs.getUChar("disp_mode", 0);
}

void nvs_save_auto_cycle(bool auto_cycle) {
    s_prefs.putBool("auto_cycle", auto_cycle);
}

bool nvs_load_auto_cycle() {
    return s_prefs.getBool("auto_cycle", false);
}
