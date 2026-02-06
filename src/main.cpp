#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Cấu hình màn hình OLED 0.91" (128x32)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Khai báo các hàm test
void testText();
void testGraphics();
void testAnimation();

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\nESP32-S3 OLED Test");
  
  // Khởi tạo I2C (SDA=GPIO8, SCL=GPIO9)
  Wire.begin(8, 9);
  
  // Khởi tạo màn hình OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Dừng nếu không khởi tạo được
  }
  
  Serial.println("OLED initialized successfully!");
  
  // Hiển thị splash screen
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("ESP32-S3"));
  display.setTextSize(1);
  display.println(F("OLED Test OK!"));
  display.display();
  delay(2000);
}

void loop() {
  // Test 1: Text hiển thị
  testText();
  delay(2000);
  
  // Test 2: Đồ họa
  testGraphics();
  delay(2000);
  
  // Test 3: Animation
  testAnimation();
  delay(1000);
}

void testText() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Text Test:"));
  display.println(F("Size 1: Hello!"));
  display.setTextSize(2);
  display.println(F("Size 2"));
  display.display();
  Serial.println("Text test displayed");
}

void testGraphics() {
  display.clearDisplay();
  
  // Vẽ hình chữ nhật
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
  
  // Vẽ đường thẳng
  display.drawLine(0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, SSD1306_WHITE);
  display.drawLine(0, SCREEN_HEIGHT-1, SCREEN_WIDTH-1, 0, SSD1306_WHITE);
  
  // Vẽ hình tròn
  display.fillCircle(SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 8, SSD1306_WHITE);
  
  display.display();
  Serial.println("Graphics test displayed");
}

void testAnimation() {
  // Animation: Di chuyển hình vuông
  for(int x = 0; x < SCREEN_WIDTH - 16; x += 2) {
    display.clearDisplay();
    display.fillRect(x, 8, 16, 16, SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("Moving..."));
    display.display();
    delay(20);
  }
  Serial.println("Animation test completed");
}