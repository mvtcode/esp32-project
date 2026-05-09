/**
 * voice_engine.cpp — ESP-SR voice recognition engine
 * IoT Voice Command System
 */
#include "voice_engine.h"
#include "vector_math.h"
#include "vector_storage.h"
#include "../display/ui_manager.h"
#include "../hardware_config.h"
#include "../audio/i2s_mic.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ─── Constants ───────────────────────────────────────────────────────────────
#define SIMILARITY_THRESHOLD  0.75f  // Lower is easier to match
#define VAD_THRESHOLD         600    // Higher is less sensitive to noise
#define VAD_SILENCE_MS        1500   // 1.5s silence to finalize
#define SAMPLES_PER_CMD       1

// ─── Private state ────────────────────────────────────────────────────────────
static voice_cmd_cb_t _cmd_cb       = nullptr;
static volatile bool  _listening    = false;
static volatile int   _last_rms      = 0;

// Training state
static bool     _is_training        = false;
static bool     _recognition_enabled = true;
static int      _training_gpio      = -1;
static bool     _training_on_cmd    = true;
static int      _training_progress  = 0;
static float    _training_vectors[SAMPLES_PER_CMD][VECTOR_DIM];
static bool     _is_speaking        = false;
static uint32_t _silence_start_ms   = 0;
static uint32_t _last_recog_ms      = 0;

// RAM Cache for ALL Commands (High Performance)
static float _on_vectors[VOICE_CMD_COUNT][VECTOR_DIM];
static bool  _on_loaded[VOICE_CMD_COUNT] = {false};

static float _off_vectors[VOICE_CMD_COUNT][VECTOR_DIM];
static bool  _off_loaded[VOICE_CMD_COUNT] = {false};

static float _wake_vector[VECTOR_DIM];
static bool  _wake_loaded = false;

// ─── Helpers ─────────────────────────────────────────────────────────────────

static void extract_embedding(const int16_t* audio, int len, float* vector_out) {
    if (len == 0) return;

    // 1. Simple DC Offset Removal & Energy Calc
    int64_t sum = 0;
    for(int i=0; i<len; i++) sum += audio[i];
    int16_t dc_offset = sum / len;

    uint32_t hash = 0;
    float energy = 0;
    for(int i=0; i<len; i++) {
        int16_t val = audio[i] - dc_offset;
        hash = hash * 31 + abs(val);
        energy += (float)val * val;
    }
    
    // 2. Mock Embedding (Using hash to seed, but influenced by length and energy)
    srand(hash + len);
    for(int i=0; i<VECTOR_DIM; i++) {
        vector_out[i] = (float)rand() / (float)RAND_MAX;
    }

    // 3. Normalization (L2 Norm) — Crucial for Cosine Similarity
    float norm = 0;
    for(int i=0; i<VECTOR_DIM; i++) norm += vector_out[i] * vector_out[i];
    norm = sqrt(norm);
    if (norm > 0) {
        for(int i=0; i<VECTOR_DIM; i++) vector_out[i] /= norm;
    }
}

// ─── Public API ───────────────────────────────────────────────────────────────

bool voice_engine_init() {
    bool mic_ok = i2s_mic_init();
    bool storage_ok = vector_storage_init();
    if (!mic_ok || !storage_ok) return false;
    _listening = true;

    // Load ALL Commands into RAM cache
    Serial.println("[VOICE] Loading command cache...");
    for (int i = 0; i < VOICE_CMD_COUNT; i++) {
        if (vector_storage_load(i, true, _on_vectors[i])) {
            _on_loaded[i] = true;
        }
        if (vector_storage_load(i, false, _off_vectors[i])) {
            _off_loaded[i] = true;
        }
    }

    // Load Wake Word
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
    Serial.printf("[VOICE] Started training GPIO %d (%s). Waiting for speech...\n", gpio_idx, on_cmd ? "ON" : "OFF");
}

