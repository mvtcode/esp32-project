#include "voice_engine.h"
#include "../audio/i2s_mic.h"
#include "../display/ui_manager.h"
#include "../control/relay_controller.h"
#include "vector_math.h"
#include "vector_storage.h"
#include <Arduino.h>
#include <SPIFFS.h>
#include <string.h>

// ─── Constants ───────────────────────────────────────────────────────────────
#define SIMILARITY_THRESHOLD  0.75f  // Lower is easier to match
#define VAD_THRESHOLD         400     // Giảm xuống để bắt tiếng nhạy hơn đồng bộ với sóng nhạc
#define VAD_SILENCE_MS        1500   // 1.5s silence to finalize
#define SAMPLES_PER_CMD       1
#define RECOG_COOLDOWN_MS     1500   // Thời gian nghỉ giữa 2 lần nhận dạng
#define TRAINING_NOISE_GATE_MS 250   // Chặn nhiễu nút nhấn lúc bắt đầu training

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

// Bật log để kiểm tra audio: python .\tools\serial_audio_dumper.py COM14
static bool _dump_training_enabled = false; // Bật log training
static bool _dump_recognition_enabled = false; // Bật log nhận dạng

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
        // Tăng độ nhạy cho Vector giả lập vì tín hiệu Raw nhỏ hơn 10 lần
        out_vec[i] = (avg / 100.0f) + ((float)rand() / RAND_MAX * 0.1f);
    }
}

// ─── API ─────────────────────────────────────────────────────────────────────

bool voice_engine_init() {
    s_audio_buffer = (int16_t*)ps_malloc(MAX_AUDIO_SAMPLES * sizeof(int16_t));
    if (!s_audio_buffer) return false;

    if (!i2s_mic_init()) return false;
    if (!vector_storage_init()) return false;

    // 1. Load Wake Word RIÊNG BIỆT (File path riêng)
    Serial.println("[VOICE] Loading Wake Word...");
    FILE* f = fopen("/spiffs/records/wake_word.bin", "rb");
    if (f) {
        if (fread(_wake_vector, sizeof(float), VECTOR_DIM, f) == VECTOR_DIM) {
            _wake_loaded = true;
            Serial.println("[VOICE] Wake Word loaded from /records/wake_word.bin");
        }
        fclose(f);
    }
    
    // 2. Load GPIO Commands
    Serial.println("[VOICE] Loading command cache...");
    for (int i = 0; i < VOICE_CMD_COUNT; i++) {
        if (vector_storage_load(i, true, _on_vectors[i])) {
            _on_loaded[i] = true;
            Serial.printf("[VOICE] GPIO %d ON cached.\n", i);
        }
        if (vector_storage_load(i, false, _off_vectors[i])) {
            _off_loaded[i] = true;
            Serial.printf("[VOICE] GPIO %d OFF cached.\n", i);
        }
    }
    
    Serial.println("[VOICE] Cache loaded.");
    return true;
}

void voice_engine_set_dump_training(bool enable) {
    _dump_training_enabled = enable;
}

void voice_engine_set_dump_recognition(bool enable) {
    _dump_recognition_enabled = enable;
}

bool voice_engine_get_dump_training() {
    return _dump_training_enabled;
}

bool voice_engine_get_dump_recognition() {
    return _dump_recognition_enabled;
}

void voice_engine_set_callback(voice_cmd_cb_t cb) {
    _cmd_cb = cb;
}

