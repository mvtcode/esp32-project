/**
 * relay_controller.h — GPIO relay/output management
 * IoT Voice Command System
 *
 * Quản lý trạng thái và điều khiển 6 relay/output GPIO.
 * State được lưu nội bộ, tránh đọc GPIO liên tục.
 */
#ifndef RELAY_CONTROLLER_H
#define RELAY_CONTROLLER_H

#include <stdbool.h>

// ─── Lifecycle ───────────────────────────────────────────────────────────────
void relay_init();

// ─── Control ─────────────────────────────────────────────────────────────────
void relay_set(int idx, bool on);    // idx: 0–5
void relay_toggle(int idx);
void relay_all_off();

// ─── Query ───────────────────────────────────────────────────────────────────
bool relay_get(int idx);
const bool* relay_get_all();         // trả về pointer tới array trạng thái [NUM_RELAYS]
int  relay_count();                  // trả về NUM_RELAYS
const char* relay_get_alias(int idx); // trả về tên định danh (alias) từ config

#endif // RELAY_CONTROLLER_H
