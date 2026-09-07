#pragma once
#include <Arduino.h>

/**
 * pin_config.h - Quản lý chân GPIO tập trung theo chuẩn Rule 3 & 11 trong GEMINI.md
 * 
 * Hỗ trợ 2 mẫu board phổ biến của dòng ESP32-S3 3.5 Inch (320x480):
 *  - BOARD_JC3248W535 (Mặc định - Bản mạch đen Guition / Jingcai dùng AXS15231B QSPI)
 *  - BOARD_SUNTON_S3  (Bản mạch vàng/đen Sunton dùng ST7796 SPI)
 */

#define BOARD_JC3248W535
// #define BOARD_SUNTON_S3

#if defined(BOARD_JC3248W535)
  #define BOARD_NAME "Guition JC3248W535 (ESP32-S3 3.5\")"

  // ---------------- MÀN HÌNH (QSPI AXS15231B) ----------------
  #define PIN_TFT_QSPI_CS     45
  #define PIN_TFT_QSPI_SCK    47
  #define PIN_TFT_QSPI_D0     21
  #define PIN_TFT_QSPI_D1     48
  #define PIN_TFT_QSPI_D2     40
  #define PIN_TFT_QSPI_D3     39
  #define PIN_TFT_BL          1    // Đèn nền LCD (PWM)
  #define PIN_TFT_RST         -1   // Reset (nối với EN/RST chung)
  #define TFT_WIDTH           320  // Chiều ngang chuẩn Portrait
  #define TFT_HEIGHT          480  // Chiều dọc chuẩn Portrait
  #define TFT_ROTATION        0    // 0 = Native Portrait chuẩn

  // ---------------- CẢM ỨNG (I2C) ----------------
  #define PIN_TOUCH_I2C_SDA   4
  #define PIN_TOUCH_I2C_SCL   8
  #define PIN_TOUCH_INT       3
  #define PIN_TOUCH_RST       -1   // Không dùng chân 2 (GPIO 2 dành cho I2S LRCK)

  // ---------------- ÂM THANH (I2S DAC / AMP) ----------------
  #define PIN_I2S_BCLK        42
  #define PIN_I2S_LRCK        2
  #define PIN_I2S_DOUT        41

  // ---------------- THẺ NHỚ MICROSD (SPI) ----------------
  #define PIN_SD_CS           10   // Hoặc 11 trên một số revision
  #define PIN_SD_MOSI         11   // Chân MOSI cho SD
  #define PIN_SD_MISO         13   // Chân MISO cho SD
  #define PIN_SD_SCK          12   // Chân SCK cho SD

#elif defined(BOARD_SUNTON_S3)
  #define BOARD_NAME "Sunton ESP32-S3-3248S035C"

  // ---------------- MÀN HÌNH (ST7796 SPI) ----------------
  #define PIN_TFT_CS          45
  #define PIN_TFT_DC          48
  #define PIN_TFT_MOSI        13
  #define PIN_TFT_SCK         12
  #define PIN_TFT_BL          1
  #define PIN_TFT_RST         -1

  // ---------------- CẢM ỨNG (I2C GT911) ----------------
  #define PIN_TOUCH_I2C_SDA   4
  #define PIN_TOUCH_I2C_SCL   8
  #define PIN_TOUCH_INT       3
  #define PIN_TOUCH_RST       2

  // ---------------- ÂM THANH (I2S) ----------------
  #define PIN_I2S_BCLK        42
  #define PIN_I2S_LRCK        2
  #define PIN_I2S_DOUT        41

  // ---------------- THẺ NHỚ MICROSD (SPI) ----------------
  #define PIN_SD_CS           10
  #define PIN_SD_MOSI         11
  #define PIN_SD_MISO         13
  #define PIN_SD_SCK          12
#endif

// Buttons & Built-in LED (nếu có)
#define PIN_BUTTON_BOOT       0