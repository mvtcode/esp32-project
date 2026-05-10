#include "voice_engine.h"
#include "../audio/i2s_mic.h"
#include "../display/ui_manager.h"
#include "vector_math.h"
#include "vector_storage.h"
#include <Arduino.h>
#include <string.h>

// ─── Constants ───────────────────────────────────────────────────────────────
#define SIMILARITY_THRESHOLD  0.75f  // Lower is easier to match
#define VAD_THRESHOLD         800    // Cân bằng với Gain x32 để bắt chính xác tiếng nói
#define VAD_SILENCE_MS        1500   // 1.5s silence to finalize
#define SAMPLES_PER_CMD       1

#define MAX_AUDIO_SAMPLES     (16000 * 3) // 3 seconds max
#define PRE_CHUNKS            5           // Keep last 150ms
#define FRAME_SAMPLES         480         // 30ms

// ─── State ───────────────────────────────────────────────────────────────────
static bool _recognition_enabled = false;
static bool _is_training = false;
static bool _is_speaking = false;
static bool _is_processing = false;
static uint32_t _silence_start_ms = 0;
static uint32_t _last_recog_ms = 0;

static int _training_gpio = -1;
static bool _training_on_cmd = true;
static int _training_progress = 0;
static uint32_t _training_start_ms = 0;
static float _training_vectors[SAMPLES_PER_CMD][VECTOR_DIM];

static float _wake_vector[VECTOR_DIM];
static bool _wake_loaded = false;

static float _on_vectors[VOICE_CMD_COUNT][VECTOR_DIM];
static bool _on_loaded[VOICE_CMD_COUNT];

static float _off_vectors[VOICE_CMD_COUNT][VECTOR_DIM];
static bool _off_loaded[VOICE_CMD_COUNT];

static int16_t* s_audio_buffer = nullptr;
static int s_audio_len = 0;

static voice_cmd_cb_t _cmd_cb = nullptr;
static int _last_rms = 0;

// ─── Internal ────────────────────────────────────────────────────────────────

static void extract_vector(const int16_t* audio, int len, float* out_vec) {
    if (len <= 0) {
        for(int i=0; i<VECTOR_DIM; i++) out_vec[i] = 0.0f;
        return;
    }
    // Dummy implementation: in a real system, you'd call esp_afe or your DSP lib
    // For demo, we just fill with random/based on RMS
    float sum = 0;
    for(int i=0; i<len; i++) sum += abs(audio[i]);
    float avg = sum / len;
    
    for(int i=0; i<VECTOR_DIM; i++) {
        out_vec[i] = (avg / 1000.0f) + ((float)rand() / RAND_MAX * 0.1f);
    }
}

// ─── API ─────────────────────────────────────────────────────────────────────

bool voice_engine_init() {
    s_audio_buffer = (int16_t*)ps_malloc(MAX_AUDIO_SAMPLES * sizeof(int16_t));
    if (!s_audio_buffer) return false;

    if (!i2s_mic_init()) return false;
    if (!vector_storage_init()) return false;

    // Load command cache
    Serial.println("[VOICE] Loading command cache...");
    if (vector_storage_load(VOICE_CMD_WAKE, true, _wake_vector)) {
        _wake_loaded = true;
        Serial.println("[VOICE] Wake Word cached.");
    }
    
    Serial.println("[VOICE] Cache loaded.");
    return true;
}

void voice_engine_set_callback(voice_cmd_cb_t cb) {
    _cmd_cb = cb;
}

void voice_engine_start_training(int gpio_idx, bool on_cmd) {
    _training_gpio = gpio_idx;
    _training_on_cmd = on_cmd;
    _training_progress = 0;
    _is_training = true;
    _training_start_ms = millis();
    Serial.printf("[VOICE] Started training GPIO %d (%s). Waiting for speech...\n", gpio_idx, on_cmd ? "ON" : "OFF");
}

static bool _pending_save = false;

void voice_engine_stop_training(bool save) {
    if (save && _is_training && _training_progress > 0) {
        _pending_save = true;
    } else {
        _is_training = false;
        _training_progress = 0;
        _is_speaking = false;
        _silence_start_ms = 0;
    }
}

int voice_engine_get_training_progress() {
    return _training_progress;
}

int voice_engine_get_last_rms() {
    return _last_rms;
}

