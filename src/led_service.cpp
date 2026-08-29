#include "led_service.h"

LedState LedService::current_state = LED_STATE_OFF;
uint32_t LedService::state_start_time = 0;
uint32_t LedService::last_blink_time = 0;
bool LedService::blink_toggle = false;
uint8_t LedService::brightness_scale = LED_BRIGHTNESS_PERCENT; // 30%
bool LedService::is_enabled = true; // Mặc định bật

// =========================================================================
// DRIVER WS2812 DIRECT GPIO CYCLE-ACCURATE BITBANG (ESP32-S3 @ 240MHz)
// =========================================================================
void IRAM_ATTR LedService::sendPixel(uint8_t r, uint8_t g, uint8_t b) {
    if (!is_enabled) {
        r = 0; g = 0; b = 0;
    }

    // 1. Áp dụng tỷ lệ cường độ sáng (30%)
    uint8_t scaled_r = (uint16_t)r * brightness_scale / 100;
    uint8_t scaled_g = (uint16_t)g * brightness_scale / 100;
    uint8_t scaled_b = (uint16_t)b * brightness_scale / 100;

    // Chuẩn WS2812 truyền thứ tự: Green -> Red -> Blue (24-bit GRB)
    uint32_t grb = ((uint32_t)scaled_g << 16) | ((uint32_t)scaled_r << 8) | (uint32_t)scaled_b;

    // Direct Register Access cho GPIO >= 32 trên ESP32-S3
    volatile uint32_t *gpio_set = (BOARD_LED_PIN >= 32) ? &GPIO.out1_w1ts.val : &GPIO.out_w1ts;
    volatile uint32_t *gpio_clr = (BOARD_LED_PIN >= 32) ? &GPIO.out1_w1tc.val : &GPIO.out_w1tc;
    uint32_t mask = (BOARD_LED_PIN >= 32) ? (1UL << (BOARD_LED_PIN - 32)) : (1UL << BOARD_LED_PIN);

    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&mux);

    for (int i = 23; i >= 0; --i) {
        if (grb & (1UL << i)) {
            // Bit 1: HIGH ~700ns (168 cycles @ 240MHz), LOW ~600ns (144 cycles)
            *gpio_set = mask;
            uint32_t start = ESP.getCycleCount();
            while ((ESP.getCycleCount() - start) < 168);
            *gpio_clr = mask;
            start = ESP.getCycleCount();
            while ((ESP.getCycleCount() - start) < 144);
        } else {
            // Bit 0: HIGH ~350ns (84 cycles @ 240MHz), LOW ~800ns (192 cycles)
            *gpio_set = mask;
            uint32_t start = ESP.getCycleCount();
            while ((ESP.getCycleCount() - start) < 84);
            *gpio_clr = mask;
            start = ESP.getCycleCount();
            while ((ESP.getCycleCount() - start) < 192);
        }
    }

    portEXIT_CRITICAL(&mux);
}

void LedService::setEnabled(bool enabled) {
    is_enabled = enabled;
    if (!is_enabled) {
        setRawColor(0, 0, 0);
    }
}

void LedService::setRawColor(uint8_t r, uint8_t g, uint8_t b) {
    sendPixel(r, g, b);
}

void LedService::init() {
    pinMode(BOARD_LED_PIN, OUTPUT);
    digitalWrite(BOARD_LED_PIN, LOW);
    setRawColor(0, 0, 0);

    if (is_enabled) {
        // Chớp test khởi động: Xanh dương -> Xanh lá -> Tắt
        setRawColor(0, 100, 255);
        delay(100);
        setRawColor(0, 255, 0);
        delay(150);
        setRawColor(0, 0, 0);
    }

    current_state = LED_STATE_IDLE;
    state_start_time = millis();
}

void LedService::setState(LedState state) {
    current_state = state;
    state_start_time = millis();
}

