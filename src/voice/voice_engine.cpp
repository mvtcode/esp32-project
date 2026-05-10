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
#include <esp_dsp.h>

// ─── Constants ───────────────────────────────────────────────────────────────
#define SIMILARITY_THRESHOLD  0.75f  // Lower is easier to match
#define VAD_THRESHOLD         1000   // Increased to match higher digital gain
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

static volatile bool  _is_processing = false;

// ─── Helpers ─────────────────────────────────────────────────────────────────

#define NUM_FRAMES 16
#define FFT_SIZE 256
#define BINS_PER_FRAME 16

static void extract_embedding(const int16_t* audio, int len, float* vector_out) {
    if (len < FFT_SIZE) {
        memset(vector_out, 0, VECTOR_DIM * sizeof(float));
        return;
    }

    float* fft_work = (float*)malloc(FFT_SIZE * 2 * sizeof(float));
    if (!fft_work) {
        memset(vector_out, 0, VECTOR_DIM * sizeof(float));
        return;
    }

    int step = (len - FFT_SIZE) / NUM_FRAMES;
    if (step < 1) step = 1;

    for (int i = 0; i < NUM_FRAMES; i++) {
        int start_idx = i * step;
        if (start_idx + FFT_SIZE > len) start_idx = len - FFT_SIZE;

        // --- Frame-level Noise Gate ---
        int32_t frame_sum = 0;
        for (int j = 0; j < FFT_SIZE; j++) {
            frame_sum += abs(audio[start_idx + j]);
        }
        int32_t frame_rms = frame_sum / FFT_SIZE;

        // If this specific frame is just background noise, zero it out completely.
        // This prevents short table knocks (which are mostly silence) from matching long voice commands.
        if (frame_rms < (VAD_THRESHOLD / 2)) { // Slightly lower than global VAD to preserve word endings
            for (int b = 0; b < BINS_PER_FRAME; b++) {
                vector_out[i * BINS_PER_FRAME + b] = 0.0f;
            }
            continue;
        }

        for (int j = 0; j < FFT_SIZE; j++) {
            fft_work[j * 2 + 0] = (float)audio[start_idx + j]; // Real
            fft_work[j * 2 + 1] = 0.0f;                        // Imag
        }

        dsps_fft2r_fc32(fft_work, FFT_SIZE);
        dsps_bit_rev_fc32(fft_work, FFT_SIZE);

        for (int b = 0; b < BINS_PER_FRAME; b++) {
            float band_energy = 0;
            for (int k = 0; k < 8; k++) {
                int bin_idx = b * 8 + k;
                float re = fft_work[bin_idx * 2 + 0];
                float im = fft_work[bin_idx * 2 + 1];
                band_energy += (re * re + im * im);
            }
            vector_out[i * BINS_PER_FRAME + b] = log10f(band_energy + 1.0f);
        }
    }

    free(fft_work);

    // Normalize the entire vector to unit length for the fast dot product
    float norm = vector_magnitude(vector_out, VECTOR_DIM);
    if (norm > 0) {
        for(int i=0; i<VECTOR_DIM; i++) vector_out[i] /= norm;
    }
}

// ─── Public API ───────────────────────────────────────────────────────────────

