/**
 * display_driver.cpp — U8g2 low-level display implementation
 * IoT Voice Command System
 */
#include "display_driver.h"
#include "../hardware_config.h"
#include <Wire.h>
#include <Arduino.h>

// ─── U8g2 instance: SH1106 128x64, Full buffer, Hardware I2C ─────────────────
static U8G2_SH1106_128X64_NONAME_F_HW_I2C _u8g2(
    U8G2_R0,
    /* reset= */ U8X8_PIN_NONE,
    /* clock= */ OLED_SCL,
    /* data=  */ OLED_SDA
);

static bool _display_ok = false;

// ─── Internal: kiểm tra I2C device có respond không ──────────────────────────
static bool _i2c_scan(uint8_t addr) {
    Serial.printf("[DISP] Scanning 0x%02X... ", addr);
    Serial.flush();
    
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();
    
    if (error == 0) {
        Serial.println("FOUND");
        return true;
    } else if (error == 4) {
        Serial.println("UNKNOWN ERROR");
    } else {
        Serial.printf("NOT FOUND (error %d)\n", error);
    }
    Serial.flush();
    return false;
}

// ─── Lifecycle ───────────────────────────────────────────────────────────────

void display_init() {
    Serial.println("[DISP] Initializing Hardware I2C...");
    Serial.flush();

    // 1. Khởi tạo Hardware I2C
    // Nếu Bus already started, Wire.begin() sẽ reconfigure chân và clock.
    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(400000);
    Wire.setTimeOut(100); // 100ms timeout để tránh hang

    // 2. Scan I2C
    Serial.printf("[DISP] SDA=%d, SCL=%d\n", OLED_SDA, OLED_SCL);
    bool found = _i2c_scan(OLED_I2C_ADDR);
    if (!found) {
        found = _i2c_scan(0x3D); // Thử địa chỉ phụ
    }

    if (!found) {
        Serial.println("[DISP] OLED not detected. App will continue without UI.");
        _display_ok = false;
        return;
    }

    // 3. Init U8g2
    Serial.println("[DISP] Calling u8g2.begin()...");
    Serial.flush();
    
    _u8g2.begin();
    _u8g2.setContrast(200);
    _u8g2.clearBuffer();
    _u8g2.sendBuffer();
    _display_ok = true;

    Serial.println("[DISP] U8g2 initialization complete");
    Serial.flush();
}

bool display_is_ok() {
    return _display_ok;
}

// ─── Buffer control ──────────────────────────────────────────────────────────

void display_clear() {
    if (!_display_ok) return;
    _u8g2.clearBuffer();
}

void display_flush() {
    if (!_display_ok) return;
    _u8g2.sendBuffer();
}

// ─── Font helpers ────────────────────────────────────────────────────────────

void display_font_small() {
    _u8g2.setFont(u8g2_font_5x7_tf);
}

void display_font_medium() {
    _u8g2.setFont(u8g2_font_6x10_tf);
}

void display_font_large() {
    _u8g2.setFont(u8g2_font_8x13_tf);
}

// ─── Draw primitives ─────────────────────────────────────────────────────────

void display_str(uint8_t x, uint8_t y, const char* str) {
    if (str) _u8g2.drawStr(x, y, str);
}

void display_hline(uint8_t x, uint8_t y, uint8_t w) {
    _u8g2.drawHLine(x, y, w);
}

void display_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    _u8g2.drawFrame(x, y, w, h);
}

void display_box(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    _u8g2.drawBox(x, y, w, h);
}

void display_pixel(uint8_t x, uint8_t y) {
    _u8g2.drawPixel(x, y);
}

uint8_t display_str_width(const char* str) {
    if (!str) return 0;
    return (uint8_t)_u8g2.getStrWidth(str);
}

// ─── Raw access ──────────────────────────────────────────────────────────────

U8G2& display_get() {
    return _u8g2;
}
