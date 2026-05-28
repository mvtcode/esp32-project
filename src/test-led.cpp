// test-led.cpp  --  6-panel HUB75 matrix diagnostic (128x96 virtual canvas)
// Panel wiring / signal-chain (front view):
//
//   ESP32 -> [Panel 1, rot 0 ] -> [Panel 2, rot 0 ]
//                                        |
//                                        v
//            [Panel 4, rot 180] <- [Panel 3, rot 180]
//                |
//                v
//            [Panel 5, rot 0 ] -> [Panel 6, rot 0 ]
//
// DMA library lays the chain out as a single 384x32 strip:
//   chain index:   0       1       2       3       4       5
//   phys_x range: 0-63  64-127 128-191 192-255 256-319 320-383
//   phys_y range: 0-31  (same for all)
//
// Virtual (128x96) -> physical mapping:
//   Row 0 (y  0-31) : Panel 1 (idx 0) left,  Panel 2 (idx 1) right  -- normal
//     phys_x = x,          phys_y = y
//   Row 1 (y 32-63) : Panel 3 (idx 2) right, Panel 4 (idx 3) left   -- rotated 180
//     phys_x = 255 - x,    phys_y = 63 - y
//   Row 2 (y 64-95) : Panel 5 (idx 4) left,  Panel 6 (idx 5) right  -- normal
//     phys_x = 256 + x,    phys_y = y - 64

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "vietnamese_helper.h"
#include "Verdana_Vietnamese10pt.h"
#include "Verdana_Vietnamese12pt.h"
#include "Verdana_Bold14pt.h"
#include "Verdana_Bold18pt.h"

// Offset to compensate for physical alignment vertical offset of the left panels.
// Positive values shift the left half down. Adjust this value (e.g., 0, 1, 2) to align.
const int LEFT_PANEL_Y_OFFSET = 2;

#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 6

MatrixPanel_I2S_DMA *dma_display = nullptr;

// ============================================================================
//  CustomMatrixPanel  --  virtual 128x96 coordinate space
// ============================================================================
class CustomMatrixPanel : public Adafruit_GFX {
private:
  MatrixPanel_I2S_DMA *dma;

public:
  CustomMatrixPanel(MatrixPanel_I2S_DMA *d, int16_t w, int16_t h)
      : Adafruit_GFX(w, h), dma(d) {}

  uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return dma->color565(r, g, b);
  }

  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (x < 0 || x >= 128 || y < 0 || y >= 96) return;

    int16_t px, py;

    if (y < 32) {
      // Row 0: Panel 6 (slot 5, left), Panel 5 (slot 4, right) - both rotated 180
      px = 383 - x;
      py = 31 - y;
    } else if (y < 64) {
      // Row 1: Panel 3 (slot 2, left), Panel 4 (slot 3, right) - both normal (0 deg)
      px = 128 + x;
      py = y - 32;
    } else {
      // Row 2: Panel 2 (slot 1, left), Panel 1 (slot 0, right) - both rotated 180
      px = 127 - x;
      py = 95 - y;
    }

    dma->drawPixel(px, py, color);
  }

  void fillScreen(uint16_t color) override {
    dma->fillScreen(color);
  }
};

CustomMatrixPanel *virtual_display = nullptr;

// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== ESP32-S3 LED Matrix Test (128x96, 6 panels) ===");

  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);

  mxconfig.gpio.r1  = 4;
  mxconfig.gpio.g1  = 5;
  mxconfig.gpio.b1  = 6;
  mxconfig.gpio.r2  = 7;
  mxconfig.gpio.g2  = 15;
  mxconfig.gpio.b2  = 16;
  mxconfig.gpio.a   = 17;
  mxconfig.gpio.b   = 18;
  mxconfig.gpio.c   = 8;
  mxconfig.gpio.d   = 42;
  mxconfig.gpio.clk = 41;
  mxconfig.gpio.lat = 40;
  mxconfig.gpio.oe  = 2;

  // Fix 1-pixel shift bleeding and timing issues
  mxconfig.clkphase = false;
  mxconfig.latch_blanking = 4;
  mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_10M; // Revert to 8MHz to prevent signal noise/regional flickering
  mxconfig.double_buff = true;               // Keep double buffering enabled to prevent drawing flicker

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  if (!dma_display->begin()) {
    Serial.println("FAILED to initialise DMA display!");
  } else {
    Serial.println("DMA display OK.");
  }

  dma_display->setBrightness8(64); // ~25% -- safe for initial testing
  dma_display->fillScreen(0);
  dma_display->flipDMABuffer(); // Swap buffer to show blank screen

  virtual_display = new CustomMatrixPanel(dma_display, 128, 96);
}

