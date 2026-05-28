#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_GFX.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <stdint.h>
#include "Verdana_Vietnamese10pt.h"
#include "Verdana_Vietnamese12pt.h"
#include "Verdana_Bold14pt.h"
#include "Verdana_Bold18pt.h"
#include "ClockFont24px.h"

// Offset to compensate for physical alignment vertical offset of the left panels.
// Positive values shift the left half down. Adjust this value (e.g., 0, 1, 2) to align.
const int LEFT_PANEL_Y_OFFSET = 2;

class CustomMatrixPanel : public Adafruit_GFX {
private:
  MatrixPanel_I2S_DMA *dma;

public:
  CustomMatrixPanel(MatrixPanel_I2S_DMA *dma_panel, int16_t w, int16_t h) 
    : Adafruit_GFX(w, h), dma(dma_panel) {}

  uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return dma->color565(r, g, b);
  }

  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (x < 0 || x >= 128 || y < 0 || y >= 96) return;

    int16_t phys_x = 0;
    int16_t phys_y = 0;

    // Serpentine mapping for 6 panels (128x96 logic -> 384x32 physical)
    // Physical Panel Layout & Signal Flow (front view):
    // ESP32 ---> [Panel 1 (top-left, normal)]  ---> [Panel 2 (top-right, normal)]
    //                                                   |
    //                                                   v
    //            [Panel 4 (mid-left, rotated)]  <--- [Panel 3 (mid-right, rotated)]
    //               |
    //               v
    //            [Panel 5 (bot-left, normal)]   ---> [Panel 6 (bot-right, normal)]
    if (y < 32) {
      // Row 0: Panel 6 (slot 5, left), Panel 5 (slot 4, right) - both rotated 180
      phys_x = 383 - x;
      phys_y = 31 - y;
    } 
    else if (y < 64) {
      // Row 1: Panel 3 (slot 2, left), Panel 4 (slot 3, right) - both normal (0 deg)
      phys_x = 128 + x;
      phys_y = y - 32;
    } 
    else {
      // Row 2: Panel 2 (slot 1, left), Panel 1 (slot 0, right) - both rotated 180
      phys_x = 127 - x;
      phys_y = 95 - y;
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

