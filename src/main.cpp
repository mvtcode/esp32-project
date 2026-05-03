#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <DHT.h>
#include <SD.h>
#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

TFT_eSPI tft = TFT_eSPI();

// --- 1. Cấu hình chân ---
#define LDR_PIN 34
#define DHT_PIN 22
#define DHT_TYPE DHT11

// RGB LED (4, 16, 21 - Tuỳ thuộc bo mạch, CYD thường là active LOW)
#define LED_R 4
#define LED_G 16
#define LED_B 21

// Khai báo SPI riêng cho thẻ nhớ (để không xung đột với màn hình)
SPIClass spiSD(HSPI);

// --- Biến toàn cục ---
DHT dht(DHT_PIN, DHT_TYPE);
AudioGeneratorMP3 *mp3 = NULL;
AudioFileSourceSD *file = NULL;
AudioOutputI2S *out = NULL;

uint16_t last_x = 0, last_y = 0;
bool wasTouched = false;

unsigned long lastLdrTime = 0;
unsigned long lastDhtTime = 0;
unsigned long lastRgbTime = 0;
uint8_t rgb_state = 0; 

// --- 2. Các hàm Giao diện ---
void touch_calibrate() {
  uint16_t calData[5] = { 3620, 275, 264, 3532, 1 }; 
  tft.setTouch(calData);
}

void setup_UI() {
  tft.fillScreen(TFT_BLACK);
  // Vẽ chia khu vực
  tft.drawLine(0, 40, 480, 40, TFT_DARKGREY);
  tft.drawLine(240, 0, 240, 40, TFT_DARKGREY);
  tft.drawLine(0, 280, 480, 280, TFT_DARKGREY);
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(10, 10);
  tft.print("Temp: -- C");
  
  tft.setCursor(250, 10);
  tft.print("LDR: ---");
  
  tft.setCursor(10, 290);
  tft.print("Audio: Init...");
}

// --- 3. Các hàm Chức năng (Module) ---

// 3.1. Test Cảm biến ánh sáng (LDR)
void test_LDR() {
  if (millis() - lastLdrTime > 500) { // Cập nhật mỗi 500ms
    lastLdrTime = millis();
    int ldrValue = analogRead(LDR_PIN);
    tft.setCursor(250, 10);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.printf("LDR: %4d", ldrValue);
  }
}

// 3.2. Test RGB LED (Nháy 7 màu)
void set_RGB(bool r, bool g, bool b) {
  digitalWrite(LED_R, r ? LOW : HIGH); // Đèn LED trên mạch thường kích ở mức LOW (Active Low)
  digitalWrite(LED_G, g ? LOW : HIGH);
  digitalWrite(LED_B, b ? LOW : HIGH);
}

void test_RGB_LED() {
  if (millis() - lastRgbTime > 1000) { // Chuyển màu mỗi 1 giây
    lastRgbTime = millis();
    switch(rgb_state) {
      case 0: set_RGB(1, 0, 0); break; // Red
      case 1: set_RGB(0, 1, 0); break; // Green
      case 2: set_RGB(0, 0, 1); break; // Blue
      case 3: set_RGB(1, 1, 0); break; // Yellow
      case 4: set_RGB(1, 0, 1); break; // Magenta
      case 5: set_RGB(0, 1, 1); break; // Cyan
      case 6: set_RGB(1, 1, 1); break; // White
    }
    rgb_state = (rgb_state + 1) % 7;
  }
}

// 3.3. Test DHT11
void test_DHT11() {
  if (millis() - lastDhtTime > 2000) { // DHT11 cần lấy mẫu sau mỗi 2 giây
    lastDhtTime = millis();
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    
    tft.setCursor(10, 10);
    if (isnan(t)) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.print("DHT: Error   ");
    } else {
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.printf("T:%.1fC H:%.0f%% ", t, h);
    }
  }
}

// 3.4. Test Thẻ nhớ (SD) và Âm thanh (MP3)
void init_SD_and_Audio() {
  // Khởi tạo SD Card
  spiSD.begin(18, 19, 23, 5); // SCK, MISO, MOSI, SS
  
  if (!SD.begin(5, spiSD)) {
    tft.setCursor(10, 290);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("Audio: SD Card Failed");
    return;
  }
  
  // Khởi tạo luồng đọc file
  audioLogger = &Serial;
  file = new AudioFileSourceSD("/test.mp3");
  if (!file->isOpen()) {
    tft.setCursor(10, 290);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("Audio: No /test.mp3  ");
    return;
  }
  
  // Khởi tạo bộ giải mã và đầu ra (Internal DAC = 1, port 0)
  out = new AudioOutputI2S(0, 1); 
  mp3 = new AudioGeneratorMP3();
  mp3->begin(file, out);
  
  tft.setCursor(10, 290);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.print("Audio: Playing...    ");
}

void loop_Audio() {
  if (mp3 && mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
      tft.setCursor(10, 290);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.print("Audio: Finished      ");
    }
  }
}

// 3.5. Khu vực test cảm ứng
void test_Touch() {
  uint16_t x, y;
  if (tft.getTouch(&x, &y)) {
    // Chỉ cho vẽ trong khu vực giữa màn hình (tránh đè text)
    if (y > 45 && y < 275) {
      if (wasTouched) {
        tft.drawLine(last_x, last_y, x, y, TFT_CYAN);
        tft.drawLine(last_x + 1, last_y, x + 1, y, TFT_CYAN);
        tft.drawLine(last_x - 1, last_y, x - 1, y, TFT_CYAN);
        tft.fillCircle(x, y, 1, TFT_CYAN); 
      } else {
        tft.fillCircle(x, y, 1, TFT_CYAN);
      }
      last_x = x;
      last_y = y;
      wasTouched = true;
    }
  } else {
    wasTouched = false;
  }
}

// --- 4. Main ---
void setup() {
  Serial.begin(115200);
  
  // Setup RGB LED
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  set_RGB(0, 0, 0); // Tắt đèn lúc đầu
  
  // Bật đèn nền
  pinMode(27, OUTPUT);
  digitalWrite(27, HIGH);

  // Khởi tạo màn hình
  tft.init();
  tft.setRotation(1); 
  touch_calibrate();
  setup_UI();
  
  // Khởi tạo DHT
  dht.begin();
  
  // Khởi tạo SD và Audio (Gọi sau cùng vì I2S cần bộ đệm dồi dào)
  init_SD_and_Audio();
}

void loop() {
  test_Touch();
  test_LDR();
  test_RGB_LED();
  test_DHT11();
  loop_Audio(); // Hàm này chạy liên tục để nạp buffer âm thanh
}