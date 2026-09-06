#pragma once

#include <Arduino.h>

/**
 * @file pin_config.h
 * @brief Quản lý tập trung toàn bộ cấu hình chân GPIO (Rule 3 & Rule 11 - GEMINI.md)
 * @details Định nghĩa phần cứng cho board ESP32-3248S035 (CYD 3.5" ST7796 + XPT2046)
 */

// ============================================================================
// 1. MÀN HÌNH TFT LCD (ST7796 SPI, 480x320 px)
// ============================================================================
#define PIN_TFT_MISO        12
#define PIN_TFT_MOSI        13
#define PIN_TFT_SCLK        14
#define PIN_TFT_CS          15
#define PIN_TFT_DC          2
#define PIN_TFT_RST         -1
#define PIN_TFT_BL          27  // Backlight PWM

// ============================================================================
// 2. CẢM ỨNG ĐIỆN TRỞ (Touch Controller - XPT2046)
// ============================================================================
#define PIN_TOUCH_CS        33
// Bus SPI dùng chung với TFT (MOSI=13, MISO=12, SCLK=14)

// ============================================================================
// 3. THẺ NHỚ MICRO SD (Dedicated VSPI bus)
// ============================================================================
#define PIN_SD_CS           5
#define PIN_SD_MOSI         23
#define PIN_SD_MISO         19
#define PIN_SD_SCLK         18

// Alias tương thích ngược
#define SD_CS_PIN           PIN_SD_CS
#define SD_MOSI_PIN         PIN_SD_MOSI
#define SD_MISO_PIN         PIN_SD_MISO
#define SD_SCLK_PIN         PIN_SD_SCLK

// ============================================================================
// 4. ÂM THANH & CẢM BIẾN NGOẠI VI
// ============================================================================
#define PIN_AUDIO_DAC       26  // Internal DAC Channel 2 (Speaker)
#define PIN_LDR_ADC         34  // Quang trở cảm biến ánh sáng (ADC1_CH6)
#define PIN_RGB_LED         21  // RGB LED Onboard
