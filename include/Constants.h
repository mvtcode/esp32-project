#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <Arduino.h>

// --- HARDWARE PIN DEFINITIONS ---
// I2S (PCM5102)
#define I2S_BCK 26
#define I2S_WS 27
#define I2S_DOUT 25

// OLED (I2C)
#define OLED_SDA 21
#define OLED_SCL 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// SD Card (SPI)
#define SD_CS 5
#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCK 18

// Buttons
#define BTN_MODE 32
#define BTN_PLAY 33
#define BTN_PREV_VOL_MINUS 34 // Pin 34: Vol- (Short) / Prev (Long)
#define BTN_NEXT_VOL_PLUS 35  // Pin 35: Vol+ (Short) / Next (Long)

// --- CONSTANTS & ENUMS ---
enum AudioMode { MODE_BT = 0, MODE_MP3, MODE_RADIO };

enum InputAction {
  ACTION_NONE = 0,
  ACTION_NEXT_MODE,
  ACTION_DEEP_SLEEP,
  ACTION_TOGGLE_PLAY,
  ACTION_PREV_TRACK,
  ACTION_VOL_DOWN,
  ACTION_NEXT_TRACK,
  ACTION_VOL_UP
};

#define DEBOUNCE_DELAY 50
#define RADIO_URL                                                              \
  "http://stream.radioreklama.bg:80/radio1rock128" // Example URL

#endif // CONSTANTS_H
