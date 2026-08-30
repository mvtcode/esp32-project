#include "audio_service.h"
#include <math.h>

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 256

bool AudioService::is_initialized = false;
bool AudioService::is_enabled = true;
uint8_t AudioService::volume_percent = 40;
QueueHandle_t AudioService::soundQueue = NULL;
TaskHandle_t AudioService::audioTaskHandle = NULL;
uint32_t AudioService::last_face_sound_time = 0;

static int16_t audio_pcm_buf[BUFFER_SIZE * 2];

void AudioService::playToneDirect(float freq, uint32_t duration_ms, float gain, float end_freq) {
    if (!is_initialized || !is_enabled || volume_percent == 0) return;

    float vol = (volume_percent / 100.0f) * gain;
    if (vol > 1.0f) vol = 1.0f;
    if (vol < 0.0f) vol = 0.0f;

    size_t total_samples = (SAMPLE_RATE * duration_ms) / 1000;
    if (total_samples == 0) return;

    size_t samples_generated = 0;
    float phase = 0.0f;

    bool is_sweep = (end_freq > 0.0f && fabsf(end_freq - freq) > 1.0f);

    while (samples_generated < total_samples) {
        size_t chunk_samples = total_samples - samples_generated;
        if (chunk_samples > BUFFER_SIZE) chunk_samples = BUFFER_SIZE;

        for (size_t i = 0; i < chunk_samples; ++i) {
            float progress = (float)(samples_generated + i) / (float)total_samples;

            // Tính tần số hiện tại (hỗ trợ sweep luyến âm mượt mà cho tiếng bloop/pop)
            float cur_freq = freq;
            if (is_sweep) {
                cur_freq = freq + (end_freq - freq) * progress;
            }
            float phase_step = (2.0f * M_PI * cur_freq) / SAMPLE_RATE;

            // ADSR Envelope: Attack nhanh nảy 6%, Decay mũ êm ái kiểu hộp nhạc pha lê
            float envelope = 1.0f;
            if (progress < 0.06f) {
                envelope = progress / 0.06f;
            } else {
                float decay_progress = (progress - 0.06f) / 0.94f;
                envelope = (1.0f - decay_progress) * (1.0f - decay_progress); // Exponential decay
            }

            // Sóng âm hộp nhạc pha lê (Celesta / Marimba): Sine chính + Họa âm quãng 8 + Chime nhẹ
            float sample_val = 0.75f * sinf(phase) 
                             + 0.20f * sinf(phase * 2.0f) 
                             + 0.05f * sinf(phase * 3.0f);

            phase += phase_step;
            if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;

            // Biên độ an toàn 12000 giúp âm thanh trong trẻo và không làm tụt áp nguồn USB
            int16_t pcm_sample = (int16_t)(sample_val * envelope * vol * 12000.0f);

            // Ghi ra 2 kênh Trái - Phải
            audio_pcm_buf[i * 2]     = pcm_sample; // Left
            audio_pcm_buf[i * 2 + 1] = pcm_sample; // Right
        }

        size_t bytes_written = 0;
        i2s_write(I2S_SPEAKER_NUM, audio_pcm_buf, chunk_samples * 2 * sizeof(int16_t), &bytes_written, portMAX_DELAY);
        samples_generated += chunk_samples;
    }

    // Đệm một đoạn ngắn im lặng để chống click âm
    memset(audio_pcm_buf, 0, 128 * sizeof(int16_t));
    size_t bytes_written = 0;
    i2s_write(I2S_SPEAKER_NUM, audio_pcm_buf, 128 * sizeof(int16_t), &bytes_written, portMAX_DELAY);
}

