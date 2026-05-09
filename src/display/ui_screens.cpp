/**
 * ui_screens.cpp — High-level screen compositions
 * IoT Voice Command System
 *
 * Layout OLED 128×64:
 *   Row 0  [y=10] — Tiêu đề / trạng thái
 *   Row 1  [y=11] — Đường kẻ ngang separator
 *   Row 2+ [y=24+] — Nội dung chính
 */
#include "ui_screens.h"
#include "display_driver.h"
#include <stdio.h>
#include <string.h>

// ─── Internal helpers ────────────────────────────────────────────────────────

/**
 * Vẽ thanh tiêu đề với text căn giữa và đường kẻ phía dưới.
 */
static void _draw_header(const char* title) {
    display_font_medium();
    uint8_t tw = display_str_width(title);
    uint8_t tx = (128 - tw) / 2;
    display_str(tx, 10, title);
    display_hline(0, 13, 128);
}

/**
 * Vẽ text căn giữa tại y cho trước.
 */
static void _draw_centered(uint8_t y, const char* text) {
    display_font_medium();
    uint8_t tw = display_str_width(text);
    uint8_t tx = (128 - tw) / 2;
    display_str(tx, y, text);
}

/**
 * Vẽ animation dots theo tick (0='.', 1='..', 2='...')
 * Dùng millis() để tự động cycle.
 */
static const char* _anim_dots() {
    static const char* frames[] = { ".", "..", "..." };
    return frames[(millis() / 500) % 3];
}

// ─── Screens ─────────────────────────────────────────────────────────────────

/**
 * Splash screen: boot logo + tên ứng dụng.
 */
void ui_screen_splash() {
    display_clear();

    // Border
    display_rect(0, 0, 128, 64);

    // App name
    display_font_large();
    _draw_centered(24, "Voice IoT");

    // Subtitle
    display_font_small();
    _draw_centered(38, "ESP32-S3 Controller");

    // Version
    display_font_small();
    _draw_centered(54, "v1.0  Initializing...");

    display_flush();
}

/**
 * Ready screen: hệ thống sẵn sàng nhận lệnh.
 */
void ui_screen_ready() {
    display_clear();

    _draw_header("READY");

    // Icon mic (đơn giản hóa bằng ký tự)
    display_font_large();
    _draw_centered(36, "[MIC]");

    display_font_small();
    _draw_centered(52, "Say a command...");

    display_flush();
}

/**
 * Listening screen: đang lắng nghe, hiển thị animation.
 */
void ui_screen_listening() {
    display_clear();

    _draw_header("LISTENING");

    // Waveform giả — 3 thanh nhảy theo thời gian
    const uint8_t cx    = 64;
    const uint8_t bases = 48;
    uint32_t t = millis();
    int8_t h1 = 4 + (t / 120) % 12;
    int8_t h2 = 4 + (t / 90)  % 16;
    int8_t h3 = 4 + (t / 150) % 10;

    display_box(cx - 18, bases - h1, 6, h1 * 2);
    display_box(cx -  4, bases - h2, 6, h2 * 2);
    display_box(cx + 10, bases - h3, 6, h3 * 2);

    display_font_small();
    char buf[20];
    snprintf(buf, sizeof(buf), "Listening%s", _anim_dots());
    _draw_centered(62, buf);

    display_flush();
}

/**
 * Command screen: hiển thị lệnh vừa nhận.
 */
void ui_screen_command(const char* cmd_name) {
    display_clear();

    _draw_header("COMMAND");

    display_font_medium();
    _draw_centered(34, cmd_name ? cmd_name : "Unknown");

    display_font_small();
    _draw_centered(52, "Executing...");

    display_flush();
}

/**
 * Dashboard screen: grid 3×2 trạng thái 6 relay.
 *
 * Layout (128×64):
 *   Header y=10, sep y=13
 *   Grid từ y=16:
 *     Col0 x=2,  Col1 x=44, Col2 x=86
 *     Row0 y=28, Row1 y=52
 *   Mỗi ô: [R1 ON] hoặc [R1 --]
 */
void ui_screen_dashboard(const bool* relay_states, int n) {
    display_clear();

    _draw_header("DASHBOARD");

    const uint8_t col_x[3] = { 2, 44, 86 };
    const uint8_t row_y[2] = { 28, 50 };

    for (int i = 0; i < n && i < 6; i++) {
        int col = i % 3;
        int row = i / 3;

        uint8_t x = col_x[col];
        uint8_t y = row_y[row];

        // Vẽ viền ô
        display_rect(x - 1, y - 12, 40, 16);

        // Tô nền nếu ON
        if (relay_states[i]) {
            display_box(x - 1, y - 12, 40, 16);
        }

        // Text: "D1 ON" hoặc "D1 --"
        char buf[8];
        snprintf(buf, sizeof(buf), "D%d %s", i + 1, relay_states[i] ? "ON" : "--");

        display_font_small();

        if (relay_states[i]) {
            // Text trắng trên nền đen: dùng XOR draw mode
            display_get().setDrawColor(0);
            display_str(x + 1, y, buf);
            display_get().setDrawColor(1);
        } else {
            display_str(x + 1, y, buf);
        }
    }

    display_flush();
}

/**
 * Error screen: hiển thị thông báo lỗi.
 */
void ui_screen_error(const char* msg) {
    display_clear();

    // Header với dấu cảnh báo
    display_font_medium();
    _draw_centered(10, "! ERROR !");
    display_hline(0, 13, 128);

    // Message — tối đa 3 dòng, mỗi dòng ~21 ký tự
    display_font_small();
    if (!msg || !*msg) {
        _draw_centered(36, "Unknown error");
        display_flush();
        return;
    }

    const int  MAX_COLS = 21;
    const int  LINE_H   = 12;
    uint8_t    y        = 26;
    char       line[MAX_COLS + 1];
    int        col      = 0;
    const char* p       = msg;

    while (*p && y < 62) {
        if (*p == '\n' || col >= MAX_COLS) {
            line[col] = '\0';
            uint8_t tw = display_str_width(line);
            display_str((128 - tw) / 2, y, line);
            y  += LINE_H;
            col = 0;
            if (*p == '\n') { p++; continue; }
        }
        line[col++] = *p++;
    }
    if (col > 0) {
        line[col] = '\0';
        uint8_t tw = display_str_width(line);
        display_str((128 - tw) / 2, y, line);
    }

    display_flush();
}
