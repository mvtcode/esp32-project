#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SDA_PIN 0
#define SCL_PIN 1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n=== ESP32 C3 + OLED 128x32 ===");
  Serial.println("Man hinh hoat dong tot!\n");
  
  Wire.begin(SDA_PIN, SCL_PIN);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Loi khoi tao!");
    for(;;);
  }
  
  // Chào mừng
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 8);
  display.println("ESP32 C3");
  display.display();
  delay(2000);
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Man hinh OLED");
  display.println("128x32 pixels");
  display.println("SSD1306 - I2C");
  display.println("Hoat dong tot!");
  display.display();
  delay(2000);
  
  Serial.println("Bat dau demo...\n");
}

void loop() {
  static int demoMode = 0;
  static unsigned long lastChange = 0;
  static int counter = 0;
  
  if(millis() - lastChange >= 3000) {
    lastChange = millis();
    demoMode = (demoMode + 1) % 6;
    counter = 0;
  }
  
  display.clearDisplay();
  
  switch(demoMode) {
    case 0: {
      // Demo 1: Đồng hồ đếm
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("DONG HO DEM");
      display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
      
      display.setTextSize(2);
      display.setCursor(20, 14);
      display.print(millis() / 1000);
      display.print(" s");
      break;
    }
      
    case 1: {
      // Demo 2: Nhiệt độ giả lập
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("NHIET DO");
      display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
      
      display.setTextSize(2);
      display.setCursor(25, 14);
      display.print(25 + (millis() / 1000) % 10);
      display.print(" C");
      break;
    }
      
    case 2: {
      // Demo 3: Thanh tiến trình
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("TIEN TRINH");
      
      int progress = (millis() / 30) % 101;
      display.drawRect(0, 12, SCREEN_WIDTH, 10, SSD1306_WHITE);
      display.fillRect(2, 14, map(progress, 0, 100, 0, SCREEN_WIDTH-4), 6, SSD1306_WHITE);
      
      display.setCursor(50, 24);
      display.print(progress);
      display.print("%");
      break;
    }
      
    case 3: {
      // Demo 4: Biểu đồ cột
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("BIEU DO");
      
      for(int i = 0; i < 8; i++) {
        int height = 5 + (millis() / 100 + i * 10) % 15;
        display.fillRect(i * 16, 32 - height - 10, 12, height, SSD1306_WHITE);
      }
      break;
    }
      
    case 4: {
      // Demo 5: Thông tin hệ thống
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("ESP32-C3 INFO");
      display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
      display.setCursor(0, 12);
      display.print("RAM: ");
      display.print(ESP.getFreeHeap() / 1024);
      display.println(" KB");
      display.print("Uptime: ");
      display.print(millis() / 1000);
      display.print(" s");
      break;
    }
      
    case 5: {
      // Demo 6: Animation
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("ANIMATION");
      
      int x = (millis() / 20) % (SCREEN_WIDTH + 20) - 10;
      display.fillCircle(x, 20, 5, SSD1306_WHITE);
      display.drawCircle(x, 20, 7, SSD1306_WHITE);
      break;
    }
  }
  
  display.display();
  delay(50);
}