bool voice_engine_init() {
    esp_err_t ret = dsps_fft2r_init_fc32(NULL, 4096);
    if (ret != ESP_OK) {
        Serial.printf("[VOICE] DSP FFT init failed: %d\n", ret);
    }

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

        // Pre-normalize the averaged vector for ultra-fast dot product later
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
        _silence_start_ms = 1; // Force timeout next loop
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
    const int FRAME_SAMPLES = 480; // 30ms
    int16_t audio_buf[FRAME_SAMPLES];
    static float current_vector[VECTOR_DIM];

    #define MAX_AUDIO_SAMPLES (16000 * 2) // 2 seconds max
    int16_t* s_audio_buffer = (int16_t*)heap_caps_malloc(MAX_AUDIO_SAMPLES * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    if (!s_audio_buffer) s_audio_buffer = (int16_t*)malloc(MAX_AUDIO_SAMPLES * sizeof(int16_t));
    int s_audio_len = 0;

    // --- Pre-record buffer (Lookback) to catch leading consonants ---
    const int PRE_CHUNKS = 5; // 5 * 30ms = 150ms pre-roll
    int16_t pre_record[PRE_CHUNKS][FRAME_SAMPLES];
    int pre_idx = 0;

    while (true) {
        int n = i2s_mic_read(audio_buf, FRAME_SAMPLES);
        if (n <= 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        // Save current frame to ring buffer
        memcpy(pre_record[pre_idx], audio_buf, FRAME_SAMPLES * sizeof(int16_t));
        pre_idx = (pre_idx + 1) % PRE_CHUNKS;

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
            s_audio_len = 0;
        } else {
            if (rms > VAD_THRESHOLD) {
                if (!_is_speaking) {
                    if (!_is_training && (millis() - _last_recog_ms < 1500)) {
                        // Cooldown
                    } else {
                        _is_speaking = true;
                        _silence_start_ms = 0;
                        s_audio_len = 0;
                        
                        // Copy the past 150ms from ring buffer to capture soft leading consonants!
                        for (int i = 0; i < PRE_CHUNKS - 1; i++) {
                            int idx = (pre_idx + i) % PRE_CHUNKS;
                            memcpy(&s_audio_buffer[s_audio_len], pre_record[idx], FRAME_SAMPLES * sizeof(int16_t));
                            s_audio_len += FRAME_SAMPLES;
                        }

                        if (_is_training) Serial.println("[VOICE] Training: Speaking...");
                        else Serial.println("[VOICE] Recognition: Listening...");
                    }
                } else {
                    _silence_start_ms = 0; 
                }
            } 
            
            if (_is_speaking) {
                // Accumulate audio samples
                if (s_audio_len + n <= MAX_AUDIO_SAMPLES) {
                    memcpy(&s_audio_buffer[s_audio_len], audio_buf, n * sizeof(int16_t));
                    s_audio_len += n;
                }

                if (rms <= VAD_THRESHOLD) {
                    if (_silence_start_ms == 0) {
                        _silence_start_ms = millis();
                    } else if (millis() - _silence_start_ms > VAD_SILENCE_MS || _silence_start_ms == 1) {
                        _is_processing = true; 
                        
                        // 1. Trimming trailing silence
                        int speech_end = s_audio_len;
                        const int noise_floor = VAD_THRESHOLD / 2;
                        for (int i = s_audio_len - 1; i >= 0; i--) {
                            if (abs(s_audio_buffer[i]) > noise_floor) {
                                speech_end = i + 1;
                                break;
                            }
                        }

                        // 2. Trimming leading silence
                        int speech_start = 0;
                        for (int i = 0; i < speech_end; i++) {
                            if (abs(s_audio_buffer[i]) > noise_floor) {
                                speech_start = i;
                                break;
                            }
                        }
                        
                        int final_len = speech_end - speech_start;

                        if (final_len > 0) {
                            Serial.printf("[VOICE] Speech ended. Length: %d ms. Processing...\n", (final_len * 1000) / 16000);

                            extract_embedding(&s_audio_buffer[speech_start], final_len, current_vector);
                            
                            char audio_label[64] = "unknown";
                            
                            if (_is_training) {
                                snprintf(audio_label, sizeof(audio_label), "train_gpio%d_%s", _training_gpio, _training_on_cmd ? "ON" : "OFF");
                                if (_training_progress < SAMPLES_PER_CMD) {
                                    memcpy(_training_vectors[_training_progress], current_vector, sizeof(current_vector));
                                    _training_progress++;
                                    Serial.printf("[VOICE] Captured sample %d/%d\n", _training_progress, SAMPLES_PER_CMD);
                                }
                            } else {
                                bool is_sleeping = ui_manager_is_sleeping();
                                float best_score = 0;
                                int best_cmd = -1;

                                if (is_sleeping) {
                                    // ─── SLEEP: Wake Word Only ───
                                    if (_wake_loaded) {
                                        float score = vector_cosine_similarity(current_vector, _wake_vector, VECTOR_DIM);
                                        Serial.printf("[VOICE] Sleep Mode - Wake Word Score: %.2f\n", score);
                                        if (score > SIMILARITY_THRESHOLD) {
                                            best_score = score;
                                            best_cmd = VOICE_CMD_WAKE;
                                        }
                                    }
                                } else {
                                    // ─── AWAKE: All GPIO Commands ───
                                    for (int i = 0; i < VOICE_CMD_COUNT; i++) {
                                        if (_on_loaded[i]) {
                                            float s = vector_cosine_similarity(current_vector, _on_vectors[i], VECTOR_DIM);
                                            if (s > best_score) { best_score = s; best_cmd = i; }
                                        }
                                        if (_off_loaded[i]) {
                                            float s = vector_cosine_similarity(current_vector, _off_vectors[i], VECTOR_DIM);
                                            if (s > best_score) { best_score = s; best_cmd = i + 10; }
                                        }
                                    }
                                }

                                if (best_cmd != -1 && best_score > SIMILARITY_THRESHOLD) {
                                    _last_recog_ms = millis();
                                    if (best_cmd == VOICE_CMD_WAKE) {
                                        snprintf(audio_label, sizeof(audio_label), "match_WAKE");
                                        Serial.println("[VOICE] MATCH: WAKE WORD!");
                                        if (_cmd_cb) _cmd_cb(VOICE_CMD_WAKE, "WAKE WORD");
                                    } else {
                                        snprintf(audio_label, sizeof(audio_label), "match_ID%d", best_cmd);
                                        Serial.printf("[VOICE] MATCH: ID %d (Score: %.2f)\n", best_cmd, best_score);
                                        if (_cmd_cb) _cmd_cb(best_cmd, voice_engine_cmd_name(best_cmd));
                                    }
                                } else {
                                    snprintf(audio_label, sizeof(audio_label), "unrecognized");
                                    Serial.printf("[VOICE] No match (Best: %.2f)\n", best_score);
                                    if (_cmd_cb) _cmd_cb(-1, "NO MATCH");
                                }
                            }

                            // --- DUMP AUDIO QUA SERIAL VỚI LABEL ---
                            bool dump_audio = true; 
                            if (dump_audio) {
                                Serial.printf("---AUDIO_START:%s---\n", audio_label);
                                int16_t* ptr = &s_audio_buffer[speech_start];
                                for (int i = 0; i < final_len; i += 32) {
                                    char hex_buf[130];
                                    int idx = 0;
                                    for (int j = 0; j < 32 && (i + j) < final_len; j++) {
                                        sprintf(&hex_buf[idx], "%04X", (uint16_t)ptr[i + j]);
                                        idx += 4;
                                    }
                                    hex_buf[idx] = '\0';
                                    Serial.println(hex_buf);
                                    vTaskDelay(pdMS_TO_TICKS(5)); // Prevent watchdog
                                }
                                Serial.println("---AUDIO_END---");
                            }
                            // ---------------------------------------
                        }
                        _is_processing = false;
                        _is_speaking = false;
                        _silence_start_ms = 0;
                        s_audio_len = 0;
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
