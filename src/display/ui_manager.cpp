/**
 * ui_manager.cpp — Core UI State Machine
 * IoT Voice Command System
 */
#include "ui_manager.h"
#include "display_driver.h"
#include "../control/relay_controller.h"
#include "../voice/voice_engine.h"
#include <Arduino.h>
#include <stdio.h>

// ─── Forward Declarations ────────────────────────────────────────────────────
void go_screen_home();
void go_screen_gpio_list();
void go_screen_gpio_detail();
void go_screen_voice_change();

void home_on_enter();
void home_on_boot();
void home_on_mic();

void list_on_up();
void list_on_down();
void list_on_enter();
void list_on_back();

void detail_on_up();
void detail_on_down();
void detail_on_enter();
void detail_on_back();

void change_on_enter();
void change_on_back();
void change_on_mic();

void draw_home_screen();
void draw_gpio_list_screen();
void draw_gpio_detail_screen();
void draw_voice_change_screen();

// ─── Global State ────────────────────────────────────────────────────────────

static ScreenID g_current_screen = SCREEN_HOME;
static bool g_ui_needs_refresh = true;
static uint32_t g_last_activity_time = 0;
static bool g_sleep_mode = false;
static char g_last_cmd[32] = {0};
static uint32_t g_cmd_show_start = 0;

// State for GPIO List
static const int MAX_LIST_ITEMS = 7;
static int g_list_cursor = 0;

// State for GPIO Detail
static int g_detail_cursor = 0;
static bool g_voice_enabled[MAX_LIST_ITEMS] = {true, true, true, false, true, true, true}; // Mock state

// State for Voice Change
static int g_voice_change_step = 0; // 0: start, 1: 1/3, 2: 2/3, 3: 3/3, 4: done

// ─── Screen Configs ──────────────────────────────────────────────────────────

const ScreenConfig app_screens[] = {
    // [0] SCREEN_HOME
    {
        .on_up = nullptr,
        .on_down = nullptr,
        .on_enter = go_screen_gpio_list,
        .on_back = [](){ if(!g_sleep_mode) { g_sleep_mode = true; g_ui_needs_refresh = true; } },
        .on_boot = home_on_boot,
        .on_mic = home_on_mic,
        .draw_ui = draw_home_screen
    },
    // [1] SCREEN_GPIO_LIST
    {
        .on_up = list_on_up,
        .on_down = list_on_down,
        .on_enter = list_on_enter,
        .on_back = go_screen_home,
        .on_boot = nullptr, // Don't jump to home if already awake
        .on_mic = nullptr,
        .draw_ui = draw_gpio_list_screen
    },
    // [2] SCREEN_GPIO_DETAIL
    {
        .on_up = detail_on_up,
        .on_down = detail_on_down,
        .on_enter = detail_on_enter,
        .on_back = go_screen_gpio_list,
        .on_boot = nullptr, // Don't jump to home if already awake
        .on_mic = nullptr,
        .draw_ui = draw_gpio_detail_screen
    },
    // [3] SCREEN_VOICE_CHANGE
    {
        .on_up = nullptr,
        .on_down = nullptr,
        .on_enter = change_on_enter,
        .on_back = change_on_back,
        .on_boot = nullptr, // Don't jump to home if already awake
        .on_mic = nullptr,
        .draw_ui = draw_voice_change_screen
    }
};

// ─── Helpers ─────────────────────────────────────────────────────────────────
ScreenID ui_manager_get_current_screen() {
    return g_current_screen;
}

bool ui_manager_is_sleeping() {
    return g_sleep_mode;
}

static void reset_activity() {
    g_last_activity_time = millis();
}

static void wake_up() {
    g_last_activity_time = millis();
    if (g_sleep_mode) {
        g_sleep_mode = false;
        display_get().setContrast(200);
        g_ui_needs_refresh = true;
        Serial.println("[UI] Device Woken Up!");
    }
}

static bool check_voice_cmd_exists(int gpio_idx, bool on_cmd) {
    return voice_engine_has_command(gpio_idx, on_cmd);
}

// ─── Navigation Actions ──────────────────────────────────────────────────────

void go_screen_home() {
    g_current_screen = SCREEN_HOME;
    g_ui_needs_refresh = true;
}

void go_screen_gpio_list() {
    g_current_screen = SCREEN_GPIO_LIST;
    g_ui_needs_refresh = true;
}

