#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// ===========================================
// CẤU HÌNH LED MATRIX P5
// ===========================================
#define PANEL_RES_X 64 // Số cột (pixels)
#define PANEL_RES_Y 32 // Số hàng (pixels)
#define PANEL_CHAIN 1  // Số panel nối tiếp (1 panel)

// Tạo đối tượng matrix
MatrixPanel_I2S_DMA *dma_display = nullptr;

// ===========================================
// FORWARD DECLARATIONS
// ===========================================
void demoColors();
void demoShapes();
void demoText();
uint8_t sin8(uint8_t x);

// ===========================================
// PIN MAPPING (HUB75)
// ===========================================
// Cấu hình mặc định của thư viện:
// R1: GPIO25, G1: GPIO26, B1: GPIO27
// R2: GPIO14, G2: GPIO12, B2: GPIO13
// A: GPIO23, B: GPIO19, C: GPIO5, D: GPIO17, E: GPIO18
// CLK: GPIO16, LAT: GPIO4, OE: GPIO15

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=================================");
  Serial.println("ESP32 LED Matrix P5 Controller");
  Serial.println("=================================\n");

  // Cấu hình HUB75
  HUB75_I2S_CFG mxconfig(PANEL_RES_X, // Chiều rộng module
                         PANEL_RES_Y, // Chiều cao module
                         PANEL_CHAIN  // Số lượng module nối tiếp
  );

  // Tùy chọn nâng cao (có thể bỏ comment nếu cần)
  // mxconfig.gpio.e = 18;  // Pin E (cho panel > 32 rows)
  // mxconfig.clkphase = false;
  // mxconfig.driver = HUB75_I2S_CFG::FM6126A; // Nếu dùng driver FM6126A

  // Khởi tạo matrix
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(90); // Độ sáng (0-255)
  dma_display->clearScreen();

  Serial.println("✅ LED Matrix đã sẵn sàng!");
  Serial.println("Độ phân giải: " + String(PANEL_RES_X) + "x" +
                 String(PANEL_RES_Y));
  Serial.println();

  // Demo các màu cơ bản
  demoColors();
  delay(2000);

  // Demo vẽ hình
  demoShapes();
  delay(2000);

  // Demo text
  demoText();
}

void loop() {
  // Hiệu ứng rainbow
  static uint8_t hue = 0;

  for (int x = 0; x < PANEL_RES_X; x++) {
    for (int y = 0; y < PANEL_RES_Y; y++) {
      // Tạo màu rainbow
      uint8_t pixelHue =
          hue + (x * 256 / PANEL_RES_X) + (y * 256 / PANEL_RES_Y);
      uint16_t color = dma_display->color565(
          sin8(pixelHue), sin8(pixelHue + 85), sin8(pixelHue + 170));
      dma_display->drawPixel(x, y, color);
    }
  }

  hue++;
  delay(20);
}

// ===========================================
// DEMO FUNCTIONS
// ===========================================

void demoColors() {
  Serial.println("📺 Demo màu sắc cơ bản...");

  // Đỏ
  dma_display->fillScreen(dma_display->color565(255, 0, 0));
  delay(500);

  // Xanh lá
  dma_display->fillScreen(dma_display->color565(0, 255, 0));
  delay(500);

  // Xanh dương
  dma_display->fillScreen(dma_display->color565(0, 0, 255));
  delay(500);

  // Trắng
  dma_display->fillScreen(dma_display->color565(255, 255, 255));
  delay(500);

  dma_display->clearScreen();
}

void demoShapes() {
  Serial.println("🎨 Demo vẽ hình...");

  dma_display->clearScreen();

  // Vẽ đường thẳng
  dma_display->drawLine(0, 0, PANEL_RES_X - 1, PANEL_RES_Y - 1,
                        dma_display->color565(255, 0, 0));
  dma_display->drawLine(PANEL_RES_X - 1, 0, 0, PANEL_RES_Y - 1,
                        dma_display->color565(0, 255, 0));

  // Vẽ hình chữ nhật
  dma_display->drawRect(10, 10, 20, 12, dma_display->color565(0, 0, 255));

  // Vẽ hình tròn
  dma_display->drawCircle(PANEL_RES_X / 2, PANEL_RES_Y / 2, 8,
                          dma_display->color565(255, 255, 0));
}

void demoText() {
  Serial.println("📝 Demo hiển thị text...");

  dma_display->clearScreen();
  dma_display->setTextSize(1);
  dma_display->setTextWrap(false);

  // Text màu đỏ
  dma_display->setCursor(2, 2);
  dma_display->setTextColor(dma_display->color565(255, 0, 0));
  dma_display->println("ESP32");

  // Text màu xanh
  dma_display->setCursor(2, 11);
  dma_display->setTextColor(dma_display->color565(0, 255, 0));
  dma_display->println("LED");

  // Text màu vàng
  dma_display->setCursor(2, 20);
  dma_display->setTextColor(dma_display->color565(255, 255, 0));
  dma_display->println("Matrix");
}

// ===========================================
// HELPER FUNCTIONS
// ===========================================

// Hàm sin8 để tạo hiệu ứng (0-255)
uint8_t sin8(uint8_t x) { return (sin(x * 2 * PI / 256.0) + 1.0) * 127.5; }