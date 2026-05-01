/**
 * ui_manager.h — Display abstraction layer
 * IoT Voice Command System
 * 
 * Uses U8g2 directly (no LVGL) for SH1106 128x64 OLED.
 * Simple, reliable, no header conflicts.
 */
#ifndef UI_MANAGER_H
#define UI_MANAGER_H

void ui_init();
void ui_update();
void ui_show_message(const char* msg);
void ui_set_device_status(int index, bool status);

#endif