void go_screen_gpio_detail() {
    g_current_screen = SCREEN_GPIO_DETAIL;
    g_ui_needs_refresh = true;
}

void go_screen_voice_change() {
    g_current_screen = SCREEN_VOICE_CHANGE;
    g_voice_change_step = 0;
    g_ui_needs_refresh = true;
}

static void draw_visualizer(uint8_t bases, uint8_t cx) {
    int rms = voice_engine_get_last_rms();
    if (rms > 200) { // Only show if there's actual sound signal
        // Scale RMS to bar height (e.g., 600 -> 12px)
        int h = rms / 50; 
        if (h > 24) h = 24; // Cap height
        
        // Draw 3 dynamic bars responding to the same volume with slight variations
        display_box(cx - 15, bases - (h/2), 6, h);
        display_box(cx - 2,  bases - (h*0.8), 6, h*1.6);
        display_box(cx + 11, bases - (h/1.5), 6, h/1.5 * 2);
    } else {
        // Flat line if silent
        display_hline(cx - 20, bases, 40);
    }
}

// ─── Screen 0: HOME ──────────────────────────────────────────────────────────

void home_on_boot() {
    g_sleep_mode = false;
    g_ui_needs_refresh = true;
}

void home_on_mic() {
    if (g_sleep_mode) {
        g_sleep_mode = false;
    }
    // Handle command input
    g_ui_needs_refresh = true;
}

static void draw_centered(uint8_t y, const char* text) {
    uint8_t tw = display_str_width(text);
    uint8_t tx = (128 - tw) / 2;
    display_str(tx, y, text);
}

void draw_home_screen() {
    display_clear();

    if (g_sleep_mode) {
        // Sleep screen
        display_font_medium();
        draw_centered(10, "SLEEP MODE");
        display_hline(0, 13, 128);
        display_font_small();
        draw_centered(46, "Hay noi 'hey esp'");
        draw_centered(56, "hoac nhan BOOT");
    } else {
        if (g_cmd_show_start != 0 && (millis() - g_cmd_show_start < 2000)) {
            // Show command popup
            display_font_medium();
            draw_centered(10, "> COMMAND <");
            display_hline(0, 13, 128);
            draw_centered(34, g_last_cmd);
        } else {
            g_cmd_show_start = 0;
            display_font_medium();
            draw_centered(10, "LISTENING");
            display_hline(0, 13, 128);

            draw_visualizer(44, 64);

            display_font_small();
            if (voice_engine_is_processing()) {
                draw_centered(60, "Processing...");
            } else if (voice_engine_is_speaking()) {
                draw_centered(60, "Listening...");
            } else {
                draw_centered(60, "Ready");
            }
        }
    }
    display_flush();
}

// ─── Screen 1: GPIO LIST ─────────────────────────────────────────────────────

void list_on_up() {
    g_list_cursor--;
    if (g_list_cursor < 0) g_list_cursor = MAX_LIST_ITEMS - 1;
    g_ui_needs_refresh = true;
}

void list_on_down() {
    g_list_cursor++;
    if (g_list_cursor >= MAX_LIST_ITEMS) g_list_cursor = 0;
    g_ui_needs_refresh = true;
}

void list_on_enter() {
    if (g_list_cursor == 6) {
        // Wake word training
        voice_engine_start_training(VOICE_CMD_WAKE, true);
        go_screen_voice_change();
    } else {
        g_detail_cursor = 0;
        go_screen_gpio_detail();
    }
}

void draw_gpio_list_screen() {
    display_clear();
    display_font_medium();
    draw_centered(10, "GPIO SETTING");
    display_hline(0, 13, 128);

    display_font_small();
    
    // We can show ~4 lines
    int start_idx = g_list_cursor - 1;
    if (start_idx < 0) start_idx = 0;
    if (start_idx > MAX_LIST_ITEMS - 4) start_idx = MAX_LIST_ITEMS - 4;
    if (start_idx < 0) start_idx = 0;

    int y = 24;
    for (int i = 0; i < 4 && (start_idx + i) < MAX_LIST_ITEMS; i++) {
        int idx = start_idx + i;
        char buf[32];
        
        const char* name = (idx == 6) ? "WAKE WORD" : relay_get_alias(idx);
        bool has_cmd = (idx == 6) ? voice_engine_has_command(VOICE_CMD_WAKE, true) : (check_voice_cmd_exists(idx, true) || check_voice_cmd_exists(idx, false));
        const char* status = (idx == 6) ? (has_cmd ? "SET" : "--") : (has_cmd ? (relay_get(idx) ? "ON" : "OFF") : "--");

        snprintf(buf, sizeof(buf), "%s [%s] %s", 
            (idx == g_list_cursor) ? ">" : " ",
            status,
            name);
        
        if (idx == g_list_cursor) {
            // Draw white box
            display_get().setDrawColor(1);
            display_box(0, y - 9, 128, 11);
            
            // Draw black text over white box
            display_get().setDrawColor(0);
            display_str(2, y, buf);
            
            // Restore white color
            display_get().setDrawColor(1);
        } else {
            display_str(2, y, buf);
        }
        y += 12;
    }

    display_flush();
}

