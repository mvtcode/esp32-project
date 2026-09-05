#include <Arduino.h>
#include "log.h"
#include "pin_config.h"

static const char *TAG = "Main";

void setup() {
  // Khởi tạo Serial
  Serial.begin(115200);
  delay(1000); // Đợi Serial ổn định

  LOG_I(TAG, "=================================");
  LOG_I(TAG, "ESP32 đã sẵn sàng hoạt động");
  LOG_I(TAG, "=================================");

  // Cấu hình LED built-in là OUTPUT
  pinMode(PIN_LED_BUILTIN, OUTPUT);

  LOG_I(TAG, "LED built-in đang nhấp nháy (mỗi giây bật/tắt một lần)");
}

void loop() {
  // Bật LED
  digitalWrite(PIN_LED_BUILTIN, HIGH);
  LOG_D(TAG, "LED: BẬT");
  delay(1000);

  // Tắt LED
  digitalWrite(PIN_LED_BUILTIN, LOW);
  LOG_D(TAG, "LED: TẮT");
  delay(1000);
}