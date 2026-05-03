#include <Arduino.h>
#include <FastLED.h>

// Cấu hình phần cứng
#define LED_PIN     6      // Chân tín hiệu kết nối với LED
#define NUM_LEDS    24     // Số lượng LED (24 LED)
#define BRIGHTNESS  102    // Độ sáng (0-255) - 40% của 255
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB    // Thứ tự màu của chip WS2812

CRGB leds[NUM_LEDS];

uint8_t gHue = 0; // Biến lưu màu hiện tại

void setup() {
  // Trễ 2 giây để bảo vệ LED khi mới cấp nguồn
  delay(2000); 

  // Khởi tạo FastLED cho chip WS2812
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // Thiết lập độ sáng tối đa
  FastLED.setBrightness(BRIGHTNESS);
}

void loop() {
  // Tạo hiệu ứng cầu vồng mượt mà cho vòng tròn
  // Sử dụng công thức (i * 256 / NUM_LEDS) để chia đều dải màu 256 đơn vị cho NUM_LEDS bóng
  // Điều này đảm bảo điểm đầu và điểm cuối khớp nhau hoàn hảo
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(gHue + (i * 256 / NUM_LEDS), 255, 255);
  }

  // Hiển thị ra các LED
  FastLED.show();

  // Thay đổi màu sắc theo thời gian
  // Mỗi 20ms sẽ tăng gHue lên 1 đơn vị để hiệu ứng cầu vồng "chạy"
  EVERY_N_MILLISECONDS(20) { 
    gHue++; 
  }
}