void voice_engine_stop_speaking() {
    if (_is_speaking) {
        _silence_start_ms = 1; // Ép kết thúc ngay lập tức
        Serial.println("[VOICE] Force finalized by user.");
    }
}

bool voice_engine_has_command(int gpio_idx, bool on_cmd) {
    if (gpio_idx == VOICE_CMD_WAKE) return _wake_loaded;
    if (gpio_idx >= 0 && gpio_idx < VOICE_CMD_COUNT) {
        return on_cmd ? _on_loaded[gpio_idx] : _off_loaded[gpio_idx];
    }
    return false;
}

bool voice_engine_is_listening() {
    return _recognition_enabled && !_is_training;
}

bool voice_engine_is_speaking() {
    return _is_speaking;
}

bool voice_engine_is_processing() {
    return _is_processing;
}

const char* voice_engine_cmd_name(int cmd_id) {
    static char buf[32];
    if (cmd_id == VOICE_CMD_WAKE) return "WAKE WORD";
    if (cmd_id < VOICE_CMD_COUNT) snprintf(buf, sizeof(buf), "GPIO %d ON", cmd_id);
    else if (cmd_id >= 10 && cmd_id < 10 + VOICE_CMD_COUNT) snprintf(buf, sizeof(buf), "GPIO %d OFF", cmd_id - 10);
    else snprintf(buf, sizeof(buf), "ID %d", cmd_id);
    return buf;
}

// ─── FreeRTOS Task ───────────────────────────────────────────────────────────

