#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_GFX.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <stdint.h>

class CustomMatrixPanel : public Adafruit_GFX {
private:
  MatrixPanel_I2S_DMA *dma;

public:
  CustomMatrixPanel(MatrixPanel_I2S_DMA *dma_panel, int16_t w, int16_t h) 
    : Adafruit_GFX(w, h), dma(dma_panel) {}

  uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }

  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (x < 0 || x >= 128 || y < 0 || y >= 96) return;

    int16_t phys_x = 0;
    int16_t phys_y = 0;

    // Serpentine mapping for 6 panels (128x96 logic -> 384x32 physical)
    // Panel connections:
    // P1 (top-left) -> P2 (top-right)
    //                    |
    // P4 (mid-left) <- P3 (mid-right)
    //   |
    // P5 (bot-left) -> P6 (bot-right)
    if (y < 32) {
      // Row 1: P1 and P2
      phys_x = x;
      phys_y = y;
    } 
    else if (y < 64) {
      // Row 2: P4 and P3 (reversed direction)
      if (x >= 64) {
        // P3 (Mid-Right)
        phys_x = 128 + (x - 64);
      } else {
        // P4 (Mid-Left)
        phys_x = 192 + x;
      }
      phys_y = y - 32;
    } 
    else {
      // Row 3: P5 and P6
      if (x < 64) {
        // P5 (Bot-Left)
        phys_x = 256 + x;
      } else {
        // P6 (Bot-Right)
        phys_x = 320 + (x - 64);
      }
      phys_y = y - 64;
    }

    dma->drawPixel(phys_x, phys_y, color);
  }

  void fillScreen(uint16_t color) override {
    dma->fillScreen(color);
  }
};

// Global variables
extern MatrixPanel_I2S_DMA *dma_display;
extern CustomMatrixPanel *virtual_display;
extern uint8_t globalHue;

// Initialize display with configuration
void initDisplay();

// Color conversion utility
uint16_t hsvToRgb565(uint8_t hue, uint8_t sat, uint8_t val);

// Custom Vietnamese character drawing functions
void drawThu(int16_t x, int16_t y, uint16_t color);
void drawACircumflexDotBelow(int16_t x, int16_t y, uint16_t color);

// Weather icon drawing functions
void drawOutdoorIcon(int16_t x, int16_t y, uint16_t color); // Tree icon
void drawIndoorIcon(int16_t x, int16_t y, uint16_t color);  // Home icon

// Display brightness control
void setDisplayBrightness(uint8_t brightness); // Set brightness 10-100%

#endif // DISPLAY_H

