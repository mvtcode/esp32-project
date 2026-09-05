#pragma once

// ==============================================================================
// ESP32-S3 2.8" (320x240) Hardware Pin Configuration (ES3N28P / ES3C28P)
// ==============================================================================

#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_ARCH_ESP32S3) || defined(BOARD_ESP32S3)

// 1. User Button & Built-in LED
#define PIN_BUTTON_BOOT     0
#define PIN_LED_BUILTIN     42
#define PIN_PA_ENABLE       1   // FM8002E Audio Amp Enable (Active LOW on ES3C28P)

#ifndef LED_BUILTIN
  #define LED_BUILTIN PIN_LED_BUILTIN
#endif

// 2. Display SPI Pins (ILI9341V / ST7789V 320x240)
#define PIN_TFT_MOSI        11
#define PIN_TFT_MISO        13
#define PIN_TFT_SCLK        12
#define PIN_TFT_CS          10
#define PIN_TFT_DC          46  // RS / Command-Data
#define PIN_TFT_RST         -1  // Tied to hardware reset
#define PIN_TFT_BL          45  // Backlight PWM control

// 3. Touch Pins
// FT6336G Capacitive Touch (I2C)
#define PIN_TOUCH_SDA       16
#define PIN_TOUCH_SCL       15
#define PIN_TOUCH_INT       17
#define PIN_TOUCH_RST       18

// (Lưu ý: Không dùng GPIO 33-37 trên ESP32-S3 vì là các chân bus Octal PSRAM)

// 4. MicroSD Card Pins (SDIO / SD_MMC 4-bit)
#define PIN_SD_CLK          38
#define PIN_SD_CMD          40
#define PIN_SD_D0           39
#define PIN_SD_D1           41
#define PIN_SD_D2           48
#define PIN_SD_D3           47

// 5. I2S Audio Bus Pins (ES8311 Codec / MAX98357A / Audio Amp)
#define PIN_I2S_BCLK        5   // Bit Clock
#define PIN_I2S_LRC         7   // Word Select / Left-Right Clock (WS)
#define PIN_I2S_DOUT        8   // Data Out (TX to DAC/Codec)
#define PIN_I2S_DIN         6   // Data In (RX from Mic)
#define PIN_I2S_MCLK        4   // Master Clock (optional)

// I2C Control Pins for ES8311 Audio Codec (Shared with Touch I2C)
#define PIN_I2C_SDA         16
#define PIN_I2C_SCL         15

#else // Fallback to classic ESP32 CYD 3.5" (ST7796 480x320)

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

#endif