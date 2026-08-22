#pragma once
#include <Arduino.h>

enum AudioMode {
    AUDIO_MODE_MIC     = 0,
    AUDIO_MODE_BT      = 1,
    AUDIO_MODE_CLOCK   = 2,
    AUDIO_MODE_XIAOZHI = 3
};

/**
 * @brief Initialize Preferences / NVS storage namespace.
 */
void nvs_storage_init();

/**
 * @brief Audio Mode (MIC vs BT) persistence.
 */
void nvs_save_audio_mode(AudioMode mode);
AudioMode nvs_load_audio_mode();

/**
 * @brief Bluetooth last paired MAC address (6 bytes).
 */
void nvs_save_bt_mac(const uint8_t mac[6]);
bool nvs_load_bt_mac(uint8_t mac[6]);
void nvs_erase_bt_mac();

/**
 * @brief Volume level (0 - 127 for AVRCP standard).
 */
void nvs_save_volume(uint8_t volume);
uint8_t nvs_load_volume();

/**
 * @brief Display mode and Auto-cycle persistence.
 */
void nvs_save_display_mode(uint8_t mode);
uint8_t nvs_load_display_mode();
void nvs_save_auto_cycle(bool auto_cycle);
bool nvs_load_auto_cycle();

/**
 * @brief XiaoZhi AI configuration and state persistence.
 */
void nvs_save_xz_ota_url(const char *url);
bool nvs_load_xz_ota_url(char *out, size_t max_len);
void nvs_save_xz_ws_url(const char *url);
bool nvs_load_xz_ws_url(char *out, size_t max_len);
void nvs_save_xz_token(const char *token);
bool nvs_load_xz_token(char *out, size_t max_len);
void nvs_save_xz_client_id(const char *cid);
bool nvs_load_xz_client_id(char *out, size_t max_len);
void nvs_save_xz_activated(bool activated);
bool nvs_load_xz_activated();
void nvs_save_xz_volume(uint8_t volume);
uint8_t nvs_load_xz_volume();
