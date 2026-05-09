/**
 * ui_manager.h — UI State Machine Manager
 * IoT Voice Command System
 */
#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <stdint.h>

// UI Button Events
typedef enum {
    BTN_EVENT_UP,
    BTN_EVENT_DOWN,
    BTN_EVENT_ENTER,
    BTN_EVENT_BACK,
    BTN_EVENT_BOOT,
    BTN_EVENT_MIC
} UIButtonEvent;

// Screen IDs
typedef enum {
    SCREEN_HOME = 0,
    SCREEN_GPIO_LIST = 1,
    SCREEN_GPIO_DETAIL = 2,
    SCREEN_VOICE_CHANGE = 3
} ScreenID;

// Screen Configuration
typedef struct {
    void (*on_up)();
    void (*on_down)();
    void (*on_enter)();
    void (*on_back)();
    void (*on_boot)();
    void (*on_mic)();
    void (*draw_ui)();
} ScreenConfig;

// Setup and Loop
void ui_manager_init();
void ui_manager_loop();

// Handle a button press event
void ui_manager_handle_button(UIButtonEvent event);

// Trigger a UI refresh explicitly
void ui_manager_request_refresh();

// Set the command to display (used when a voice command is recognized)
void ui_manager_set_command(const char* cmd_name);
ScreenID ui_manager_get_current_screen();
bool ui_manager_is_sleeping();

#endif // UI_MANAGER_H
