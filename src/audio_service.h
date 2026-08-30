#ifndef AUDIO_SERVICE_H
#define AUDIO_SERVICE_H

#include <Arduino.h>
#include "driver/i2s.h"

// =========================================================================
// CẤU HÌNH CHÂN GIAO TIẾP I2S CHO LOA MAX98357A TRÊN ESP32-S3
// =========================================================================
#ifndef I2S_SPEAKER_BCLK_PIN
#define I2S_SPEAKER_BCLK_PIN  14 // Bit Clock
#endif

#ifndef I2S_SPEAKER_LRC_PIN
#define I2S_SPEAKER_LRC_PIN   3  // Word Select / Left-Right Clock
#endif

#ifndef I2S_SPEAKER_DIN_PIN
#define I2S_SPEAKER_DIN_PIN   42 // Serial Data Out
#endif

#define I2S_SPEAKER_NUM       I2S_NUM_1 // Dùng I2S1 độc lập để không xung đột DMA với Camera I2S0/CAM

// Các loại hiệu ứng âm thanh hệ thống
enum SoundType {
    SOUND_NONE = 0,
    SOUND_STARTUP,          // Khởi động hệ thống (Arpeggio 4 nốt tươi sáng)
    SOUND_FACE_DETECTED,    // Phát hiện khuôn mặt (Ding-Dong / Ting)
    SOUND_WIFI_CONNECTED,   // Kết nối WiFi thành công (Chime 2 nốt cao)
    SOUND_UPLOAD_SUCCESS,   // Upload Google Drive thành công (Ting nhẹ)
    SOUND_UPLOAD_FAILED,    // Upload thất bại / Lỗi (2 nốt cảnh báo trầm)
    SOUND_BUTTON_CLICK,     // Phản hồi bấm nút BOOT (Tick ngắn 25ms)
    SOUND_PORTAL_ACTIVE     // Mở chế độ Captive Portal AP (3 nốt chào mừng)
};

class AudioService {
private:
    static bool is_initialized;
    static bool is_enabled;
    static uint8_t volume_percent; // 0 - 100%
    static QueueHandle_t soundQueue;
    static TaskHandle_t audioTaskHandle;
    static uint32_t last_face_sound_time;

    static void audioTaskWorker(void *param);
    static void playToneDirect(float freq, uint32_t duration_ms, float gain, float end_freq = -1.0f);
    static void playSoundEffect(SoundType type);

public:
    static bool init();
    static void setEnabled(bool enabled);
    static bool isEnabled() { return is_enabled; }
    static void setVolume(uint8_t volume); // 0 - 100
    static uint8_t getVolume() { return volume_percent; }

    // Phát âm thanh bất đồng bộ (Non-blocking)
    static void play(SoundType type);

    // Kích hoạt âm thanh nhận diện khuôn mặt có kèm Cooldown 3s chống lặp
    static void triggerFaceDetected();
};

#endif // AUDIO_SERVICE_H
