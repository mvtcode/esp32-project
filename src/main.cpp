#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <WiFi.h>
#include <time.h>
#include <FastLED.h>

// Thông tin cấu hình từ source cũ
const char* ssid = "HPSTAR";
const char* password = "0964335688";
#define PANEL_RES_X 64 
#define PANEL_RES_Y 32 
#define PANEL_CHAIN 1  

MatrixPanel_I2S_DMA *dma_display = nullptr;
uint8_t globalHue = 0;

uint16_t hsvToRgb565(uint8_t hue, uint8_t sat, uint8_t val) {
  CRGB rgb;
  CHSV hsv(hue, sat, val);
  hsv2rgb_rainbow(hsv, rgb);
  return dma_display->color565(rgb.r, rgb.g, rgb.b);
}

// Cải tiến: WiFi không gây treo máy
void connectWiFi() {
  Serial.println("Connecting WiFi...");
  WiFi.begin(ssid, password);
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) { // Giới hạn 10 giây
    delay(500);
    Serial.print(".");
    timeout++;
  }
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    configTime(7 * 3600, 0, "pool.ntp.org");
  } else {
    Serial.println("\nWiFi Failed! Running in offline mode.");
  }
}

void setup() {
  Serial.begin(115200);

  // 1. Cấu hình Pin theo sơ đồ của bạn
  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);
  mxconfig.gpio.r1 = 25; mxconfig.gpio.g1 = 26; mxconfig.gpio.b1 = 27;
  mxconfig.gpio.r2 = 14; mxconfig.gpio.g2 = 12; mxconfig.gpio.b2 = 13;
  mxconfig.gpio.a  = 23; mxconfig.gpio.b  = 19; mxconfig.gpio.c  = 5; 
  mxconfig.gpio.d  = 17; mxconfig.gpio.e  = 18; 
  mxconfig.gpio.clk = 16; mxconfig.gpio.lat = 4; mxconfig.gpio.oe = 15;

  // 2. Khởi tạo màn hình TRƯỚC khi kết nối WiFi
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  if(!dma_display->begin()) {
    Serial.println("Failed to initialize DMA Display!");
  }
  dma_display->setBrightness8(100);
  dma_display->clearScreen();

  // Test màn hình bằng một dòng chữ ngay lập tức
  dma_display->setCursor(10, 10);
  dma_display->setTextColor(dma_display->color565(255, 0, 0));
  dma_display->print("BOOTING...");

  // 3. Sau đó mới kết nối WiFi
  connectWiFi();
}

void loop() {
  struct tm timeinfo;
  bool hasTime = getLocalTime(&timeinfo);

  dma_display->clearScreen();
  
  if (hasTime) {
    // Phần vẽ đồng hồ (Giữ nguyên logic cũ của bạn)
    char hStr[3], mStr[3], sStr[3];
    sprintf(hStr, "%02d", timeinfo.tm_hour);
    sprintf(mStr, "%02d", timeinfo.tm_min);
    sprintf(sStr, "%02d", timeinfo.tm_sec);

    dma_display->setTextSize(2);
    // Vẽ Giờ
    dma_display->setCursor(1, 2);
    dma_display->setTextColor(hsvToRgb565(globalHue, 255, 255));
    dma_display->print(hStr);
    // Dấu :
    dma_display->setCursor(22, 2);
    dma_display->print(":");
    // Vẽ Phút
    dma_display->setCursor(30, 2);
    dma_display->setTextColor(hsvToRgb565(globalHue + 40, 255, 255));
    dma_display->print(mStr);
    // Vẽ Giây (với gradient cho từng chữ số)
    dma_display->setTextSize(1);
    // Chữ số giây thứ nhất
    dma_display->setCursor(52, 10);
    dma_display->setTextColor(hsvToRgb565(globalHue + 80, 255, 255));
    dma_display->print(sStr[0]);
    // Chữ số giây thứ hai
    dma_display->setCursor(58, 10);
    dma_display->setTextColor(hsvToRgb565(globalHue + 100, 255, 255));
    dma_display->print(sStr[1]);
    
    // Vẽ dòng 2: Thứ và Ngày/Tháng
    dma_display->setTextSize(1);
    
    // Hiển thị Thứ (wday: 0=CN, 1=Thứ 2, ..., 6=Thứ 7)
    const char* dayNames[] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
    dma_display->setCursor(2, 23);
    dma_display->setTextColor(hsvToRgb565(globalHue + 120, 255, 255));
    dma_display->print(dayNames[timeinfo.tm_wday]);
    
    // Dấu phẩy
    dma_display->print(",");
    
    // Hiển thị ngày/tháng DD/MM
    char dateStr[10];
    sprintf(dateStr, " %02d/%02d", timeinfo.tm_mday, timeinfo.tm_mon + 1);
    dma_display->setTextColor(hsvToRgb565(globalHue + 160, 255, 255));
    dma_display->print(dateStr);
  } else {
    // Nếu chưa có WiFi, hiện thông báo
    dma_display->setTextSize(1);
    dma_display->setCursor(5, 12);
    dma_display->setTextColor(dma_display->color565(0, 255, 0));
    dma_display->print("Waiting WiFi...");
  }

  globalHue += 2;
  delay(50);
}