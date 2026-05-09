/**
 * ui_screens.h — High-level screen compositions
 * IoT Voice Command System
 *
 * Mỗi hàm vẽ hoàn chỉnh một màn hình lên buffer và flush.
 * Gọi display_driver primitives, không gọi U8g2 trực tiếp.
 */
#ifndef UI_SCREENS_H
#define UI_SCREENS_H

#include <stdbool.h>

// ─── Screens ─────────────────────────────────────────────────────────────────
void ui_screen_splash();
void ui_screen_ready();
void ui_screen_listening();
void ui_screen_command(const char* cmd_name);
void ui_screen_dashboard(const bool* relay_states, int n);
void ui_screen_error(const char* msg);

#endif // UI_SCREENS_H
