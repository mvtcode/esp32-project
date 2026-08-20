#pragma once
#include <Arduino.h>

enum AudioMode {
    AUDIO_MODE_MIC   = 0,
    AUDIO_MODE_BT    = 1,
    AUDIO_MODE_CLOCK = 2
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
