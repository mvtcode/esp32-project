#pragma once

// ==============================================================================
// ESP32 CYD 3.5" (ESP32-3248S035) Hardware Pin Configuration
// ==============================================================================

// 1. LED & User Buttons
#define PIN_LED_BUILTIN     2
#define PIN_BUTTON_BOOT     0

#ifndef LED_BUILTIN
  #define LED_BUILTIN PIN_LED_BUILTIN
#endif

// 2. ST7796 Display Pins (Dedicated HSPI Bus)
#define PIN_TFT_MOSI        13
#define PIN_TFT_MISO        12
#define PIN_TFT_SCLK        14
#define PIN_TFT_CS          15
#define PIN_TFT_DC          2
#define PIN_TFT_RST         -1  // Tied to EN / Reset pin
#define PIN_TFT_BL          27  // Backlight PWM / GPIO

// 3. XPT2046 Resistive Touch Pins
#define PIN_TOUCH_CS        33

// 4. MicroSD Card Pins (Dedicated VSPI Bus)
#define PIN_SD_CS           5
#define PIN_SD_MOSI         23
#define PIN_SD_MISO         19
#define PIN_SD_SCLK         18

// 5. Audio DAC Output Pin (Internal 8-bit DAC Channel 2)
#define PIN_AUDIO_DAC       26