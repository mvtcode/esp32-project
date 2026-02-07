#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <btAudio.h>

// Cấu hình màn hình OLED 1.3" (128x64)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// I2C pins cho OLED
#define I2C_SDA 21
#define I2C_SCL 22

// I2S pins cho PCM5102A
#define I2S_BCK  26  // Bit Clock
#define I2S_WS   27  // Word Select (LRC)
#define I2S_DOUT 25  // Data Out

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
btAudio audio = btAudio("ESP32-BT-Speaker");

// Biến lưu metadata
String lastTitle = "";
String lastArtist = "";
String lastAlbum = "";
unsigned long lastMetadataUpdate = 0;
int scrollOffset = 0;

void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  
  // Lấy metadata từ btAudio
  String title = audio.title;
  String artist = audio.artist;
  String album = audio.album;
  
  // Kiểm tra có metadata mới không
  if (title != lastTitle || artist != lastArtist) {
    lastTitle = title;
    lastArtist = artist;
    lastAlbum = album;
    lastMetadataUpdate = millis();
    scrollOffset = 0;
  }
  
  // Hiển thị tiêu đề
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Now Playing:");
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SH110X_WHITE);
  
  // Hiển thị tên bài hát (có scroll nếu dài)
  display.setCursor(0, 14);
  if (title.length() > 0) {
    if (title.length() > 21) {
      // Scroll text
      int maxScroll = (title.length() - 21) * 6;
      String displayTitle = title.substring(scrollOffset / 6);
      display.println(displayTitle.substring(0, 21));
      
      // Tự động scroll
      if (millis() - lastMetadataUpdate > 2000) {
        scrollOffset++;
        if (scrollOffset > maxScroll) {
          scrollOffset = 0;
          lastMetadataUpdate = millis();
        }
      }
    } else {
      display.println(title);
    }
  } else {
    display.println("(Unknown)");
  }
  
  // Hiển thị nghệ sĩ
  display.setCursor(0, 26);
  display.print("By: ");
  if (artist.length() > 0) {
    String shortArtist = artist;
    if (shortArtist.length() > 17) {
      shortArtist = shortArtist.substring(0, 14) + "...";
    }
    display.println(shortArtist);
  } else {
    display.println("Unknown Artist");
  }
  
  // Hiển thị album (nếu có)
  if (album.length() > 0) {
    display.setCursor(0, 38);
    display.setTextSize(1);
    String shortAlbum = album;
    if (shortAlbum.length() > 21) {
      shortAlbum = shortAlbum.substring(0, 18) + "...";
    }
    display.println(shortAlbum);
  }
  
  // Thanh trạng thái ở dưới
  display.drawLine(0, 52, SCREEN_WIDTH, 52, SH110X_WHITE);
  display.setCursor(0, 56);
  display.setTextSize(1);
  
  // Animation đang phát
  int dots = (millis() / 300) % 4;
  display.print("Playing");
  for(int i = 0; i < dots; i++) {
    display.print(".");
  }
  
  // Hiển thị volume
  display.setCursor(80, 56);
  display.print("Vol:MAX");
  
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=================================");
  Serial.println("ESP32 Bluetooth Speaker");
  Serial.println("with Metadata Display");
  Serial.println("=================================\n");
  
  // Khởi tạo I2C cho OLED
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.println("✅ I2C initialized");
  
  // Khởi tạo OLED
  if(!display.begin(SCREEN_ADDRESS, true)) {
    Serial.println("❌ OLED init failed!");
  } else {
    Serial.println("✅ OLED initialized");
    
    // Splash screen
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(10, 10);
    display.println("ESP32");
    display.setCursor(10, 30);
    display.println("Speaker");
    display.setTextSize(1);
    display.setCursor(0, 56);
    display.println("Starting Bluetooth...");
    display.display();
    delay(2000);
  }
  
  // Bắt đầu Bluetooth Audio
  Serial.println("\nStarting Bluetooth Audio...");
  audio.begin();
  Serial.println("✅ Bluetooth started");
  
  // Thử kết nối lại nếu đã pair trước đó
  Serial.println("\nTrying to reconnect...");
  audio.reconnect();
  
  // Cấu hình I2S cho PCM5102A - PHẢI SAU begin() và reconnect()
  Serial.println("\nConfiguring I2S...");
  Serial.printf("  BCK  = GPIO %d\n", I2S_BCK);
  Serial.printf("  DOUT = GPIO %d\n", I2S_DOUT);
  Serial.printf("  WS   = GPIO %d\n", I2S_WS);
  
  audio.I2S(I2S_BCK, I2S_DOUT, I2S_WS);
  Serial.println("✅ I2S configured");
  
  // Đặt âm lượng tối đa
  audio.volume(1.0);
  Serial.println("✅ Volume set to maximum");
  
  Serial.println("\n✅ Setup complete!");
  Serial.println("\n🎵 Ready to pair!");
  Serial.println("Device name: ESP32-BT-Speaker");
  Serial.println("Connect from your phone's Bluetooth settings\n");
  
  // Hiển thị waiting screen
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("Waiting for");
  display.println("Bluetooth");
  display.println("connection...");
  display.println();
  display.println("Device:");
  display.setTextSize(1);
  display.println("ESP32-BT-Speaker");
  display.display();
}

void loop() {
  // Cập nhật metadata từ btAudio - CHỈ MỖI 2 GIÂY để tránh làm vấp audio
  static unsigned long lastMetaUpdate = 0;
  if (millis() - lastMetaUpdate > 2000) {
    lastMetaUpdate = millis();
    audio.updateMeta();
  }
  
  // Cập nhật OLED mỗi 500ms (thay vì 100ms) để giảm tải CPU
  static unsigned long lastDisplayUpdate = 0;
  if (millis() - lastDisplayUpdate > 500) {
    lastDisplayUpdate = millis();
    updateDisplay();
  }
  
  // Debug metadata mỗi 10 giây
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 10000) {
    lastDebug = millis();
    Serial.println("🎵 Metadata:");
    Serial.printf("  Title:  %s\n", audio.title.c_str());
    Serial.printf("  Artist: %s\n", audio.artist.c_str());
    Serial.printf("  Album:  %s\n", audio.album.c_str());
  }
  
  delay(100);
}