#ifndef DISPLAY_H
#define DISPLAY_H

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <stdint.h>

// Global variables
extern MatrixPanel_I2S_DMA *dma_display;
extern uint8_t globalHue;

// Initialize display with configuration
void initDisplay();

// Color conversion utility
uint16_t hsvToRgb565(uint8_t hue, uint8_t sat, uint8_t val);

// Custom Vietnamese character drawing functions
void drawThu(int16_t x, int16_t y, uint16_t color);
void drawACircumflexDotBelow(int16_t x, int16_t y, uint16_t color);

// Weather icon drawing functions
void drawOutdoorIcon(int16_t x, int16_t y, uint16_t color); // 3 horizontal bars (≡)
void drawIndoorIcon(int16_t x, int16_t y, uint16_t color);  // Home icon

// Display brightness control
void setDisplayBrightness(uint8_t brightness); // Set brightness 10-100%

#endif

