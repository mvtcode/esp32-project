/**
 * voice_engine.h — ESP-SR voice recognition engine
 * IoT Voice Command System
 *
 * Wrap ESP-SR (WakeNet + MultiNet) chạy trên FreeRTOS Core 1.
 * Feed audio từ INMP441, gọi callback khi phát hiện lệnh.
 */
#ifndef VOICE_ENGINE_H
#define VOICE_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

// ─── Command map ─────────────────────────────────────────────────────────────
// Command IDs 0–5  → Turn ON  device 1–6
// Command IDs 10–15 → Turn OFF device 1–6
// Command ID 20    → Wake Word
#define VOICE_CMD_ON_BASE    0
#define VOICE_CMD_OFF_BASE   10
#define VOICE_CMD_COUNT      6
#define VOICE_CMD_WAKE       20

// ─── Callback type ───────────────────────────────────────────────────────────
typedef void (*voice_cmd_cb_t)(int cmd_id, const char* cmd_name);

// ─── Lifecycle ───────────────────────────────────────────────────────────────
bool voice_engine_init();
void voice_engine_set_callback(voice_cmd_cb_t cb);

// ─── Training API ────────────────────────────────────────────────────────────
/**
 * Start training mode for a specific GPIO command.
 * Will expect 3 voice samples.
 */
void voice_engine_start_training(int gpio_idx, bool on_cmd);
void voice_engine_stop_training(bool save);

/**
 * Returns: 0 (not training), 1, 2, 3 (samples captured), 4 (done/saving)
 */
int voice_engine_get_training_progress();
int voice_engine_get_last_rms();
void voice_engine_force_finalize();

// ─── FreeRTOS task entry (chạy trên Core 1) ─────────────────────────────────
void voice_engine_task(void* arg);

// ─── State query ─────────────────────────────────────────────────────────────
bool voice_engine_is_listening();
const char* voice_engine_cmd_name(int cmd_id);
bool voice_engine_has_command(int gpio_idx, bool on_cmd);

#endif // VOICE_ENGINE_H
