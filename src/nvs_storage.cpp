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
    const char *name = (mode == AUDIO_MODE_BT) ? "BT" : ((mode == AUDIO_MODE_CLOCK) ? "CLOCK" : ((mode == AUDIO_MODE_XIAOZHI) ? "XIAOZHI" : "MIC"));
    LOG_D("NVS", "Saved audio_mode: %d (%s)", (int)mode, name);
}

AudioMode nvs_load_audio_mode() {
    uint8_t mode = s_prefs.getUChar("audio_mode", (uint8_t)AUDIO_MODE_MIC);
    if (mode > (uint8_t)AUDIO_MODE_XIAOZHI) mode = (uint8_t)AUDIO_MODE_MIC;
    const char *name = (mode == AUDIO_MODE_BT) ? "BT" : ((mode == AUDIO_MODE_CLOCK) ? "CLOCK" : ((mode == AUDIO_MODE_XIAOZHI) ? "XIAOZHI" : "MIC"));
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

void nvs_save_xz_ota_url(const char *url) {
    if (!url) return;
    s_prefs.putString("xz_ota_url", url);
    LOG_D("NVS", "Saved xz_ota_url: %s", url);
}

bool nvs_load_xz_ota_url(char *out, size_t max_len) {
    if (!out || max_len == 0) return false;
    String val = s_prefs.getString("xz_ota_url", "");
    if (val.length() > 0 && val.length() < max_len) {
        strncpy(out, val.c_str(), max_len - 1);
        out[max_len - 1] = '\0';
        return true;
    }
    return false;
}

void nvs_save_xz_ws_url(const char *url) {
    if (!url) return;
    s_prefs.putString("xz_ws_url", url);
    LOG_D("NVS", "Saved xz_ws_url: %s", url);
}

bool nvs_load_xz_ws_url(char *out, size_t max_len) {
    if (!out || max_len == 0) return false;
    String val = s_prefs.getString("xz_ws_url", "");
    if (val.length() > 0 && val.length() < max_len) {
        strncpy(out, val.c_str(), max_len - 1);
        out[max_len - 1] = '\0';
        return true;
    }
    return false;
}

void nvs_save_xz_token(const char *token) {
    if (!token) return;
    s_prefs.putString("xz_tok", token);
    LOG_D("NVS", "Saved xz_tok: %s", token);
}

bool nvs_load_xz_token(char *out, size_t max_len) {
    if (!out || max_len == 0) return false;
    String val = s_prefs.getString("xz_tok", "");
    if (val.length() > 0 && val.length() < max_len) {
        strncpy(out, val.c_str(), max_len - 1);
        out[max_len - 1] = '\0';
        return true;
    }
    return false;
}

void nvs_save_xz_client_id(const char *cid) {
    if (!cid) return;
    s_prefs.putString("xz_cid", cid);
    LOG_D("NVS", "Saved xz_cid: %s", cid);
}

bool nvs_load_xz_client_id(char *out, size_t max_len) {
    if (!out || max_len == 0) return false;
    String val = s_prefs.getString("xz_cid", "");
    if (val.length() > 0 && val.length() < max_len) {
        strncpy(out, val.c_str(), max_len - 1);
        out[max_len - 1] = '\0';
        return true;
    }
    return false;
}

void nvs_save_xz_activated(bool activated) {
    s_prefs.putBool("xz_act", activated);
    LOG_D("NVS", "Saved xz_act: %d", activated ? 1 : 0);
}

bool nvs_load_xz_activated() {
    return s_prefs.getBool("xz_act", false);
}

void nvs_save_xz_volume(uint8_t volume) {
    if (volume > 127) volume = 127;
    s_prefs.putUChar("xz_vol", volume);
}

uint8_t nvs_load_xz_volume() {
    return s_prefs.getUChar("xz_vol", 80);
}
