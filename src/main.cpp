#include <Arduino.h>
#include <Preferences.h>
#include <time.h>
#include "portal_service.h"
#include "camera_service.h"
#include "face_detector.h"
#include "google_drive_service.h"
#include "display_service.h"
#include "led_service.h"
#include "weather_service.h"

// Chân nút nhấn BOOT trên ESP32-S3
#define BOOT_BUTTON_PIN 0

// NVS Preferences và Cấu hình toàn cục
static Preferences prefs;
static AppConfig appConfig;

// Bộ đệm hình ảnh thu nhỏ 2:1 (320x240) từ ảnh gốc VGA (640x480)
static uint16_t *downscaled_buf = nullptr;

// Biến đo FPS hiển thị
static uint32_t frame_count = 0;
static uint32_t last_fps_time = 0;
static float display_fps = 0.0f;

// Standby & Mode Control
static uint32_t last_face_seen_time = 0;

// =========================================================================
// STATE MACHINE XỬ LÝ NÚT NHẤN BOOT
// - Nhấn nhả 1 lần : Bật / Tắt Google Drive Upload
// - Nhấn giữ 1s    : Chuyển đổi qua lại Camera AI <-> Đồng hồ, Lịch & Thời tiết (Clock & Calendar)
// - Nhấn giữ 3s    : Factory Reset App (Xóa NVS)
// =========================================================================
static uint32_t btn_down_time = 0;
static bool btn_is_held = false;
static bool long_press_1s_triggered = false;
static bool long_press_3s_triggered = false;