// ============================================================================
void loop() {
  uint16_t RED    = dma_display->color565(255,   0,   0);
  uint16_t GREEN  = dma_display->color565(  0, 255,   0);
  uint16_t BLUE   = dma_display->color565(  0,   0, 255);
  uint16_t WHITE  = dma_display->color565(255, 255, 255);
  uint16_t YELLOW = dma_display->color565(255, 255,   0);
  uint16_t CYAN   = dma_display->color565(  0, 255, 255);
  uint16_t MAGENTA = dma_display->color565(255,   0, 255);

  // Vietnamese text messages
  static String header = utf8ToCustom("Bảng Điều Khiển");
  static String marqueeText1 = utf8ToCustom("Xin chào! Đây là chương trình chạy chữ marquee Tiếng Việt có dấu: á, à, ả, ã, ạ, â, ê, ô, ư, đ... hiển thị cực kỳ mượt mà và không bị lỗi font!");
  static String footerText = utf8ToCustom("Chúc Mừng Năm Mới - An Khang Thịnh Vượng - Vạn Sự Như Ý! ");

  static int16_t scrollX0 = 128;
  static int16_t scrollX1 = 128;
  static int16_t scrollX2 = 128;

  static uint16_t headerWidth = 0;
  static uint16_t marqueeWidth = 0;
  static uint16_t footerWidth = 0;

  static bool initialized = false;
  if (!initialized) {
    int16_t x1, y1;
    uint16_t h;

    virtual_display->setFont(&Verdana_Bold14pt);
    virtual_display->getTextBounds(header, 0, 0, &x1, &y1, &headerWidth, &h);

    virtual_display->setFont(&Verdana_Vietnamese12pt);
    virtual_display->getTextBounds(marqueeText1, 0, 0, &x1, &y1, &marqueeWidth, &h);

    virtual_display->setFont(&Verdana_Bold14pt);
    virtual_display->getTextBounds(footerText, 0, 0, &x1, &y1, &footerWidth, &h);

    initialized = true;
  }

  virtual_display->fillScreen(0);
  virtual_display->setTextWrap(false);

  // 1. Draw Marquee Header in Row 0 (scrolling left)
  virtual_display->setFont(&Verdana_Bold14pt);
  virtual_display->setTextColor(CYAN);
  virtual_display->setCursor(scrollX0, 22);
  virtual_display->print(header);

  // Divider line
  virtual_display->drawFastHLine(0, 31, 128, BLUE);

  // 2. Draw Marquee 1 in Row 1 (scrolling left)
  virtual_display->setFont(&Verdana_Vietnamese12pt);
  virtual_display->setTextColor(YELLOW);
  virtual_display->setCursor(scrollX1, 52);
  virtual_display->print(marqueeText1);

  // Divider line
  virtual_display->drawFastHLine(0, 63, 128, BLUE);

  // 3. Draw Marquee 2 in Row 2 (scrolling left)
  virtual_display->setFont(&Verdana_Bold14pt);
  virtual_display->setTextColor(GREEN);
  virtual_display->setCursor(scrollX2, 84);
  virtual_display->print(footerText);

  // Update positions
  scrollX0 -= 1; // Speed 1 for header
  if (scrollX0 < -headerWidth) {
    scrollX0 = 128;
  }

  scrollX1 -= 2; // Speed 2 for main marquee
  if (scrollX1 < -marqueeWidth) {
    scrollX1 = 128;
  }

  scrollX2 -= 1; // Speed 1 for footer
  if (scrollX2 < -footerWidth) {
    scrollX2 = 128;
  }

  dma_display->flipDMABuffer(); // Swap buffer to show the finished frame
  delay(25); // ~40 FPS animation speed
}
