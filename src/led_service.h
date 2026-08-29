#ifndef LED_SERVICE_H
#define LED_SERVICE_H

#include <Arduino.h>
#include "upload_types.h"  // UploadStatus enum (shared)

// =========================================================================
// CẤU HÌNH PHẦN CỨNG ĐÈN LED RGB WS2812 TRÊN BOARD (GPIO 48)
// =========================================================================
#ifndef BOARD_LED_PIN
#define BOARD_LED_PIN 48 // Chân LED WS2812 RGB trên ESP32-S3 DevKit / CAM
#endif

// Cường độ sáng 30% (mặc định)
#define LED_BRIGHTNESS_PERCENT 30

// Các chế độ hiển thị trạng thái LED
enum LedState {
    LED_STATE_OFF,               // Tắt LED (0, 0, 0)
    LED_STATE_PORTAL,            // Chế độ Captive Portal (Màu Xanh dương nháy chậm)
    LED_STATE_WIFI_DISCONNECTED, // Mất kết nối WiFi (Màu Đỏ nháy chu kỳ 400ms)
    LED_STATE_IDLE,              // Hệ thống sẵn sàng, WiFi OK (Tắt LED)
    LED_STATE_UPLOADING,         // Đang tải ảnh lên Google Drive (Màu Xanh lá sáng liên tục)
    LED_STATE_UPLOAD_SUCCESS,    // Tải ảnh thành công (Màu Xanh lá/Cyan chớp 2 nhịp rồi tắt)
    LED_STATE_UPLOAD_FAILED,     // Tải ảnh thất bại (Màu Vàng nháy cảnh báo trong 2s)
    LED_STATE_FACE_DETECTED      // Phát hiện khuôn mặt (Màu Tím chớp nhẹ 100ms)
};

class LedService {
private:
    static LedState current_state;
    static uint32_t state_start_time;
    static uint32_t last_blink_time;
    static bool blink_toggle;
    static uint8_t brightness_scale;
    static bool is_enabled;

    // Hàm xuất dữ liệu 24-bit chuẩn WS2812 bằng Direct GPIO Bitbang (an toàn tuyệt đối, không crash RMT/PSRAM)
    static void sendPixel(uint8_t r, uint8_t g, uint8_t b);

public:
    static void init();
    static void setEnabled(bool enabled);
    static bool isEnabled() { return is_enabled; }
    static void setRawColor(uint8_t r, uint8_t g, uint8_t b);
    static void setState(LedState state);
    static void triggerFaceDetected();
    static void triggerUploadStart();
    static void triggerUploadSuccess();
    static void triggerUploadFailed();
    
    // Cập nhật trạng thái tự động theo pipeline
    // Bug fix #1: Dùng UploadStatus thay vì int để tránh magic number
    static void update(bool is_portal, bool is_wifi_connected, UploadStatus upload_status);
};

#endif // LED_SERVICE_H
