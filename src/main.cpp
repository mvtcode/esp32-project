#include <Arduino.h>
#include <TFT_eSPI.h>
#include "pin_config.h"
#include "log.h"
#include "services/storage_service.h"
#include "services/audio_dac_service.h"
#include "services/video_player_service.h"
#include "ui/video_ui.h"

static const char *TAG = "Main";

// Các đối tượng phần cứng và dịch vụ
static TFT_eSPI tft = TFT_eSPI();
static AudioDacService audioService;
static VideoUI videoUI(tft);
static VideoPlayerService playerService(tft, audioService, videoUI);

// Trạng thái nút bấm và cảm ứng (Debounce)
static uint32_t lastButtonPress = 0;
static uint32_t lastTouchPress = 0;

void setup() {
    Serial.begin(115200);
    delay(500);

    LOG_I(TAG, "=============================================");
    LOG_I(TAG, "ESP32 CYD 3.5 Video Player (ST7796 + DAC)   ");
    LOG_I(TAG, "=============================================");

    // 1. Cấu hình đèn nền màn hình (Backlight)
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);

    // 2. Cấu hình nút bấm BOOT (GPIO0)
    pinMode(PIN_BUTTON_BOOT, INPUT_PULLUP);

    // 3. Khởi tạo màn hình ST7796
    tft.init();
    tft.setRotation(1); // Chế độ nằm ngang (Landscape: 480x320)
    tft.fillScreen(TFT_BLACK);

    // Cấu hình thông số hiệu chuẩn cảm ứng XPT2046 cho CYD 3.5"
    uint16_t calData[5] = { 334, 3478, 384, 3435, 3 };
    tft.setTouch(calData);

    // Hiển thị màn hình chờ khởi động
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("ESP32 CYD 3.5 Video Player", 90, 130, 4);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Dang kiem tra the nho MicroSD...", 120, 170, 2);

    // 4. Khởi tạo thẻ nhớ MicroSD trên bus VSPI
    if (!StorageService::begin()) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("LOI THE NHO MICROSD!", 130, 120, 4);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString("Vui long cam the MicroSD (dinh dang FAT32)", 90, 160, 2);
        tft.drawString("va nhan nut RESET de khoi dong lai.", 115, 190, 2);
        LOG_E(TAG, "Thẻ MicroSD không thể kết nối.");
        while (true) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    // 5. Khởi tạo dịch vụ âm thanh DAC (GPIO26)
    if (!audioService.begin()) {
        LOG_E(TAG, "Lỗi khởi tạo AudioDacService!");
    }

    // 6. Khởi tạo Video Player & UI
    if (!playerService.begin()) {
        LOG_E(TAG, "Lỗi khởi tạo VideoPlayerService!");
    }
    videoUI.init();

    // 7. Kiểm tra các file media trên thẻ nhớ:
    // Ưu tiên 1: File video.avi (All-in-One chứa cả video và audio) -> Chỉ cần 1 file duy nhất!
    const char* aviPath = "/esp32-video/video.avi";
    const char* mjpegPath = "/esp32-video/video.mjpeg";
    const char* audioPath = "/esp32-video/audio.wav";

    const char* videoToPlay = nullptr;
    const char* audioToPlay = nullptr;

    if (StorageService::fileExists(aviPath)) {
        LOG_I(TAG, "Phát hiện file AVI All-in-One: %s", aviPath);
        videoToPlay = aviPath;
        audioToPlay = nullptr; // AVI tự chứa audio
    } else if (StorageService::fileExists(mjpegPath)) {
        if (StorageService::fileExists(audioPath)) {
            LOG_I(TAG, "Phát hiện file MJPEG + WAV: %s & %s", mjpegPath, audioPath);
            videoToPlay = mjpegPath;
            audioToPlay = audioPath;
        } else {
            LOG_W(TAG, "Tìm thấy video.mjpeg nhưng thiếu audio.wav!");
        }
    }

    if (!videoToPlay) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawString("KHONG TIM THAY FILE VIDEO!", 95, 110, 4);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString("Vui long copy file vao the nho:", 125, 155, 2);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawString("/esp32-video/video.avi", 140, 185, 2);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString("(Chua ca hinh anh va am thanh)", 125, 215, 2);
        LOG_W(TAG, "Không tìm thấy file video hợp lệ trên thẻ nhớ.");
        while (true) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    // 8. Nạp video: Render frame đầu tiên và dừng ở trạng thái PAUSED
    tft.fillScreen(TFT_BLACK);
    if (playerService.openVideo(videoToPlay, audioToPlay, 20)) {
        LOG_I(TAG, "Video da san sang. Dang o trang thai PAUSE, cho user bam Play.");
    } else {
        LOG_E(TAG, "Lỗi khi nạp file video!");
    }
}

void loop() {
    uint32_t now = millis();

    // 1. Kiểm tra nút bấm cứng BOOT (GPIO0 - Active LOW)
    if (digitalRead(PIN_BUTTON_BOOT) == LOW) {
        if (now - lastButtonPress > 350) { // Chống rung phím 350ms
            lastButtonPress = now;
            LOG_I(TAG, "Nhan nut BOOT (GPIO0) -> Chuyen trang thai Play/Pause");
            playerService.togglePlayPause();
        }
    }

    // 2. Kiểm tra cảm ứng chạm trên màn hình (Touch Screen)
    uint16_t touchX, touchY;
    if (tft.getTouch(&touchX, &touchY)) {
        if (now - lastTouchPress > 350) { // Chống rung chạm 350ms
            lastTouchPress = now;
            if (playerService.isPlaying()) {
                if (!videoUI.isOverlayVisible()) {
                    // Nếu đang phát và overlay đang ẩn: Chạm để đánh thức Header, Footer, Center hiện lên 1.5s
                    LOG_I(TAG, "Cham cam ung tai (%d, %d) -> Danh thuc Overlay", touchX, touchY);
                    videoUI.triggerOverlay(1500);
                } else {
                    // Nếu overlay đang hiển thị: Chạm để tạm dừng
                    LOG_I(TAG, "Cham cam ung tai (%d, %d) -> Tạm dừng (Pause)", touchX, touchY);
                    playerService.pause();
                }
            } else {
                // Đang Pause: Chạm để tiếp tục phát
                LOG_I(TAG, "Cham cam ung tai (%d, %d) -> Tiep tuc phat (Play)", touchX, touchY);
                playerService.play();
            }
        }
    }

    // 3. Cập nhật chu trình phát video & đồng bộ âm thanh
    playerService.update();

    // 4. Nhường thời gian thực thi (Non-blocking)
    vTaskDelay(1);
}