void LedService::triggerFaceDetected() {
    if (!is_enabled) return;
    if (current_state == LED_STATE_IDLE || current_state == LED_STATE_OFF) {
        current_state = LED_STATE_FACE_DETECTED;
        state_start_time = millis();
    }
}

void LedService::triggerUploadStart() {
    if (!is_enabled) return;
    current_state = LED_STATE_UPLOADING;
    state_start_time = millis();
}

void LedService::triggerUploadSuccess() {
    if (!is_enabled) return;
    current_state = LED_STATE_UPLOAD_SUCCESS;
    state_start_time = millis();
}

void LedService::triggerUploadFailed() {
    if (!is_enabled) return;
    current_state = LED_STATE_UPLOAD_FAILED;
    state_start_time = millis();
}

void LedService::update(bool is_portal, bool is_wifi_connected, UploadStatus upload_status) {
    if (!is_enabled) {
        setRawColor(0, 0, 0);
        return;
    }

    uint32_t now = millis();

    // 1. Máy tạo xung nháy chu kỳ 400ms
    if (now - last_blink_time >= 400) {
        last_blink_time = now;
        blink_toggle = !blink_toggle;
    }

    // 2. ƯU TIÊN 1: Đang trong quá trình Upload ảnh -> Màu XANH LÁ CÂY (Solid Green)
    if (upload_status == UPLOAD_IN_PROGRESS || current_state == LED_STATE_UPLOADING) {
        setRawColor(0, 255, 0);
        return;
    }

    // 3. ƯU TIÊN 2: Upload thất bại -> Màu VÀNG (Yellow) nháy cảnh báo trong 2 giây
    if (upload_status == UPLOAD_FAILED || current_state == LED_STATE_UPLOAD_FAILED) {
        if (now - state_start_time < 2000) {
            bool fast_blink = ((now / 150) % 2) == 0;
            if (fast_blink) {
                setRawColor(255, 180, 0); // Màu Vàng rực
            } else {
                setRawColor(0, 0, 0);
            }
            return;
        } else {
            current_state = LED_STATE_IDLE;
        }
    }

    // 4. ƯU TIÊN 3: Upload thành công -> Màu CYAN chớp 2 nhịp nhanh trong 600ms
    if (upload_status == UPLOAD_SUCCESS || current_state == LED_STATE_UPLOAD_SUCCESS) {
        if (now - state_start_time < 600) {
            bool pulse = ((now / 150) % 2) == 0;
            if (pulse) {
                setRawColor(0, 255, 200); // Xanh ngọc Cyan
            } else {
                setRawColor(0, 0, 0);
            }
            return;
        } else {
            current_state = LED_STATE_IDLE;
        }
    }

    // 5. ƯU TIÊN 4: Đang ở chế độ Web Captive Portal -> Màu XANH DƯƠNG nháy chậm 600ms
    if (is_portal) {
        bool slow_blink = ((now / 600) % 2) == 0;
        if (slow_blink) {
            setRawColor(0, 80, 255); // Xanh dương
        } else {
            setRawColor(0, 0, 0);
        }
        return;
    }

    // 6. ƯU TIÊN 5: Mất kết nối WiFi hoặc đang kết nối -> Màu ĐỎ nháy chu kỳ 400ms
    if (!is_wifi_connected) {
        if (blink_toggle) {
            setRawColor(255, 0, 0); // Màu Đỏ
        } else {
            setRawColor(0, 0, 0);
        }
        return;
    }

    // 7. ƯU TIÊN 6: Chớp nhẹ màu TÍM khi phát hiện khuôn mặt (100ms)
    if (current_state == LED_STATE_FACE_DETECTED) {
        if (now - state_start_time < 100) {
            setRawColor(200, 0, 255); // Màu Tím
            return;
        } else {
            current_state = LED_STATE_IDLE;
        }
    }

    // 8. TRẠNG THÁI BÌNH THƯỜNG (IDLE, WiFi OK): Tắt LED hoàn toàn
    setRawColor(0, 0, 0);
}