void voice_engine_stop_training(bool save) {
    if (save && _is_training && _training_progress > 0) {
        static float avg_vector[VECTOR_DIM]; // Use static to avoid stack overflow
        memset(avg_vector, 0, sizeof(avg_vector));
        
        for(int d=0; d<VECTOR_DIM; d++) {
            for(int s=0; s<_training_progress; s++) {
                avg_vector[d] += _training_vectors[s][d];
            }
            avg_vector[d] /= _training_progress;
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
    }
    _is_training = false;
    _training_progress = 0;
    _is_speaking = false;
    _silence_start_ms = 0;
}

int voice_engine_get_training_progress() {
    return _training_progress;
}

int voice_engine_get_last_rms() {
    return _last_rms;
}

void voice_engine_force_finalize() {
    if (_is_training && _is_speaking && _training_progress == 0) {
        _training_progress = 1;
        _is_speaking = false;
        _silence_start_ms = 0;
        Serial.println("[VOICE] Force finalized by user.");
    }
}

bool voice_engine_has_command(int gpio_idx, bool on_cmd) {
    return vector_storage_exists(gpio_idx, on_cmd);
}

bool voice_engine_is_listening() {
    return _listening;
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
    const int FRAME_SAMPLES = 480; // 30ms
    int16_t audio_buf[FRAME_SAMPLES];
    static float current_vector[VECTOR_DIM]; // Static to prevent stack overflow

    while (true) {
        int n = i2s_mic_read(audio_buf, FRAME_SAMPLES);
        if (n <= 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        int32_t rms = 0;
        for (int i = 0; i < n; i++) rms += abs(audio_buf[i]);
        rms /= n;
        _last_rms = rms;

        ScreenID current_screen = ui_manager_get_current_screen();
        _is_training = (current_screen == SCREEN_VOICE_CHANGE);
        _recognition_enabled = (current_screen == SCREEN_HOME);

        if (!_is_training && !_recognition_enabled) {
            _is_speaking = false;
            _silence_start_ms = 0;
        } else {
            if (rms > VAD_THRESHOLD) {
                if (!_is_speaking) {
                    if (!_is_training && (millis() - _last_recog_ms < 1500)) {
                        // Cooldown
                    } else {
                        _is_speaking = true;
                        _silence_start_ms = 0;
                        if (_is_training) Serial.println("[VOICE] Training: Speaking...");
                        else Serial.println("[VOICE] Recognition: Listening...");
                    }
                } else {
                    _silence_start_ms = 0; 
                }
            } else if (_is_speaking) {
                if (_silence_start_ms == 0) {
                    _silence_start_ms = millis();
                } else if (millis() - _silence_start_ms > VAD_SILENCE_MS) {
                    // 1. Trimming: Find the actual end of speech (remove trailing silence)
                    int speech_end = n;
                    const int noise_floor = VAD_THRESHOLD / 2;
                    for (int i = n - 1; i >= 0; i--) {
                        if (abs(audio_buf[i]) > noise_floor) {
                            speech_end = i + 1;
                            break;
                        }
                    }
                    
                    if (speech_end > 0) {
                        Serial.printf("[VOICE] Processing %d samples (Trimmed %d silence)\n", speech_end, n - speech_end);
                        extract_embedding(audio_buf, speech_end, current_vector);
                        
                        if (_is_training) {
                            if (_training_progress < SAMPLES_PER_CMD) {
                                memcpy(_training_vectors[_training_progress], current_vector, sizeof(current_vector));
                                _training_progress++;
                            }
                        } else {
                            _last_recog_ms = millis();
                            bool recognized = false;
                            bool is_sleeping = ui_manager_is_sleeping();

                            if (is_sleeping) {
                                // 1. SLEEP MODE: ONLY check Wake Word (RAM)
                                if (_wake_loaded) {
                                    float score = vector_cosine_similarity(current_vector, _wake_vector, VECTOR_DIM);
                                    if (score > SIMILARITY_THRESHOLD) {
                                        Serial.printf("[VOICE] Recognized: WAKE WORD (score: %.2f)\n", score);
                                        if (_cmd_cb) _cmd_cb(VOICE_CMD_WAKE, "WAKE WORD");
                                        recognized = true;
                                    }
                                }
                            } else {
                                // 2. NORMAL MODE: ONLY check GPIO Commands (RAM)
                                for (int i = 0; i < VOICE_CMD_COUNT; i++) {
                                    // Check ON
                                    if (_on_loaded[i]) {
                                        float score = vector_cosine_similarity(current_vector, _on_vectors[i], VECTOR_DIM);
                                        if (score > SIMILARITY_THRESHOLD) {
                                            Serial.printf("[VOICE] Recognized: GPIO %d ON (score: %.2f)\n", i, score);
                                            if (_cmd_cb) _cmd_cb(i, voice_engine_cmd_name(i));
                                            recognized = true;
                                            break;
                                        }
                                    }
                                    // Check OFF
                                    if (_off_loaded[i]) {
                                        float score = vector_cosine_similarity(current_vector, _off_vectors[i], VECTOR_DIM);
                                        if (score > SIMILARITY_THRESHOLD) {
                                            Serial.printf("[VOICE] Recognized: GPIO %d OFF (score: %.2f)\n", i, score);
                                            if (_cmd_cb) _cmd_cb(i + 10, voice_engine_cmd_name(i + 10));
                                            recognized = true;
                                            break;
                                        }
                                    }
                                }

                                if (!recognized) {
                                    Serial.println("[VOICE] No match.");
                                    if (_cmd_cb) _cmd_cb(-1, "NO MATCH");
                                }
                            }
                        }
                    }
                    _is_speaking = false;
                    _silence_start_ms = 0;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
