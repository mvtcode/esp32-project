/**
 * display_driver.h — U8g2 low-level display abstraction
 * IoT Voice Command System
 *
 * Khởi tạo U8g2 cho SH1106 128x64 qua Software I2C.
 * Cung cấp các primitive draw để ui_screens sử dụng.
 * Không chứa logic màn hình cụ thể.
 */
#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <stdint.h>
#include <U8g2lib.h>

// ─── Lifecycle ───────────────────────────────────────────────────────────────
void display_init();
bool display_is_ok();  // false nếu OLED không phát hiện trên I2C bus

// ─── Buffer control ──────────────────────────────────────────────────────────
void display_clear();
void display_flush();   // gửi buffer lên màn hình (sendBuffer)

// ─── Font helpers ────────────────────────────────────────────────────────────
void display_font_small();    // u8g2_font_5x7_tf  — phụ đề nhỏ
void display_font_medium();   // u8g2_font_6x10_tf — nội dung chính
void display_font_large();    // u8g2_font_8x13_tf — tiêu đề

// ─── Draw primitives ─────────────────────────────────────────────────────────
void display_str(uint8_t x, uint8_t y, const char* str);
void display_hline(uint8_t x, uint8_t y, uint8_t w);
void display_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void display_box(uint8_t x, uint8_t y, uint8_t w, uint8_t h);   // filled
void display_pixel(uint8_t x, uint8_t y);
uint8_t display_str_width(const char* str);  // chiều rộng chuỗi (px) với font hiện tại

// ─── Raw access (nếu ui_screens cần gọi trực tiếp U8g2) ─────────────────────
U8G2& display_get();

#endif // DISPLAY_DRIVER_H
