#include "display.h"
#include <FastLED.h>

#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 1

// Global variables
MatrixPanel_I2S_DMA *dma_display = nullptr;
uint8_t globalHue = 0;

// Initialize LED Matrix display with HUB75 configuration
void initDisplay() {
  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);
  mxconfig.gpio.r1 = 25;
  mxconfig.gpio.g1 = 26;
  mxconfig.gpio.b1 = 27;
  mxconfig.gpio.r2 = 14;
  mxconfig.gpio.g2 = 12;
  mxconfig.gpio.b2 = 13;
  mxconfig.gpio.a = 23;
  mxconfig.gpio.b = 19;
  mxconfig.gpio.c = 5;
  mxconfig.gpio.d = 17;
  mxconfig.gpio.clk = 16;
  mxconfig.gpio.lat = 4;
  mxconfig.gpio.oe = 15;

  // Bật double-buffering: vẽ vào buffer ẩn, flip nguyên tử -> không nháy
  mxconfig.double_buff = true;

  // Tắt clock phase shift để giảm ghost pixel
  mxconfig.clkphase = false;

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  if (!dma_display->begin()) {
    Serial.println("Failed to initialize DMA Display!");
  }
  // Set temporary brightness during boot (will be overridden by config)
  dma_display->setBrightness8(50);  // Temporary low brightness
  dma_display->clearScreen();

  dma_display->setCursor(2, 8);
  dma_display->setTextColor(dma_display->color565(255, 0, 0));
  dma_display->print("BOOTING...");
  dma_display->setCursor(6, 20);
  dma_display->print("v 2.2.0");
  dma_display->flipDMABuffer(); // Flip để hiện màn hình boot
  delay(500);
}

// Convert HSV color to RGB565 format for LED matrix
uint16_t hsvToRgb565(uint8_t hue, uint8_t sat, uint8_t val) {
  CRGB rgb;
  CHSV hsv(hue, sat, val);
  hsv2rgb_rainbow(hsv, rgb);
  return dma_display->color565(rgb.r, rgb.g, rgb.b);
}

// Draw custom Vietnamese character "Thứ" (15x8 pixels)
void drawThu(int16_t x, int16_t y, uint16_t color) {
  // Draw "T"
  dma_display->drawLine(x, y, x + 4, y, color);         // Top horizontal
  dma_display->drawLine(x + 2, y, x + 2, y + 6, color); // Vertical

  // Draw "h"
  dma_display->drawLine(x + 6, y, x + 6, y + 6, color); // Left vertical
  dma_display->drawPixel(x + 7, y + 3, color);          // Curve
  dma_display->drawPixel(x + 8, y + 4, color);
  dma_display->drawPixel(x + 8, y + 5, color);
  dma_display->drawPixel(x + 8, y + 6, color);

  // Draw "ư" (u + hook)
  dma_display->drawLine(x + 10, y + 3, x + 10, y + 6, color); // Left vertical
  dma_display->drawLine(x + 13, y + 3, x + 13, y + 6, color); // Right vertical
  dma_display->drawPixel(x + 11, y + 6, color);               // Bottom
  dma_display->drawPixel(x + 12, y + 6, color);
  dma_display->drawPixel(x + 14, y + 4, color); // Hook mark

  // Draw acute accent (/)
  dma_display->drawPixel(x + 12, y + 1, color);
  dma_display->drawPixel(x + 13, y, color);
}