void voice_engine_start_training_wake() {
    _training_gpio = -2; // ID đặc biệt cho Wake Word, không trùng với GPIO 0-5
    _training_progress = 0;
    _is_training = true;
    _training_start_ms = millis();
    Serial.println("[VOICE] Started training WAKE WORD. Waiting for speech...");
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

bool voice_engine_has_wake_word() {
    return _wake_loaded;
}

bool voice_engine_has_command(int gpio_idx, bool on_cmd) {
    if (gpio_idx >= 0 && gpio_idx < VOICE_CMD_COUNT) {
        return on_cmd ? _on_loaded[gpio_idx] : _off_loaded[gpio_idx];
    }
    return false;
}

bool voice_engine_is_listening() {
    return _recognition_enabled && !_is_training;
}

void voice_engine_clear_all_data() {
    Serial.println("[VOICE] Clearing all training data...");
    
    // Xóa Wake Word (file mới)
    SPIFFS.remove("/records/wake_word.bin");
    
    // Xóa các lệnh GPIO (On/Off cho 6 cổng)
    for (int i = 0; i < VOICE_CMD_COUNT; i++) {
        char path_on[32], path_off[32];
        snprintf(path_on, sizeof(path_on), "/records/cmd_%d_on.bin", i);
        snprintf(path_off, sizeof(path_off), "/records/cmd_%d_off.bin", i);
        SPIFFS.remove(path_on);
        SPIFFS.remove(path_off);
        
        // Reset cache trong RAM luôn
        _on_loaded[i] = false;
        _off_loaded[i] = false;
    }
    
    // Xóa cả file ID 20 cũ (nếu có)
    SPIFFS.remove("/records/cmd_20_on.bin");
    
    _wake_loaded = false;
    Serial.println("[VOICE] All data cleared. Please restart and retrain.");
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
    
    // GPIO Commands
    if (cmd_id < VOICE_CMD_COUNT) {
        snprintf(buf, sizeof(buf), "%s ON", relay_get_alias(cmd_id));
    } else if (cmd_id >= 10 && cmd_id < 10 + VOICE_CMD_COUNT) {
        snprintf(buf, sizeof(buf), "%s OFF", relay_get_alias(cmd_id - 10));
    } else {
        snprintf(buf, sizeof(buf), "ID %d", cmd_id);
    }
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
            int count = (_training_progress > SAMPLES_PER_CMD) ? SAMPLES_PER_CMD : _training_progress;
            if (count > 0) {
                for(int d=0; d<VECTOR_DIM; d++) {
                    for(int s=0; s<count; s++) {
                        avg_vector[d] += _training_vectors[s][d];
                    }
                    avg_vector[d] /= count;
                }
            }
            
            float norm = vector_magnitude(avg_vector, VECTOR_DIM);
            if (norm > 0) {
                for(int d=0; d<VECTOR_DIM; d++) avg_vector[d] /= norm;
            }

            if (_training_gpio == -2) {
                // Lưu Wake Word vào file riêng biệt
                FILE* f_wake = fopen("/spiffs/records/wake_word.bin", "wb");
                if (f_wake) {
                    fwrite(avg_vector, sizeof(float), VECTOR_DIM, f_wake);
                    fclose(f_wake);
                    memcpy(_wake_vector, avg_vector, sizeof(_wake_vector));
                    _wake_loaded = true;
                    Serial.println("[VOICE] Wake Word saved to /records/wake_word.bin");
                }
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
        if (_is_training && (millis() - _training_start_ms < TRAINING_NOISE_GATE_MS)) {
            rms = 0;
        }
        
        _last_rms = (int)rms;
        
        // Debug RMS khi đang ở màn hình Home hoặc đang Training để bạn dễ theo dõi
        if (_is_training && (millis() % 500 < 20)) {
            Serial.printf("[DEBUG] Current RMS: %d (VAD Threshold: %d)\n", _last_rms, VAD_THRESHOLD);
        }

        current_screen = ui_manager_get_current_screen();
        _recognition_enabled = (current_screen == SCREEN_HOME);

        if (!_is_training && !_recognition_enabled) {
            _is_speaking = false;
            _silence_start_ms = 0;
            s_audio_len = 0;
        } else {
            if (rms > VAD_THRESHOLD) {
                if (!_is_speaking) {
                    if (!_is_training && (millis() - _last_recog_ms < RECOG_COOLDOWN_MS)) {
                        // Cooldown
                    } else if (_is_training && _training_progress >= SAMPLES_PER_CMD) {
                        // Đã thu đủ mẫu, không thu thêm nữa để tránh lặp file
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
                            if (_training_progress < SAMPLES_PER_CMD) {
                                memcpy(_training_vectors[_training_progress], current_vector, sizeof(float) * VECTOR_DIM);
                                _training_progress++;
                            }
                            _is_speaking = false;
                            _silence_start_ms = 0;
                            _is_processing = false;
                            Serial.printf("[VOICE] Sample %d captured (%d bytes)\n", _training_progress, s_audio_len * 2);

                            // Dump audio for Python script (Little Endian %02X%02X)
                            if (_dump_training_enabled) {
                                Serial.printf("\n---AUDIO_START:APP_TRAIN_%d---\n", _training_progress);
                                for(int i = 0; i < s_audio_len; i++) {
                                    uint16_t v = (uint16_t)s_audio_buffer[i];
                                    Serial.printf("%02X%02X", v & 0xFF, v >> 8);
                                    if ((i + 1) % 64 == 0) Serial.println();
                                }
                                Serial.println("\n---AUDIO_END---");
                            }

                        } else {
                            // Wake word or Command logic here
                            float max_sim = -1.0f;
                            int best_match = -1;

                            // Quyết định chế độ nhận diện dựa trên trạng thái màn hình
                            bool is_sleeping = ui_manager_is_sleeping();

                            if (is_sleeping) {
                                // 1. Khi đang ngủ: CHỈ tìm Wake Word
                                if (_wake_loaded) {
                                    float sim = vector_cosine_similarity(current_vector, _wake_vector, VECTOR_DIM);
                                    if (sim >= SIMILARITY_THRESHOLD) {
                                        max_sim = sim;
                                        best_match = VOICE_CMD_WAKE;
                                    }
                                }
                            } else {
                                // 2. Khi đã thức: CHỈ tìm các Lệnh điều khiển (GPIO)
                                for (int i = 0; i < VOICE_CMD_COUNT; i++) {
                                    if (_on_loaded[i]) {
                                        float sim = vector_cosine_similarity(current_vector, _on_vectors[i], VECTOR_DIM);
                                        if (sim > max_sim) {
                                            max_sim = sim;
                                            best_match = VOICE_CMD_ON_BASE + i;
                                        }
                                    }
                                    if (_off_loaded[i]) {
                                        float sim = vector_cosine_similarity(current_vector, _off_vectors[i], VECTOR_DIM);
                                        if (sim > max_sim) {
                                            max_sim = sim;
                                            best_match = VOICE_CMD_OFF_BASE + i;
                                        }
                                    }
                                }
                            }

                            if (max_sim >= SIMILARITY_THRESHOLD) {
                                Serial.printf("[VOICE] Match found: %s (Similarity: %.2f)\n", voice_engine_cmd_name(best_match), max_sim);
                                if (_cmd_cb) {
                                    _cmd_cb(best_match, voice_engine_cmd_name(best_match));
                                }

                                // Dump recognition audio if enabled (Little Endian %02X%02X)
                                if (_dump_recognition_enabled) {
                                    Serial.printf("\n---AUDIO_START:APP_RECOG_%d_SIM_%.2f---\n", best_match, max_sim);
                                    for (int i = 0; i < s_audio_len; i++) {
                                        uint16_t v = (uint16_t)s_audio_buffer[i];
                                        Serial.printf("%02X%02X", v & 0xFF, v >> 8);
                                        if ((i + 1) % 64 == 0) Serial.println();
                                    }
                                    Serial.println("\n---AUDIO_END---");
                                }
                            } else {
                                Serial.printf("[VOICE] No match found. Best similarity: %.2f\n", max_sim);
                            }

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