void handleBootButton() {
    int btn_raw = digitalRead(BOOT_BUTTON_PIN);
    uint32_t now = millis();

    // Nút được nhấn xuống (Active LOW)
    if (btn_raw == LOW) {
        if (!btn_is_held) {
            btn_is_held = true;
            btn_down_time = now;
            long_press_1s_triggered = false;
            long_press_3s_triggered = false;
        } else {
            uint32_t hold_time = now - btn_down_time;

            // Xử lý giữ 3 giây -> Factory Reset App
            if (hold_time >= 3000 && !long_press_3s_triggered) {
                long_press_3s_triggered = true;
                Serial.println("[Button BOOT] Hold 3s -> FACTORY RESET APP!");
                DisplayService::showToast("Factory Reset...", TFT_RED, TFT_WHITE, 3000);

                prefs.begin("app_cfg", false);
                prefs.clear();
                prefs.end();

                delay(1500);
                ESP.restart();
            }
            // Xử lý giữ 1 giây -> Chuyển đổi Camera AI <-> Standby Clock & Calendar Widget
            else if (hold_time >= 1000 && !long_press_1s_triggered && !long_press_3s_triggered) {
                long_press_1s_triggered = true;
                DisplayService::toggleMode();
                if (DisplayService::getMode() == DISPLAY_MODE_STANDBY_CLOCK) {
                    DisplayService::showToast("Mode: Clock & Calendar", TFT_NAVY, TFT_WHITE, 1500);
                } else {
                    DisplayService::showToast("Mode: Camera AI", TFT_DARKGREEN, TFT_WHITE, 1500);
                }
                Serial.printf("[Button BOOT] Hold 1s -> Switched to Mode: %d\n", (int)DisplayService::getMode());
            }
        }
    } else {
        // Nút được nhả ra
        if (btn_is_held) {
            btn_is_held = false;
            uint32_t press_duration = now - btn_down_time;

            // Nhấn nhả bình thường (không giữ lâu): TOGGLE BẬT/TẮT UPLOAD NGAY LẬP TỨC!
            if (!long_press_1s_triggered && !long_press_3s_triggered && press_duration >= 20) {
                bool new_state = !GoogleDriveService::isUploadEnabled();
                GoogleDriveService::setUploadEnabled(new_state);
                appConfig.upload_enabled = new_state;
                PortalService::saveConfig(appConfig);

                Serial.printf("[Button BOOT] Single Press -> Upload: %s (press %ums)\n",
                    new_state ? "ON" : "OFF", press_duration);

                if (new_state) {
                    DisplayService::showToast("Upload: ON", TFT_DARKGREEN, TFT_WHITE, 2000);
                } else {
                    DisplayService::showToast("Upload: OFF", TFT_MAROON, TFT_WHITE, 2000);
                }
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println("\n=== ESP32-S3 Smart Camera & Clock Calendar Pipeline ===");

    // Cấu hình nút nhấn BOOT (GPIO 0)
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

    // 1. Khởi tạo Khối Hiển thị màn hình ST7789
    DisplayService::init();

    // 2. Khởi tạo Khối Đèn LED Chỉ Báo Trạng Thái (GPIO 48)
    LedService::init();

    // 3. Tải cấu hình từ NVS
    appConfig = PortalService::loadConfig();
    LedService::setEnabled(appConfig.led_enabled);

    // 4. Kiểm tra chế độ khởi động (AP Portal hay WiFi STA)
    if (appConfig.wifi_ssid.isEmpty() || digitalRead(BOOT_BUTTON_PIN) == LOW) {
        Serial.println("[Setup] Starting Captive Portal AP Mode (ESP32S3-CAM-AP)...");
        PortalService::startCaptivePortal();
        
        // Hiển thị màn hình hướng dẫn cấu hình rõ ràng trên TFT
        DisplayService::showMessage(20, 30, "=== CAU HINH WIFI ===", TFT_CYAN);
        DisplayService::showMessage(20, 65, "WiFi: ESP32S3-CAM-AP", TFT_GREEN);
        DisplayService::showMessage(20, 100, "IP: 192.168.4.1", TFT_YELLOW);
        DisplayService::showMessage(20, 145, "Mo trinh duyet tren", TFT_WHITE);
        DisplayService::showMessage(20, 175, "dien thoai de cai dat!", TFT_WHITE);
        return;
    }

    // ================= CHẾ ĐỘ NORMAL (STA WIFI) =================
    Serial.println("[Setup] Starting Normal Mode with WiFi STA...");

    // Khởi tạo WiFi STA đồng bộ trên luồng chính trước để TCP/IP stack (LWIP) sẵn sàng
    WiFi.mode(WIFI_STA);

    // Khởi tạo Camera
    framesize_t cam_res = (appConfig.res_mode == 1) ? FRAMESIZE_QVGA : FRAMESIZE_VGA;
    if (!CameraService::init(cam_res, PIXFORMAT_RGB565)) {
        DisplayService::showMessage(10, 50, "Camera Init Failed!", TFT_RED);
        return;
    }

    // Cấp phát bộ đệm Downscale 320x240 từ PSRAM 8MB
    downscaled_buf = (uint16_t *)ps_malloc(320 * 240 * sizeof(uint16_t));
    if (!downscaled_buf) {
        downscaled_buf = (uint16_t *)malloc(320 * 240 * sizeof(uint16_t));
    }

    // Khởi tạo Face Detection (AI Detector)
    if (!FaceDetectorService::init()) {
        DisplayService::showMessage(10, 80, "AI Init Failed!", TFT_RED);
        return;
    }

    WeatherService::init(appConfig.weather_city.c_str());

    GoogleDriveService::setUploadEnabled(appConfig.upload_enabled);
    GoogleDriveService::init(
        appConfig.wifi_ssid.c_str(),
        appConfig.wifi_pass.c_str(),
        appConfig.google_script_url.c_str(),
        appConfig.upload_cooldown
    );

    PortalService::startWebServer();

    last_face_seen_time = millis();
}

void loop() {
    // 1. Xử lý Web Server và DNS requests
    PortalService::loop();

    // 2. Xử lý nút nhấn BOOT
    handleBootButton();

    // Nếu đang ở chế độ AP Captive Portal:
    if (PortalService::isPortalActive()) {
        LedService::update(true, false, UPLOAD_IDLE);
        delay(10);
        return;
    }

    // ================= CHẾ ĐỘ HOẠT ĐỘNG BÌNH THƯỜNG (STA) =================
    // 3. Thu nhận khung hình Camera
    CameraFrame rawFrame = CameraService::getFrame();
    if (!rawFrame.isValid()) {
        LedService::update(false, GoogleDriveService::isConnected(), GoogleDriveService::getStatus());
        delay(5);
        return;
    }

    CameraFrame displayFrame;
    if (appConfig.res_mode == 0 && rawFrame.width == 640) {
        if (downscaled_buf != nullptr) {
            const uint16_t *src = (const uint16_t *)rawFrame.buffer;
            for (int y = 0; y < 240; ++y) {
                const uint16_t *src_row = &src[y * 2 * 640];
                uint16_t *dst_row = &downscaled_buf[y * 320];
                for (int x = 0; x < 320; ++x) {
                    dst_row[x] = src_row[x * 2];
                }
            }
        }
        displayFrame.raw_fb = nullptr; // Fix #8: không sở hữu raw_fb, tránh double-free
        displayFrame.buffer = (uint8_t *)downscaled_buf;
        displayFrame.len    = 320 * 240 * sizeof(uint16_t);
        displayFrame.width  = 320;
        displayFrame.height = 240;
        displayFrame.format = PIXFORMAT_RGB565;
    } else {
        displayFrame = rawFrame;
    }

    // 4. Đưa khung hình displayFrame (320x240) vào AI Worker bằng memcpy siêu nhanh
    FaceDetectorService::feedFrame(displayFrame);
    FaceDetectionResult aiResult = FaceDetectorService::getLatestResult();

    uint32_t now = millis();

    // Cache isConnected() 1 lần/frame thay vì gọi 3 lần (Fix #9)
    bool wifi_ok = GoogleDriveService::isConnected();

    // Cập nhật dịch vụ Thời tiết
    WeatherService::update(wifi_ok);

    // Quản lý trạng thái tự động chuyển đổi Standby Clock & Calendar
    if (!aiResult.faces.empty()) {
        last_face_seen_time = now;
        if (DisplayService::getMode() == DISPLAY_MODE_STANDBY_CLOCK) {
            DisplayService::setMode(DISPLAY_MODE_CAMERA);
        }
        LedService::triggerFaceDetected();
    } else {
        if (appConfig.standby_timeout > 0 && DisplayService::getMode() == DISPLAY_MODE_CAMERA && (now - last_face_seen_time >= (uint32_t)appConfig.standby_timeout * 1000)) {
            DisplayService::setMode(DISPLAY_MODE_STANDBY_CLOCK);
        }
    }

    // Upload Google Drive nếu phát hiện khuôn mặt
    GoogleDriveService::processFaceTrigger(rawFrame, aiResult);

    // Render màn hình
    if (DisplayService::getMode() == DISPLAY_MODE_CAMERA) {
        DisplayService::render(
            displayFrame,
            aiResult,
            display_fps,
            GoogleDriveService::getStatus(),
            wifi_ok,
            GoogleDriveService::isUploadEnabled()
        );
    } else {
        DisplayService::renderStandbyClock(
            wifi_ok,
            WeatherService::getWeather(),
            aiResult.detect_fps
        );
    }

    LedService::update(
        false,
        wifi_ok,
        GoogleDriveService::getStatus()
    );

    // 5. Giải phóng frame cho camera
    CameraService::releaseFrame(rawFrame);

    // 6. Tính toán Display FPS
    frame_count++;
    if (now - last_fps_time >= 1000) {
        display_fps = (frame_count * 1000.0f) / (now - last_fps_time);
        frame_count = 0;
        last_fps_time = now;
    }
}