// Draw custom Vietnamese character "ậ" (a + circumflex + dot below)
// Based on 5x8 pixel pattern
void drawACircumflexDotBelow(int16_t x, int16_t y, uint16_t color) {
  // Row 0: Circumflex (^) - top
  dma_display->drawPixel(x + 2, y, color);

  // Row 1: Circumflex - sides
  dma_display->drawPixel(x + 1, y + 1, color);
  dma_display->drawPixel(x + 3, y + 1, color);

  // Row 2: Empty (space between circumflex and letter a)

  // Row 3: Top of letter "a" - 3 horizontal pixels
  dma_display->drawPixel(x + 1, y + 3, color);
  dma_display->drawPixel(x + 2, y + 3, color);
  dma_display->drawPixel(x + 3, y + 3, color);
  dma_display->drawPixel(x + 4, y + 3, color);

  // Row 4: Left and right edges of "a"
  dma_display->drawPixel(x, y + 4, color);
  dma_display->drawPixel(x + 4, y + 4, color);
  dma_display->drawPixel(x, y + 5, color);
  dma_display->drawPixel(x + 4, y + 5, color);

  // Row 5: Bottom of "a" - horizontal pixels in middle + right edge
  dma_display->drawPixel(x + 1, y + 6, color);
  dma_display->drawPixel(x + 2, y + 6, color);
  dma_display->drawPixel(x + 3, y + 6, color);
  dma_display->drawPixel(x + 4, y + 6, color);
  dma_display->drawPixel(x + 5, y + 6, color);

  // Row 6: Empty (space before dot below)

  // Row 7: Empty (space before dot below)

  // Row 8: Dot below
  dma_display->drawPixel(x + 2, y + 8, color);
}

// Draw outdoor weather icon (tree - 5x9 pixels)
void drawOutdoorIcon(int16_t x, int16_t y, uint16_t color) {
  // Tree top (triangle shape)
  dma_display->drawPixel(x + 3, y - 1, color);      // Peak
  dma_display->drawPixel(x + 2, y, color);  // Second row
  dma_display->drawPixel(x + 4, y, color);

  dma_display->drawPixel(x + 1, y + 1, color);  // third row
  dma_display->drawPixel(x + 5, y + 1, color);

  dma_display->drawLine(x, y + 2, x + 6, y + 2, color); // Fourth row

  dma_display->drawPixel(x + 2, y + 3, color);  // Fifth row
  dma_display->drawPixel(x + 4, y + 3, color);

  dma_display->drawPixel(x + 1, y + 4, color);  // Six`th row
  dma_display->drawPixel(x + 5, y + 4, color);

  dma_display->drawLine(x, y + 5, x + 6, y + 5, color); // Seventh row

  dma_display->drawPixel(x + 3, y + 6, color);
}

// Draw indoor icon (home - 5x6 pixels)
void drawIndoorIcon(int16_t x, int16_t y, uint16_t color) {
  // Roof (triangle)
  dma_display->drawPixel(x + 3, y, color);      // Top peak
  dma_display->drawPixel(x + 2, y + 1, color);  // Left slope
  dma_display->drawPixel(x + 4, y + 1, color);  // Right slope
  dma_display->drawPixel(x + 1, y + 2, color);      // Left base
  dma_display->drawPixel(x + 5, y + 2, color);  // Right base

  dma_display->drawPixel(x, y + 3, color);  // 
  dma_display->drawPixel(x + 6, y + 3, color);  // 
  
  // House body (square)
  // dma_display->drawLine(x, y + 3, x + 6, y + 3, color);      // line Top
  dma_display->drawLine(x + 1, y + 3, x + 1, y + 6, color);      // Left wall
  dma_display->drawLine(x + 5, y + 3, x + 5, y + 6, color); // Right wall
  dma_display->drawLine(x + 1, y + 6, x + 5, y + 6, color);  // Bottom
  
  // Door
  dma_display->drawPixel(x + 3, y + 5, color);
  dma_display->drawPixel(x + 3, y + 6, color);
}

// Set display brightness (0-100%)
void setDisplayBrightness(uint8_t brightness) {
  // Constrain brightness to valid range
  if (brightness < 0) brightness = 0;
  if (brightness > 100) brightness = 100;
  
  // Convert percentage (0-100) to HUB75 scale (0-255)
  uint8_t value = (brightness * 255) / 100;
  dma_display->setBrightness8(value);
}
