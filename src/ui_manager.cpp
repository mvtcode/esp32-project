#include "ui_manager.h"
#include "hardware_config.h"
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// ─── SH1106 128x64 OLED — Software I2C on configurable pins ──────────────────
// Using full-frame buffer (_F) for smooth redraws on ESP32-S3 (8MB PSRAM)
static U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(
    U8G2_R0,
    /* clock */ I2C_SCL,
    /* data  */ I2C_SDA,
    /* reset */ U8X8_PIN_NONE
);

// ─── Public API ───────────────────────────────────────────────────────────────

void ui_init() {
    Wire.begin(I2C_SDA, I2C_SCL);
    u8g2.begin();
    u8g2.setContrast(200);

    // Splash screen
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(20, 28, "Voice IoT");
    u8g2.drawStr(20, 44, "Initializing...");
    u8g2.sendBuffer();
}

void ui_update() {
    // No-op for U8g2 direct mode (push happens in show_message)
    // Reserved for future animation tick
}

void ui_show_message(const char* msg) {
    if (!msg) return;

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);

    // Word-wrap: split on '\n' or auto-wrap at ~21 chars per line
    const int MAX_COLS  = 21;
    const int LINE_H    = 14;  // pixels between baselines
    int y = 20;

    char line[MAX_COLS + 1];
    int  col  = 0;
    const char* p = msg;

    while (*p) {
        if (*p == '\n' || col >= MAX_COLS) {
            line[col] = '\0';
            u8g2.drawStr(2, y, line);
            y += LINE_H;
            col = 0;
            if (*p == '\n') { p++; continue; }
        }
        line[col++] = *p++;
    }
    if (col > 0) {
        line[col] = '\0';
        u8g2.drawStr(2, y, line);
    }

    u8g2.sendBuffer();
}

void ui_set_device_status(int index, bool status) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Dev%d: %s", index + 1, status ? "ON " : "OFF");
    ui_show_message(buf);
}