void voice_engine_task(void* arg) {
    int16_t audio_buf[FRAME_SAMPLES];
    static float current_vector[VECTOR_DIM];
    
    // Đưa pre_record sang static để giải phóng Stack
    static int16_t pre_record[PRE_CHUNKS][FRAME_SAMPLES];
    int pre_idx = 0;

    int n = 0;
    int32_t rms = 0;
    ScreenID current_screen = SCREEN_HOME;

    while (true) {
        // Handle SPIFFS save in this task to prevent InstrFetchProhibited
        if (_pending_save) {
            _pending_save = false;
            static float avg_vector[VECTOR_DIM];
            memset(avg_vector, 0, sizeof(avg_vector));
            for(int d=0; d<VECTOR_DIM; d++) {
                for(int s=0; s<_training_progress; s++) {
                    avg_vector[d] += _training_vectors[s][d];
                }
                avg_vector[d] /= _training_progress;
            }
            float norm = vector_magnitude(avg_vector, VECTOR_DIM);
            if (norm > 0) {
                for(int d=0; d<VECTOR_DIM; d++) avg_vector[d] /= norm;
            }

            if (_training_gpio == VOICE_CMD_WAKE) {
                vector_storage_save(VOICE_CMD_WAKE, true, avg_vector);
                memcpy(_wake_vector, avg_vector, sizeof(_wake_vector));
                _wake_loaded = true;
                Serial.println("[VOICE] Wake Word Cache updated.");
            } else {
                vector_storage_save(_training_gpio, _training_on_cmd, avg_vector);
                if (_training_on_cmd) {
                    memcpy(_on_vectors[_training_gpio], avg_vector, sizeof(avg_vector));
                    _on_loaded[_training_gpio] = true;
                } else {
                    memcpy(_off_vectors[_training_gpio], avg_vector, sizeof(avg_vector));
                    _off_loaded[_training_gpio] = true;
                }
                Serial.printf("[VOICE] GPIO %d %s Cache updated.\n", _training_gpio, _training_on_cmd ? "ON" : "OFF");
            }
            Serial.println("[VOICE] Manual save complete.");
            
            _is_training = false;
            _training_progress = 0;
            _is_speaking = false;
            _silence_start_ms = 0;
        }

        // Check forced stop signal early
        if (_is_speaking && _silence_start_ms == 1) {
            Serial.println("[VOICE] Stop signal detected, finalizing...");
            _is_processing = true;
            goto finalize_record;
        }

        n = i2s_mic_read(audio_buf, FRAME_SAMPLES);
        if (n <= 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        // Calculate RMS
        rms = 0;
        for (int i = 0; i < n; i++) rms += abs(audio_buf[i]);
        rms /= n;
        
        // Ignore mechanical button click noise in the first 250ms of training
        if (_is_training && (millis() - _training_start_ms < 250)) {
            rms = 0;
        }
        
        _last_rms = (int)rms;

        current_screen = ui_manager_get_current_screen();
        _recognition_enabled = (current_screen == SCREEN_HOME);

        if (!_is_training && !_recognition_enabled) {
            _is_speaking = false;
            _silence_start_ms = 0;
            s_audio_len = 0;
        } else {
            if (rms > VAD_THRESHOLD) {
                if (!_is_speaking) {
                    if (!_is_training && (millis() - _last_recog_ms < 1500)) {
                        // Cooldown
                    } else {
                        // CHỈ BẮT ĐẦU NẾU BUFFER HỢP LỆ
                        if (s_audio_buffer != nullptr) {
                            _is_speaking = true;
                            _silence_start_ms = 0;
                            s_audio_len = 0;
                            
                            // Copy the past 150ms from ring buffer
                            for (int i = 0; i < PRE_CHUNKS; i++) {
                                int idx = (pre_idx + i) % PRE_CHUNKS;
                                memcpy(&s_audio_buffer[s_audio_len], pre_record[idx], FRAME_SAMPLES * sizeof(int16_t));
                                s_audio_len += FRAME_SAMPLES;
                            }
                            Serial.println("[VOICE] Started listening to voice...");
                        }
                    }
                }
            }

            if (_is_speaking && s_audio_buffer != nullptr) {
                // Accumulate audio samples
                if (s_audio_len + n <= MAX_AUDIO_SAMPLES) {
                    memcpy(&s_audio_buffer[s_audio_len], audio_buf, n * sizeof(int16_t));
                    s_audio_len += n;
                }

                if (rms <= VAD_THRESHOLD || _silence_start_ms == 1) {
                    // Nếu là 1 (ép dừng) -> Xử lý ngay. Nếu là 0 -> Bắt đầu đếm thời gian im lặng.
                    if (_silence_start_ms == 0 && rms <= VAD_THRESHOLD) {
                        _silence_start_ms = millis();
                    }
                    
                    // Kiểm tra điều kiện dừng: Hoặc là lệnh ép dừng (1), hoặc là đã im lặng quá 1.5s
                    if (_silence_start_ms == 1 || (_silence_start_ms > 1 && (millis() - _silence_start_ms > VAD_SILENCE_MS))) {
                        
                    finalize_record: // NHÃN ĐỂ NHẢY TỚI KHI NHẤN ENTER
                        _is_processing = true; 
                        
                        // 1. Trimming trailing silence
                        if (_silence_start_ms > 1) {
                            int silence_samples = (16000 * (millis() - _silence_start_ms)) / 1000;
                            if (s_audio_len > silence_samples) s_audio_len -= silence_samples;
                        }

                        // 2. Extract feature vector
                        extract_vector(s_audio_buffer, s_audio_len, current_vector);
                        
                        if (_is_training) {
                            memcpy(_training_vectors[_training_progress], current_vector, sizeof(float) * VECTOR_DIM);
                            _training_progress++;
                            _is_speaking = false;
                            _silence_start_ms = 0;
                            _is_processing = false;
                            Serial.printf("[VOICE] Sample %d captured (%d bytes)\n", _training_progress, s_audio_len * 2);

                            // Dump audio for Python script (Little Endian format cho WAV)
                            Serial.printf("\n---AUDIO_START:APP_TRAIN_%d---\n", _training_progress);
                            for(int i = 0; i < s_audio_len; i++) {
                                uint16_t v = (uint16_t)s_audio_buffer[i];
                                Serial.printf("%02X%02X", v & 0xFF, v >> 8);
                                if ((i + 1) % 64 == 0) Serial.println();
                            }
                            Serial.println("\n---AUDIO_END---");

                        } else {
                            // Wake word or Command logic here
                            // ...
                            _last_recog_ms = millis();
                            _is_speaking = false;
                            _silence_start_ms = 0;
                            _is_processing = false;
                        }
                    }
                } else {
                    _silence_start_ms = 0; // Reset silence timer if sound detected
                }
            } else {
                // Not speaking, keep ring buffer
                memcpy(pre_record[pre_idx], audio_buf, FRAME_SAMPLES * sizeof(int16_t));
                pre_idx = (pre_idx + 1) % PRE_CHUNKS;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
