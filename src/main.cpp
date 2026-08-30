#include <Arduino.h>
#include <Preferences.h>
#include <time.h>
#include "portal_service.h"
#include "camera_service.h"
#include "face_detector.h"
#include "google_drive_service.h"
#include "display_service.h"
#include "led_service.h"
#include "audio_service.h"
#include "weather_service.h"
#include "market_service.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

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
// ULTRA-RESPONSIVE BUTTON ENGINE (Core 0, 10ms sampling)
// Miễn nhiễm 100% với DTR/UART boot hold, phản hồi tức thì 0ms trễ
// - Bấm nhả (30ms - 900ms) : Bật / Tắt Google Drive Upload
// - Giữ 1.2 giây           : Chuyển đổi Camera AI <-> Đồng hồ & Lịch
// - Giữ 5 giây             : Factory Reset App (Xóa NVS)
// =========================================================================
static void buttonTaskWorker(void *param) {
    bool is_pressed = false;
    uint32_t press_start = 0;
    bool hold_1s_fired = false;
    bool hold_5s_fired = false;
    
    // Nếu lúc vừa khởi động nút bị kéo LOW (do cáp nạp UART DTR), khóa nút cho tới khi nhả ra
    bool stuck_at_boot = (digitalRead(BOOT_BUTTON_PIN) == LOW);

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10));
        uint32_t now = millis();
        int pin = digitalRead(BOOT_BUTTON_PIN);

        // 1. Kiểm tra xả khóa boot
        if (stuck_at_boot) {
            if (pin == HIGH) {
                stuck_at_boot = false;
                Serial.println("[Button] Button released -> Active & Ready!");
            }
            continue;
        }

        // 2. Nút được nhấn xuống (Active LOW)
        if (pin == LOW) {
            if (!is_pressed) {
                is_pressed = true;
                press_start = now;
                hold_1s_fired = false;
                hold_5s_fired = false;
            } else {
                uint32_t hold_time = now - press_start;

                // Giữ 5 giây -> Factory Reset
                if (hold_time >= 5000 && !hold_5s_fired) {
                    hold_5s_fired = true;
                    Serial.println("[Button] Hold 5s -> FACTORY RESET APP!");
                    AudioService::play(SOUND_UPLOAD_FAILED);
                    DisplayService::showToast("Factory Reset...", TFT_RED, TFT_WHITE, 3000);

                    prefs.begin("app_cfg", false);
                    prefs.clear();
                    prefs.end();

                    delay(1500);
                    ESP.restart();
                }
                // Giữ 1.2 giây -> Chuyển đổi màn hình Camera AI <-> Đồng hồ
                else if (hold_time >= 1200 && !hold_1s_fired && !hold_5s_fired) {
                    hold_1s_fired = true;
                    AudioService::play(SOUND_BUTTON_CLICK);
                    if (!PortalService::isPortalActive()) {
                        DisplayService::toggleMode();
                        Serial.printf("[Button] Hold 1.2s -> Switched Mode: %d\n", (int)DisplayService::getMode());
                        if (DisplayService::getMode() == DISPLAY_MODE_STANDBY_CLOCK) {
                            DisplayService::showToast("Mode: Clock & Calendar", TFT_NAVY, TFT_WHITE, 1500);
                        } else {
                            DisplayService::showToast("Mode: Camera AI", TFT_DARKGREEN, TFT_WHITE, 1500);
                        }
                    } else {
                        DisplayService::showToast("Portal Active", TFT_CYAN, TFT_BLACK, 1500);
                    }
                }
            }
        }
        // 3. Nút được nhả ra (Active HIGH)
        else {
            if (is_pressed) {
                uint32_t press_duration = now - press_start;
                is_pressed = false;

                // Nhấn nhả nhanh (30ms - 900ms): BẬT / TẮT UPLOAD NGAY LẬP TỨC
                if (!hold_1s_fired && !hold_5s_fired && press_duration >= 30 && press_duration < 1000) {
                    if (!PortalService::isPortalActive()) {
                        bool new_state = !GoogleDriveService::isUploadEnabled();
                        GoogleDriveService::setUploadEnabled(new_state);
                        appConfig.upload_enabled = new_state;
                        PortalService::saveConfig(appConfig);

                        Serial.printf("[Button] Click (%ums) -> Upload: %s\n", press_duration, new_state ? "ON" : "OFF");

                        if (new_state) {
                            AudioService::play(SOUND_UPLOAD_SUCCESS);
                            DisplayService::showToast("Upload: ON", TFT_DARKGREEN, TFT_WHITE, 2000);
                        } else {
                            AudioService::play(SOUND_UPLOAD_FAILED);
                            DisplayService::showToast("Upload: OFF", TFT_MAROON, TFT_WHITE, 2000);
                        }
                    } else {
                        // Trong Portal Mode: phát âm thanh Magic Chime xác nhận nút bấm hoạt động 100%
                        AudioService::play(SOUND_PORTAL_ACTIVE);
                        Serial.println("[Button] Click in Portal Mode -> Test Chime Played");
                    }
                }
            }
        }
    }
}

