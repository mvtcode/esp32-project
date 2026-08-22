#pragma once
#ifndef MP3_PLAYER_H
#define MP3_PLAYER_H

#include <Arduino.h>
#include "sd_card.h"
#include "i2s_mic.h" // for AudioFrame and FRAME_SIZE

/**
 * @brief Initialize MP3 Player subsystem and connect to visualizer queue.
 */
void mp3_player_init(QueueHandle_t audio_queue);

/**
 * @brief Start MP3 Player engine and setup I2S DAC driver.
 */
void mp3_player_start();

/**
 * @brief Stop MP3 Player engine, stop audio tasks, release I2S DAC driver.
 */
void mp3_player_stop();

/**
 * @brief Play specific track by index.
 */
void mp3_player_play_track(int index);

/**
 * @brief Switch to next track (wraps around).
 */
void mp3_player_next_track();

/**
 * @brief Switch to previous track (wraps around).
 */
void mp3_player_prev_track();

/**
 * @brief Toggle Play / Pause state.
 */
void mp3_player_toggle_play_pause();

/**
 * @brief Pause playback with soft volume ramp down.
 */
void mp3_player_pause();

/**
 * @brief Resume playback with soft volume ramp up.
 */
void mp3_player_resume();

/**
 * @brief Adjust volume by signed delta.
 */
void mp3_player_adjust_volume(int32_t delta);

/**
 * @brief Set absolute volume (0 - 127).
 */
void mp3_player_set_volume(uint8_t volume);

/**
 * @brief Get current volume level (0 - 127).
 */
uint8_t mp3_player_get_volume();

/**
 * @brief Check if MP3 player is currently actively playing.
 */
bool mp3_player_is_playing();

/**
 * @brief Check if MP3 player is currently paused.
 */
bool mp3_player_is_paused();

/**
 * @brief Check if MP3 player has an active song loaded.
 */
bool mp3_player_is_active();

/**
 * @brief Get current playing track index.
 */
int mp3_player_get_current_track_index();

/**
 * @brief Get current playing track item pointer.
 */
const PlaylistItem* mp3_player_get_current_track();

/**
 * @brief Get current playback position in seconds.
 */
uint32_t mp3_player_get_current_pos_sec();

/**
 * @brief Get estimated or metadata total duration in seconds (or 0 if unknown).
 */
uint32_t mp3_player_get_total_duration_sec();

/**
 * @brief Get playback progress percentage (0 - 100).
 */
uint8_t mp3_player_get_progress_percent();

#endif // MP3_PLAYER_H
