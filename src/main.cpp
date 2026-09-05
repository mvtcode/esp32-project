#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include "pin_config.h"
#include "log.h"
#include "services/storage_service.h"
#include "services/audio_i2s_service.h"
#include "services/video_player_service.h"
#include "ui/video_ui.h"

static const char *TAG = "Main";

// Các đối tượng phần cứng và dịch vụ
static TFT_eSPI tft = TFT_eSPI();
static AudioI2sService audioService;
static VideoUI videoUI(tft);
static VideoPlayerService playerService(tft, audioService, videoUI);

// Trạng thái nút bấm và cảm ứng (Debounce)
static uint32_t lastButtonPress = 0;
static uint32_t lastTouchPress = 0;

// Hàm đọc cảm ứng điện dung FT6336G qua I2C (Address 0x38)
static bool getTouchCoordinates(uint16_t* x, uint16_t* y) {
#if defined(PIN_TOUCH_SDA) && defined(PIN_TOUCH_SCL)
    Wire.beginTransmission(0x38);
    Wire.write(0x02); // TD_STATUS: số điểm chạm
    if (Wire.endTransmission() != 0) {
        return false;
    }
    if (Wire.requestFrom((uint8_t)0x38, (uint8_t)5) != 5) {
        return false;
    }
    uint8_t points = Wire.read() & 0x0F;
    if (points == 0) {
        return false;
    }
    uint8_t xHigh = Wire.read();
    uint8_t xLow = Wire.read();
    uint8_t yHigh = Wire.read();
    uint8_t yLow = Wire.read();

    uint16_t rawX = ((xHigh & 0x0F) << 8) | xLow;
    uint16_t rawY = ((yHigh & 0x0F) << 8) | yLow;

    // FT6336G native: 240 (X) x 320 (Y)
    // TFT rotation 1 (Landscape: 320 x 240):
    // X_screen = rawY (0..319)
    // Y_screen = 239 - rawX (0..239)
    if (rawY > 319) rawY = 319;
    if (rawX > 239) rawX = 239;

    *x = rawY;
    *y = 239 - rawX;
    return true;
#else
    return false;
#endif
}

void setup() {
    // 1. Cấu hình đèn nền màn hình (Backlight) sáng ngay lập tức
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);

    Serial.begin(115200);
#if ARDUINO_USB_CDC_ON_BOOT
    Serial.setTxTimeoutMs(0); // Không chặn/treo CPU khi không mở Serial Monitor
#endif
    delay(100);

    LOG_I(TAG, "=============================================");
    LOG_I(TAG, "ESP32-S3 2.8 Video Player (320x240 + I2S)   ");
    LOG_I(TAG, "=============================================");

    // Kiểm tra PSRAM
    if (psramFound()) {
        LOG_I(TAG, "PSRAM khả dụng: %u KB", (uint32_t)(ESP.getPsramSize() / 1024));
    } else {
        LOG_W(TAG, "Không tìm thấy PSRAM, hệ thống dùng Internal Heap!");
    }

    // 0. Khởi tạo bus I2C cho Touch FT6336G và Codec ES8311
#if defined(PIN_TOUCH_SDA) && defined(PIN_TOUCH_SCL)
    Wire.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL);
#endif

    // 2. Cấu hình nút bấm BOOT (GPIO0)
    pinMode(PIN_BUTTON_BOOT, INPUT_PULLUP);

    // 3. Khởi tạo màn hình (Landscape: 320x240)
    LOG_I(TAG, "2. Đang khởi tạo màn hình TFT (320x240)...");
    tft.init();
    tft.setRotation(1); // Chế độ nằm ngang (Landscape: 320x240)
    tft.invertDisplay(true); // Sửa lỗi màn hình IPS (ES3C28P) bị âm bản (negative colors)
    tft.fillScreen(TFT_BLACK);
    LOG_I(TAG, "Màn hình TFT đã khởi tạo thành công");

    // Hiển thị màn hình chờ khởi động
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("ESP32-S3 Video Player", 40, 80, 4);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Dang kiem tra the nho MicroSD...", 50, 130, 2);

    // 4. Khởi tạo thẻ nhớ MicroSD
    LOG_I(TAG, "3. Đang kiểm tra kết nối thẻ nhớ MicroSD...");
    if (!StorageService::begin()) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("LOI THE NHO MICROSD!", 50, 80, 4);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString("Vui long cam the MicroSD (FAT32)", 40, 130, 2);
        tft.drawString("va nhan nut RESET de thu lai.", 50, 160, 2);
        LOG_E(TAG, "Thẻ MicroSD không thể kết nối.");
        while (true) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    LOG_I(TAG, "Thẻ nhớ MicroSD đã sẵn sàng");

    // 5. Khởi tạo dịch vụ âm thanh I2S DMA
    LOG_I(TAG, "4. Khởi tạo dịch vụ âm thanh I2S DMA...");
    if (!audioService.begin()) {
        LOG_E(TAG, "Lỗi khởi tạo AudioI2sService!");
    }

    // 6. Khởi tạo Video Player & UI
    LOG_I(TAG, "5. Khởi tạo Video Player & UI...");
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
        tft.drawString("KHONG TIM THAY VIDEO!", 40, 70, 4);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString("Vui long copy vao the nho:", 60, 115, 2);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawString("/esp32-video/video.avi", 70, 145, 2);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString("(320x180 16:9 Cinema Mode)", 60, 175, 2);
        LOG_W(TAG, "Không tìm thấy file video hợp lệ trên thẻ nhớ.");
        while (true) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    // 8. Nạp video: Render frame đầu tiên và dừng ở trạng thái PAUSED
    tft.fillScreen(TFT_BLACK);
    if (playerService.openVideo(videoToPlay, audioToPlay, 20)) {
        LOG_I(TAG, "Video da san sang (320x180 @ 20fps). Dang PAUSE, cho user bam Play.");
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

    // 2. Kiểm tra cảm ứng chạm trên màn hình (Touch Screen) — Throttle 100ms để tiết kiệm I2C bus time
    static uint32_t lastTouchCheck = 0;
    if (now - lastTouchCheck >= 100) {  // Chỉ check touch 10 lần/giây (Wire I2C thiều ~2-5ms/lần)
        lastTouchCheck = now;
        uint16_t touchX, touchY;
        if (getTouchCoordinates(&touchX, &touchY)) {
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
    }

    // 3. Cập nhật chu trình phát video & đồng bộ âm thanh
    playerService.update();

    // 4. Nhường thời gian thực thi (Non-blocking)
    // taskYIELD() khi đang phát: chỉ nhường time-slice không block CPU (~0μs, so với vTaskDelay(1) block 1ms)
    // vTaskDelay(1) khi pause: đủ cho IDLE task reset Watchdog Timer
    if (playerService.isPlaying()) {
        taskYIELD();
    } else {
        vTaskDelay(1);
    }
}