void AudioService::playSoundEffect(SoundType type) {
    if (!is_enabled) return;

    switch (type) {
        case SOUND_STARTUP: {
            // Giai điệu Nintendo / Tamagotchi tươi sáng 5 nốt: E5 -> G5 -> C6 -> E6 -> G6
            playToneDirect(659.25f, 55, 0.75f);
            vTaskDelay(pdMS_TO_TICKS(15));
            playToneDirect(783.99f, 55, 0.80f);
            vTaskDelay(pdMS_TO_TICKS(15));
            playToneDirect(1046.50f, 65, 0.85f);
            vTaskDelay(pdMS_TO_TICKS(15));
            playToneDirect(1318.51f, 75, 0.90f);
            vTaskDelay(pdMS_TO_TICKS(15));
            playToneDirect(1567.98f, 220, 1.0f);
            break;
        }

        case SOUND_FACE_DETECTED: {
            // Âm "Ting-Ting! / Kira-kira" nhận diện khuôn mặt siêu dễ thương: G6 -> C7 chuông ngân
            playToneDirect(1567.98f, 60, 0.85f);
            vTaskDelay(pdMS_TO_TICKS(20));
            playToneDirect(2093.00f, 200, 1.0f);
            break;
        }

        case SOUND_WIFI_CONNECTED: {
            // Âm "Tada! Level-up" khi có WiFi: F5 -> A5 -> C6 -> F6
            playToneDirect(698.46f, 50, 0.75f);
            vTaskDelay(pdMS_TO_TICKS(15));
            playToneDirect(880.00f, 50, 0.80f);
            vTaskDelay(pdMS_TO_TICKS(15));
            playToneDirect(1046.50f, 60, 0.85f);
            vTaskDelay(pdMS_TO_TICKS(15));
            playToneDirect(1396.91f, 180, 0.95f);
            break;
        }

        case SOUND_UPLOAD_SUCCESS: {
            // Âm ăn xu "Coin / Bling!" vui nhộn: B5 -> E6
            playToneDirect(987.77f, 45, 0.80f);
            vTaskDelay(pdMS_TO_TICKS(10));
            playToneDirect(1318.51f, 160, 0.95f);
            break;
        }

        case SOUND_UPLOAD_FAILED: {
            // Âm "Oops / Boing" trượt tần số vui nhộn
            playToneDirect(880.00f, 120, 0.85f, 440.00f);
            vTaskDelay(pdMS_TO_TICKS(20));
            playToneDirect(440.00f, 160, 0.80f, 293.66f);
            break;
        }

        case SOUND_BUTTON_CLICK: {
            // Tiếng bong bóng nước nổ "Bubble Pop / Bloop!" cực nảy
            playToneDirect(1100.0f, 35, 0.85f, 2400.0f);
            break;
        }

        case SOUND_PORTAL_ACTIVE: {
            // Âm mở AP chào mừng: C6 -> E6 -> G6 -> C7
            playToneDirect(1046.50f, 55, 0.80f);
            vTaskDelay(pdMS_TO_TICKS(15));
            playToneDirect(1318.51f, 55, 0.85f);
            vTaskDelay(pdMS_TO_TICKS(15));
            playToneDirect(1567.98f, 65, 0.90f);
            vTaskDelay(pdMS_TO_TICKS(15));
            playToneDirect(2093.00f, 180, 0.95f);
            break;
        }

        default:
            break;
    }
}

void AudioService::audioTaskWorker(void *param) {
    SoundType sound;
    while (true) {
        if (xQueueReceive(soundQueue, &sound, portMAX_DELAY) == pdTRUE) {
            playSoundEffect(sound);
        }
    }
}

bool AudioService::init() {
    if (is_initialized) return true;

    Serial.printf("[AudioService] Initializing I2S Speaker MAX98357A (BCLK: %d, LRC: %d, DIN: %d)...\n",
        I2S_SPEAKER_BCLK_PIN, I2S_SPEAKER_LRC_PIN, I2S_SPEAKER_DIN_PIN);

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SPEAKER_BCLK_PIN,
        .ws_io_num = I2S_SPEAKER_LRC_PIN,
        .data_out_num = I2S_SPEAKER_DIN_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    esp_err_t err = i2s_driver_install(I2S_SPEAKER_NUM, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[AudioService] i2s_driver_install failed: %d\n", err);
        return false;
    }

    err = i2s_set_pin(I2S_SPEAKER_NUM, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("[AudioService] i2s_set_pin failed: %d\n", err);
        return false;
    }

    i2s_zero_dma_buffer(I2S_SPEAKER_NUM);

    soundQueue = xQueueCreate(4, sizeof(SoundType));
    if (soundQueue == NULL) {
        Serial.println("[AudioService] Failed to create soundQueue");
        return false;
    }

    // Chạy Audio Worker trên Core 0 với mức ưu tiên vừa phải để không làm gián đoạn Video
    BaseType_t res = xTaskCreatePinnedToCore(
        audioTaskWorker,
        "AudioWorker",
        4096,
        NULL,
        1,
        &audioTaskHandle,
        0
    );

    if (res != pdPASS) {
        Serial.println("[AudioService] Failed to create AudioWorker Task");
        return false;
    }

    i2s_start(I2S_SPEAKER_NUM);
    is_initialized = true;
    Serial.println("[AudioService] I2S Audio Initialized Successfully!");
    return true;
}

void AudioService::setEnabled(bool enabled) {
    is_enabled = enabled;
}

void AudioService::setVolume(uint8_t volume) {
    if (volume > 100) volume = 100;
    volume_percent = volume;
}

void AudioService::play(SoundType type) {
    if (!is_initialized || !is_enabled || soundQueue == NULL) return;
    xQueueSend(soundQueue, &type, 0); // Non-blocking
}

void AudioService::triggerFaceDetected() {
    uint32_t now = millis();
    // Cooldown 3 giây giữa các lần thông báo phát hiện khuôn mặt để không gây ồn ào liên tục
    if (now - last_face_sound_time >= 3000) {
        last_face_sound_time = now;
        play(SOUND_FACE_DETECTED);
    }
}