// ─── Screen 2: GPIO DETAIL ───────────────────────────────────────────────────

void detail_on_up() {
    g_detail_cursor--;
    if (g_detail_cursor < 0) g_detail_cursor = 3;
    g_ui_needs_refresh = true;
}

void detail_on_down() {
    g_detail_cursor++;
    if (g_detail_cursor > 3) g_detail_cursor = 0;
    g_ui_needs_refresh = true;
}

void detail_on_enter() {
    if (!g_voice_enabled[g_list_cursor] && g_detail_cursor != 3) {
        return; // Disabled!
    }

    if (g_detail_cursor == 0) {
        relay_toggle(g_list_cursor);
        g_ui_needs_refresh = true;
    } else if (g_detail_cursor == 1) { // Set ON voice
        voice_engine_start_training(g_list_cursor, true);
        go_screen_voice_change();
    } else if (g_detail_cursor == 2) { // Set OFF voice
        voice_engine_start_training(g_list_cursor, false);
        go_screen_voice_change();
    } else if (g_detail_cursor == 3) { // Toggle Disable
        g_voice_enabled[g_list_cursor] = !g_voice_enabled[g_list_cursor];
        g_ui_needs_refresh = true;
    }
}

void draw_gpio_detail_screen() {
    display_clear();
    display_font_medium();
    
    char header[32];
    snprintf(header, sizeof(header), "%s - %s", 
        relay_get_alias(g_list_cursor), 
        g_voice_enabled[g_list_cursor] ? (relay_get(g_list_cursor) ? "ON" : "OFF") : "--");
        
    draw_centered(10, header);
    display_hline(0, 13, 128);

    display_font_small();
    
    bool en = g_voice_enabled[g_list_cursor];
    
    char m0[32], m1[32], m2[32], m3[32];
    snprintf(m0, sizeof(m0), "%s Toggle Relay   %s", (g_detail_cursor==0) ? ">":" ", en ? "" : "[x]");
    snprintf(m1, sizeof(m1), "%s Voice ON (%s) %s", (g_detail_cursor==1) ? ">":" ", check_voice_cmd_exists(g_list_cursor, true)?"ok":"--", en ? "" : "[x]");
    snprintf(m2, sizeof(m2), "%s Voice OFF (%s)%s", (g_detail_cursor==2) ? ">":" ", check_voice_cmd_exists(g_list_cursor, false)?"ok":"--", en ? "" : "[x]");
    snprintf(m3, sizeof(m3), "%s %s", (g_detail_cursor==3) ? ">":" ", en ? "DISABLE Voice" : "ENABLE Voice");
    
    const char* lines[] = {m0, m1, m2, m3};
    int y = 24;
    for(int i=0; i<4; i++) {
        if (i == g_detail_cursor) {
            display_get().setDrawColor(1);
            display_box(0, y - 9, 128, 11);
            display_get().setDrawColor(0);
            display_str(2, y, lines[i]);
            display_get().setDrawColor(1);
        } else {
            display_str(2, y, lines[i]);
        }
        y += 12;
    }

    display_flush();
}

// ─── Screen 3: VOICE CHANGE ──────────────────────────────────────────────────

void change_on_back() {
    voice_engine_stop_training(false);
    if (g_list_cursor == 6) {
        go_screen_gpio_list();
    } else {
        go_screen_gpio_detail();
    }
}

