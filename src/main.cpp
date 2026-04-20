#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

/**
 * ESP32-S3 Super Mini - OLED 1.3" SH1106 Test (FIXED VERSION)
 * 
 * Hướng dẫn fix lỗi nhiễu/garbled screen:
 * 1. Sử dụng Software I2C để ổn định tín hiệu trên breadboard.
 * 2. Thêm delay khởi động.
 * 3. Thử các constructor khác nếu cấu hình mặc định không khớp.
 */

// Định nghĩa chân I2C cho ESP32-S3 Super Mini
#define I2C_SDA 8
#define I2C_SCL 9

/**
 * CONSTRUCTION CHOIСE:
 * Nếu cách 1 (mặc định) vẫn lỗi, hãy comment nó lại và thử cách 2 hoặc 3.
 */

// CÁCH 1: SH1106 Software I2C (Phổ biến cho màn 1.3") - Đã chọn tối ưu cho S3
U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ I2C_SCL, /* data=*/ I2C_SDA, /* reset=*/ U8X8_PIN_NONE);

// CÁCH 2: SH1106 VHR (Thử nếu cách 1 hiện thị sai vị trí)
// U8G2_SH1106_128X64_VHR_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ I2C_SCL, /* data=*/ I2C_SDA, /* reset=*/ U8X8_PIN_NONE);

// CÁCH 3: SSD1306 (Thử nếu màn hình thực tế dùng chip SSD1306 dù là 1.3")
// U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ I2C_SCL, /* data=*/ I2C_SDA, /* reset=*/ U8X8_PIN_NONE);


void setup() {
  Serial.begin(115200);
  
  // QUAN TRỌNG: Độ trễ để màn hình ổn định sau khi cấp nguồn
  delay(1000); 
  Serial.println("ESP32-S3 Super Mini - OLED Fix Starting...");

  // Khởi tạo màn hình
  if (u8g2.begin()) {
    Serial.println("OLED Initialized Successfully!");
  } else {
    Serial.println("OLED Initialization Failed!");
  }

  // Hiển thị thông báo trạng thái
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr); 
  u8g2.drawStr(5, 20, "OLED FIXED!");
  u8g2.drawStr(5, 40, "ESP32-S3 Mini");
  u8g2.drawFrame(0, 0, 128, 64);
  u8g2.sendBuffer();
  
  delay(2000);
}

void loop() {
  static int offset = 0;
  
  u8g2.clearBuffer();
  
  // Vẽ khung viền
  u8g2.drawFrame(0, 0, 128, 64);
  
  // Hiển thị Text
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(10, 20);
  u8g2.print("Status: Working");
  
  u8g2.setCursor(10, 35);
  u8g2.print("Uptime: ");
  u8g2.print(millis() / 1000);
  u8g2.print("s");
  
  // Vẽ animation hình vuông chạy quanh khung
  if (offset < 118) {
    u8g2.drawBox(5 + offset, 50, 10, 8);
  } else {
    offset = 0;
  }
  
  // Vẽ tia chớp nhỏ để test độ sắc nét
  u8g2.drawLine(100, 10, 110, 25);
  u8g2.drawLine(110, 25, 100, 25);
  u8g2.drawLine(100, 25, 110, 40);

  u8g2.sendBuffer();
  
  offset += 2;
  delay(30);
}