void setup() {
    // Vô hiệu hóa reset do sụt áp nguồn tức thời khi loa công suất lớn phát âm thanh
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println("\n=== ESP32-S3 Smart Camera & Clock Calendar Pipeline ===");

    // Cấu hình nút nhấn BOOT (GPIO 0)
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

    // 1. Khởi tạo Khối Hiển thị màn hình ST7789
    DisplayService::init();

    // 2. Khởi tạo Khối Đèn LED Chỉ Báo Trạng Thái (GPIO 48)
    LedService::init();

    // 3. Khởi tạo Khối Âm thanh I2S MAX98357A
    AudioService::init();

    // Khởi chạy Button Task độc lập trên Core 0 (sau khi Audio/Display đã sẵn sàng)
    xTaskCreatePinnedToCore(buttonTaskWorker, "ButtonTask", 3072, NULL, 1, NULL, 0);

    // 4. Tải cấu hình từ NVS
    appConfig = PortalService::loadConfig();
    LedService::setEnabled(appConfig.led_enabled);
    AudioService::setEnabled(appConfig.audio_enabled);
    AudioService::setVolume(appConfig.audio_volume);
    Serial.printf("[Setup] Loaded Config -> SSID: '%s', Audio: %s (%d%%)\n",
        appConfig.wifi_ssid.c_str(), appConfig.audio_enabled ? "ON" : "OFF", appConfig.audio_volume);

    // 5. Kiểm tra chế độ khởi động (AP Portal hay WiFi STA)
    if (appConfig.wifi_ssid.isEmpty()) {
        Serial.println("[Setup] WiFi SSID is empty -> Starting Captive Portal AP Mode (ESP32S3-CAM-AP)...");
        AudioService::play(SOUND_PORTAL_ACTIVE);
        PortalService::startCaptivePortal();
        
        // Hiển thị màn hình hướng dẫn cấu hình Tiếng Việt có dấu trực quan trên TFT ST7789
        DisplayService::showPortalScreen("ESP32S3-CAM-AP", "192.168.4.1");
        return;
    }

    // ================= CHẾ ĐỘ NORMAL (STA WIFI) =================
    Serial.println("[Setup] Starting Normal Mode with WiFi STA...");
    AudioService::play(SOUND_STARTUP);

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

    WeatherService::init(appConfig.weather_city.c_str(), appConfig.weather_lat, appConfig.weather_lon);
    MarketService::init();

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

    // Cập nhật dịch vụ Thời tiết & Thị trường (Giá vàng & Xăng dầu)
    WeatherService::update(wifi_ok);
    MarketService::update(wifi_ok);

    // Quản lý trạng thái tự động chuyển đổi Standby Clock & Calendar
    if (!aiResult.faces.empty()) {
        last_face_seen_time = now;
        if (DisplayService::getMode() == DISPLAY_MODE_STANDBY_CLOCK) {
            DisplayService::setMode(DISPLAY_MODE_CAMERA);
        }
        LedService::triggerFaceDetected();
        AudioService::triggerFaceDetected();
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
            MarketService::getMarket(),
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