void change_on_enter() {
    int progress = voice_engine_get_training_progress();
    
    if (progress > 0) {
        // Da thu xong -> Luu va Thoat
        voice_engine_stop_training(true);
        if (g_list_cursor == 6) {
            go_screen_gpio_list();
        } else {
            go_screen_gpio_detail();
        }
    } else {
        // Chua thu xong -> Thu ket thuc thu am ngay lap tuc
        voice_engine_force_finalize();
        g_ui_needs_refresh = true;
    }
}

void draw_voice_change_screen() {
    display_clear();
    display_font_medium();
    
    char header[32];
    snprintf(header, sizeof(header), "RECORD: %s", relay_get_alias(g_list_cursor));
    draw_centered(10, header);
    display_hline(0, 13, 128);

    display_font_small();
    
    int progress = voice_engine_get_training_progress();
    if (progress == 0) {
        draw_centered(32, "Dang ghi...");
        draw_centered(44, "(Hay noi lenh)");
        
        draw_visualizer(56, 64);
    } else {
        draw_centered(32, "Da thu am!");
        draw_centered(44, "ENTER: Luu | BACK: Huy");

        // Static visualizer (horizontal line)
        display_hline(64 - 20, 56, 40);
    }

    display_flush();
}

// ─── API ─────────────────────────────────────────────────────────────────────

void ui_manager_init() {
    g_current_screen = SCREEN_HOME;
    g_ui_needs_refresh = true;
    reset_activity();
}

void ui_manager_loop() {
    // Just prevent sleep if user is speaking, don't wake up!
    if (voice_engine_is_speaking() || voice_engine_is_processing()) {
        reset_activity(); 
    }

    if (millis() - g_last_activity_time > 30000 && !g_sleep_mode && g_current_screen == SCREEN_HOME) {
        g_sleep_mode = true;
        display_get().setContrast(1); // Dim the screen
        g_ui_needs_refresh = true;
    }
    
    if (g_cmd_show_start != 0 && millis() - g_cmd_show_start >= 2000) {
        g_ui_needs_refresh = true;
    }

    // Auto refresh animation for visualizer (10 FPS to prevent I2C blocking)
    if (!g_sleep_mode && (g_current_screen == SCREEN_HOME || g_current_screen == SCREEN_VOICE_CHANGE)) {
        static uint32_t last_viz_anim = 0;
        if (millis() - last_viz_anim > 100) {
            last_viz_anim = millis();
            g_ui_needs_refresh = true;
        }
    }

    if (g_ui_needs_refresh) {
        g_ui_needs_refresh = false;
        if (app_screens[g_current_screen].draw_ui) {
            app_screens[g_current_screen].draw_ui();
        }
    }
}

void ui_manager_handle_button(UIButtonEvent event) {
    wake_up(); // Any button press wakes up the device
    
    if (g_current_screen == SCREEN_HOME && event == BTN_EVENT_BOOT) {
        if (g_sleep_mode) {
            g_sleep_mode = false;
            g_ui_needs_refresh = true;
            Serial.println("[UI] Woke up by Wake Word/Boot button");
        } else {
            // If already awake, only navigate to home if the screen explicitly allows it
            if (app_screens[g_current_screen].on_boot) {
                app_screens[g_current_screen].on_boot();
            }
        }
        return;
    }

    // Wake up if sleeping and any button pressed
    if (g_sleep_mode) {
        g_sleep_mode = false;
        g_ui_needs_refresh = true;
        return;
    }

    switch (event) {
        case BTN_EVENT_UP:    if (app_screens[g_current_screen].on_up) app_screens[g_current_screen].on_up(); break;
        case BTN_EVENT_DOWN:  if (app_screens[g_current_screen].on_down) app_screens[g_current_screen].on_down(); break;
        case BTN_EVENT_ENTER: if (app_screens[g_current_screen].on_enter) app_screens[g_current_screen].on_enter(); break;
        case BTN_EVENT_BACK:  if (app_screens[g_current_screen].on_back) app_screens[g_current_screen].on_back(); break;
        case BTN_EVENT_MIC:   if (app_screens[g_current_screen].on_mic) app_screens[g_current_screen].on_mic(); break;
        default: break;
    }
}

void ui_manager_request_refresh() {
    g_ui_needs_refresh = true;
}


void ui_manager_set_command(const char* cmd_name) {
    reset_activity(); // Voice command counts as activity
    strncpy(g_last_cmd, cmd_name ? cmd_name : "", sizeof(g_last_cmd) - 1);
    g_cmd_show_start = millis();
    g_ui_needs_refresh = true;
}
