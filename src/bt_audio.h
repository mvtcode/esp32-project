#pragma once
#include <Arduino.h>
#include "BluetoothA2DPSink.h"
#include "i2s_mic.h"    // for FRAME_SIZE

#define BT_DEVICE_NAME "MVT VU METER V2"

// DAC PCM5102A Pin assignments
#define DAC_PIN_BCK     18
#define DAC_PIN_LCK     19
#define DAC_PIN_DIN     23

/**
 * @brief Initialize Bluetooth A2DP Sink & AVRCP.
 * @param audio_queue Queue to pass decoded PCM frames to visualizer
 */
void bt_audio_init(QueueHandle_t audio_queue);

/**
 * @brief Start Bluetooth A2DP Sink (and I2S DAC output).
 */
void bt_audio_start();

/**
 * @brief Stop Bluetooth A2DP Sink.
 */
void bt_audio_stop();

/**
 * @brief Adjust volume by signed delta (+/-).
 */
void bt_audio_adjust_volume(int32_t delta);

/**
 * @brief Set absolute volume (0 - 127).
 */
void bt_audio_set_volume(uint8_t volume);

/**
 * @brief Get current volume level (0 - 127).
 */
uint8_t bt_audio_get_volume();

/**
 * @brief Toggle Play/Pause on connected phone/PC via AVRCP.
 */
void bt_audio_play_pause();

/**
 * @brief Clear paired device MAC and enter discoverable/pairing mode.
 */
void bt_audio_start_repairing();

/**
 * @brief Check if a Bluetooth device is currently connected.
 */
bool bt_audio_is_connected();

/**
 * @brief Check if Bluetooth audio is actively playing.
 */
bool bt_audio_is_playing();

/**
 * @brief Get connected device name / title string.
 */
const char* bt_audio_get_device_name();
