#include <Arduino.h>

// LED built-in trên ESP32 thường ở GPIO 2
#define LED_BUILTIN 2

void setup() {
  // Khởi tạo Serial
  Serial.begin(115200);
  delay(1000); // Đợi Serial ổn định

  Serial.println("\n\n=================================");
  Serial.println("✅ Kết nối thành công!");
  Serial.println("ESP32 đã sẵn sàng hoạt động");
  Serial.println("=================================\n");

  // Cấu hình LED built-in là OUTPUT
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("LED built-in đang nhấp nháy...");
  Serial.println("(Mỗi giây bật/tắt một lần)\n");
}

void loop() {
  // Bật LED
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("💡 LED: BẬT");
  delay(1000);

  // Tắt LED
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("⚫ LED: TẮT");
  delay(1000);
}