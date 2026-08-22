#include "mp3_player.h"
#include "nvs_storage.h"
#include "display.h"
#include "log.h"
#include "bt_audio.h" // for DAC_PIN_BCK, DAC_PIN_LCK, DAC_PIN_DIN

#include <AudioFileSourceSD.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutput.h>
#include <driver/i2s.h>
#include <esp_task_wdt.h>
#include <math.h>

#define PREBUFFER_SIZE (4 * 1024)
#define FADE_RAMP_SAMPLES 1320 // ~30ms at 44.1kHz

// RTC Memory survives software reboot/panic to prevent infinite bootloop on corrupt track
RTC_DATA_ATTR static int s_rtc_crash_count = 0;
RTC_DATA_ATTR static int s_rtc_last_crashed_idx = -1;

enum AudioCmd {
    CMD_PLAY_TRACK,
    CMD_NEXT_TRACK,
    CMD_PREV_TRACK,
    CMD_PAUSE,
    CMD_RESUME,
    CMD_STOP
};

struct AudioMsg {
    AudioCmd cmd;
    int track_idx;
};

static QueueHandle_t s_audio_cmd_queue = nullptr;
static QueueHandle_t s_vis_queue = nullptr;
static TaskHandle_t  s_mp3_task_handle = nullptr;

class AudioFileSourceSafeSD;
static AudioFileSourceSafeSD *s_file_sd = nullptr;
static AudioFileSourceBuffer *s_file_buff = nullptr;
static AudioGeneratorMP3     *s_mp3_gen = nullptr;
static AudioGeneratorWAV     *s_wav_gen = nullptr;

// ---------------------------------------------------------------------------
// Precalculated Volume Lookup Table (powf curve)
// ---------------------------------------------------------------------------
static float s_vol_lut[128];
static bool  s_vol_lut_inited = false;

static void init_vol_lut_if_needed() {
    if (s_vol_lut_inited) return;
    for (int i = 0; i <= 127; i++) {
        float f = (float)i / 127.0f;
        s_vol_lut[i] = powf(f, 1.3f);
    }
    s_vol_lut_inited = true;
}

// ---------------------------------------------------------------------------
// Custom Watchdog-Safe AudioFileSource with Corrupt Loop Detection
// ---------------------------------------------------------------------------
class AudioFileSourceSafeSD : public AudioFileSource {
public:
    AudioFileSourceSafeSD() : m_bytes_since_sample(0), m_cur_pos(0) {}
    AudioFileSourceSafeSD(const char *filename) : m_bytes_since_sample(0), m_cur_pos(0) {
        open(filename);
    }
    virtual ~AudioFileSourceSafeSD() override {
        close();
    }

    virtual bool open(const char *filename) override {
        close();
        m_file = SD.open(filename, "r");
        m_bytes_since_sample = 0;
        m_cur_pos = 0;
        return (bool)m_file;
    }

    virtual uint32_t read(void *data, uint32_t len) override {
        if (!m_file || !m_file.available()) return 0;

        esp_task_wdt_reset();

        // Safety: Yield 1ms to FreeRTOS so IDLE0 task never triggers Watchdog
        vTaskDelay(pdMS_TO_TICKS(1));

        // Safety: If reader reads > 64KB without producing any audio samples,
        // it is stuck in a corrupt sync search loop. Return 0 to abort cleanly!
        if (m_bytes_since_sample > 65536) {
            LOG_W("MP3", "Corrupt frame loop detected (>64KB without audio) -> Aborting stream");
            return 0;
        }

        size_t r = m_file.read((uint8_t*)data, len);
        m_bytes_since_sample += (uint32_t)r;
        m_cur_pos += (uint32_t)r;
        return (uint32_t)r;
    }

    virtual bool seek(int32_t pos, int dir) override {
        if (!m_file) return false;
        SeekMode mode = SeekSet;
        if (dir == SEEK_CUR) mode = SeekCur;
        else if (dir == SEEK_END) mode = SeekEnd;
        bool ok = m_file.seek(pos, mode);
        if (ok) {
            m_cur_pos = m_file.position();
        }
        return ok;
    }

    virtual bool close() override {
        if (m_file) {
            m_file.close();
        }
        m_cur_pos = 0;
        return true;
    }

    virtual bool isOpen() override {
        return (bool)m_file;
    }

    virtual uint32_t getSize() override {
        if (!m_file) return 0;
        return m_file.size();
    }

    virtual uint32_t getPos() override {
        return m_cur_pos;
    }

    void reset_sample_bytes() {
        m_bytes_since_sample = 0;
    }

private:
    File m_file;
    uint32_t m_bytes_since_sample;
    uint32_t m_cur_pos;
};

static volatile int       s_current_track_idx   = -1;
static volatile uint8_t   s_current_volume      = 80;
static volatile bool      s_is_started          = false;
static volatile bool      s_is_playing          = false;
static volatile bool      s_is_paused           = false;
static volatile uint32_t  s_last_decode_ts      = 0;
static volatile int       s_consecutive_errors  = 0;

static volatile uint32_t  s_current_sample_rate = 44100;
static volatile uint32_t  s_samples_played      = 0;
static volatile uint32_t  s_file_size_bytes     = 0;
static volatile uint32_t  s_file_pos_bytes      = 0;
static volatile uint32_t  s_track_total_duration_sec = 0;

// Soft fade state for anti-pop
enum FadeState { FADE_NORMAL, FADE_IN, FADE_OUT };
static volatile FadeState s_fade_state = FADE_NORMAL;
static volatile int       s_fade_count = 0;

// ---------------------------------------------------------------------------
// Custom AudioOutput with I2S PCM5102A & Visualizer Queue Feed
// ---------------------------------------------------------------------------
class AudioOutputVisualizerDAC : public AudioOutput {
public:
    AudioOutputVisualizerDAC() {
        m_channels = 2;
        m_bps = 16;
        m_rate = 44100;
        m_fa_count = 0;
        init_vol_lut_if_needed();
    }

    virtual ~AudioOutputVisualizerDAC() {}

    virtual bool begin() override {
        return true;
    }

    virtual bool SetRate(int hz) override {
        if (hz <= 0) hz = 44100;
        m_rate = hz;
        s_current_sample_rate = hz;
        i2s_set_sample_rates(I2S_NUM_0, hz);
        LOG_I("MP3", "Sample rate set to %d Hz", hz);
        return true;
    }

    virtual bool SetBitsPerSample(int bits) override {
        m_bps = bits;
        return true;
    }

    virtual bool SetChannels(int channels) override {
        m_channels = channels;
        return true;
    }

    virtual bool stop() override {
        i2s_zero_dma_buffer(I2S_NUM_0);
        return true;
    }

    virtual bool ConsumeSample(int16_t sample[2]) override {
        if (!s_is_playing || s_is_paused) return false;

        // Break decoder loop immediately if a control command (Next/Prev/Pause/Play) is queued
        if (s_audio_cmd_queue && uxQueueMessagesWaiting(s_audio_cmd_queue) > 0) {
            return false;
        }

        int16_t l = sample[0];
        int16_t r = (m_channels == 1) ? sample[0] : sample[1];

        // 1. Fast Perceptual Digital Volume via LUT (Zero CPU powf overhead)
        float vol_factor = s_vol_lut[s_current_volume & 0x7F];

        // 2. Soft Fade-In / Fade-Out Ramp (Anti-pop)
        if (s_fade_state == FADE_IN) {
            float fade_multiplier = (float)s_fade_count / (float)FADE_RAMP_SAMPLES;
            vol_factor *= fade_multiplier;
            s_fade_count++;
            if (s_fade_count >= FADE_RAMP_SAMPLES) {
                s_fade_state = FADE_NORMAL;
            }
        } else if (s_fade_state == FADE_OUT) {
            float fade_multiplier = 1.0f - ((float)s_fade_count / (float)FADE_RAMP_SAMPLES);
            vol_factor *= fade_multiplier;
            s_fade_count++;
            if (s_fade_count >= FADE_RAMP_SAMPLES) {
                vol_factor = 0.0f;
            }
        }

        // Apply volume scaling
        int16_t out_samples[2];
        out_samples[0] = (int16_t)constrain((int32_t)((float)l * vol_factor), -32768, 32767);
        out_samples[1] = (int16_t)constrain((int32_t)((float)r * vol_factor), -32768, 32767);

        // 3. Write to I2S DAC (PCM5102A on I2S_NUM_0)
        size_t written = 0;
        i2s_write(I2S_NUM_0, out_samples, sizeof(out_samples), &written, pdMS_TO_TICKS(2));

        // 4. Feed Visualizer Queue (128 samples per channel)
        if (s_vis_queue) {
            m_fa_left[m_fa_count]  = (int32_t)l;
            m_fa_right[m_fa_count] = (int32_t)r;
            m_fa_count++;

            if (m_fa_count >= FRAME_SIZE) {
                AudioFrame frame;
                memcpy(frame.left, m_fa_left, sizeof(m_fa_left));
                memcpy(frame.right, m_fa_right, sizeof(m_fa_right));
                xQueueSend(s_vis_queue, &frame, 0);
                m_fa_count = 0;
            }
        }

        s_samples_played++;
        s_last_decode_ts = millis();
        if (s_file_sd) {
            s_file_sd->reset_sample_bytes();
        }
        if ((s_samples_played & 0x1FF) == 0) {
            esp_task_wdt_reset();
            if (s_file_sd) {
                s_file_pos_bytes = s_file_sd->getPos();
            }
        }
        return (written > 0);
    }

private:
    int m_channels;
    int m_bps;
    int m_rate;
    int32_t m_fa_left[FRAME_SIZE];
    int32_t m_fa_right[FRAME_SIZE];
    size_t m_fa_count;
};

static AudioOutputVisualizerDAC *s_audio_out = nullptr;

// ---------------------------------------------------------------------------
// Internal Track Loader & Closer (Executes on Core 0 audio task only)
// ---------------------------------------------------------------------------
static void close_current_stream() {
    s_is_playing = false;
    s_is_paused = false;

    if (s_mp3_gen) {
        if (s_mp3_gen->isRunning()) {
            s_mp3_gen->stop();
        }
        delete s_mp3_gen;
        s_mp3_gen = nullptr;
    }
    if (s_wav_gen) {
        if (s_wav_gen->isRunning()) {
            s_wav_gen->stop();
        }
        delete s_wav_gen;
        s_wav_gen = nullptr;
    }
    if (s_file_buff) {
        s_file_buff->close();
        delete s_file_buff;
        s_file_buff = nullptr;
    }
    if (s_file_sd) {
        s_file_sd->close();
        delete s_file_sd;
        s_file_sd = nullptr;
    }

    i2s_zero_dma_buffer(I2S_NUM_0);
    s_samples_played = 0;
    s_file_size_bytes = 0;
    s_file_pos_bytes = 0;
}

static uint32_t compute_track_duration(AudioFileSource *src, uint32_t file_size, bool is_wav) {
    if (file_size == 0 || !src) return 0;
    if (is_wav) {
        // Standard WAV 44.1kHz 16-bit 2ch = 176,400 bytes/sec
        return (uint32_t)(file_size / 176400);
    }

    // Try detecting MP3 bitrate from first frame header
    uint8_t buf[1024];
    uint32_t orig_pos = src->getPos();
    src->seek(0, SeekSet);
    uint32_t bytes_read = src->read(buf, sizeof(buf));
    src->seek(orig_pos, SeekSet);

    static const int br_table[] = { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0 };

    if (bytes_read > 4) {
        for (uint32_t i = 0; i < bytes_read - 4; i++) {
            if (buf[i] == 0xFF && (buf[i+1] & 0xE0) == 0xE0) {
                uint8_t ver   = (buf[i+1] >> 3) & 0x03; // 3 = MPEG1
                uint8_t layer = (buf[i+1] >> 1) & 0x03; // 1 = Layer 3
                uint8_t br_idx = (buf[i+2] >> 4) & 0x0F;

                if (ver == 3 && layer == 1 && br_idx > 0 && br_idx < 15) {
                    int kbps = br_table[br_idx];
                    if (kbps > 0) {
                        uint32_t sec = (uint32_t)(((uint64_t)file_size * 8) / (kbps * 1000));
                        LOG_I("MP3", "Detected MP3 Bitrate: %d kbps -> Total Duration: %u sec (%02u:%02u)",
                              kbps, sec, sec / 60, sec % 60);
                        return sec;
                    }
                }
            }
        }
    }

    // Fallback: estimate at 192 kbps (24,000 bytes/sec)
    uint32_t fallback_sec = (uint32_t)(file_size / 24000);
    LOG_I("MP3", "Fallback MP3 Duration estimate: %u sec (%02u:%02u)",
          fallback_sec, fallback_sec / 60, fallback_sec % 60);
    return fallback_sec;
}

static bool open_track(int index) {
    LOG_I("MP3", "open_track: target index=%d", index);
    close_current_stream();
    vTaskDelay(pdMS_TO_TICKS(5));

    int track_count = sd_card_get_track_count();
    if (track_count <= 0) return false;

    if (index < 0) index = 0;
    if (index >= track_count) index = track_count - 1;

    // Check available heap before decoding
    if (ESP.getFreeHeap() < 24000) {
        LOG_E("MP3", "Heap too low (%u bytes), skipping track", (unsigned)ESP.getFreeHeap());
        return false;
    }

    const PlaylistItem *track = sd_card_get_track(index);
    if (!track) return false;

    LOG_I("MP3", "Opening Track [%d/%d]: %s ('%s')", index + 1, track_count, track->path, track->title);

    // Record last attempted track index in RTC RAM for crash recovery across panic/reboots
    s_rtc_last_crashed_idx = index;

    s_file_sd = new AudioFileSourceSafeSD(track->path);
    if (!s_file_sd || !s_file_sd->isOpen()) {
        LOG_E("MP3", "Failed to open file on SD: %s", track->path);
        if (s_file_sd) { delete s_file_sd; s_file_sd = nullptr; }
        return false;
    }

    s_file_size_bytes = s_file_sd->getSize();

    const char *dot = strrchr(track->path, '.');
    bool is_wav = (dot && strcasecmp(dot, ".wav") == 0);

    // 1. Compute track duration ONCE at start BEFORE buffering
    s_track_total_duration_sec = compute_track_duration(s_file_sd, s_file_size_bytes, is_wav);
    s_file_sd->seek(0, SeekSet);
    s_file_sd->reset_sample_bytes();

    // 2. Allocate fresh pre-buffer from beginning of file
    s_file_buff = new AudioFileSourceBuffer(s_file_sd, PREBUFFER_SIZE);
    if (!s_file_buff) {
        LOG_E("MP3", "Failed to allocate audio pre-buffer");
        s_file_sd->close();
        delete s_file_sd;
        s_file_sd = nullptr;
        return false;
    }

    bool ok = false;
    if (is_wav) {
        s_wav_gen = new AudioGeneratorWAV();
        if (s_wav_gen) {
            ok = s_wav_gen->begin(s_file_buff, s_audio_out);
        }
    } else {
        s_mp3_gen = new AudioGeneratorMP3();
        if (s_mp3_gen) {
            ok = s_mp3_gen->begin(s_file_buff, s_audio_out);
        }
    }

    if (!ok) {
        LOG_E("MP3", "Decoder begin() failed for: %s (FreeHeap=%u, MaxBlock=%u)",
              track->path, (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());
        close_current_stream();
        return false;
    }

    s_current_track_idx = index;
    s_is_playing = true;
    s_is_paused = false;
    s_fade_state = FADE_IN;
    s_fade_count = 0;
    s_samples_played = 0;
    s_file_pos_bytes = 0;
    s_last_decode_ts = millis();

    // Mark crash index in RTC
    s_rtc_crash_count++;
    s_rtc_last_crashed_idx = index;

    // Save track index to NVS only when track actually changes
    nvs_save_sd_track_index((uint16_t)index);

    LOG_I("MP3", "Playback started successfully");
    return true;
}

// ---------------------------------------------------------------------------
// FreeRTOS MP3 Audio Decoding Task — Core 0
// ---------------------------------------------------------------------------
static void mp3_player_task(void * /*arg*/) {
    esp_task_wdt_add(NULL);

    for (;;) {
        esp_task_wdt_reset();

        // 1. Process all queued audio commands from UI task
        AudioMsg msg;
        while (s_audio_cmd_queue && xQueueReceive(s_audio_cmd_queue, &msg, 0) == pdTRUE) {
            LOG_I("MP3", "Task handling command: %d (arg=%d)", (int)msg.cmd, msg.track_idx);
            int tc = sd_card_get_track_count();

            if (msg.cmd == CMD_PLAY_TRACK) {
                open_track(msg.track_idx);
            } else if (msg.cmd == CMD_NEXT_TRACK && tc > 0) {
                int cur = (s_current_track_idx >= 0) ? s_current_track_idx : 0;
                int target_idx = (cur + 1) % tc;
                for (int a = 0; a < tc; a++) {
                    LOG_I("MP3", "Switching NEXT: target=%d/%d", target_idx + 1, tc);
                    if (open_track(target_idx)) break;
                    LOG_W("MP3", "Track %d failed, skipping to next...", target_idx + 1);
                    display_toast("FILE ERROR -> SKIP");
                    target_idx = (target_idx + 1) % tc;
                }
            } else if (msg.cmd == CMD_PREV_TRACK && tc > 0) {
                int cur = (s_current_track_idx >= 0) ? s_current_track_idx : 0;
                int target_idx = (cur - 1 + tc) % tc;
                for (int a = 0; a < tc; a++) {
                    LOG_I("MP3", "Switching PREV: target=%d/%d", target_idx + 1, tc);
                    if (open_track(target_idx)) break;
                    LOG_W("MP3", "Track %d failed, skipping to prev...", target_idx + 1);
                    display_toast("FILE ERROR -> SKIP");
                    target_idx = (target_idx - 1 + tc) % tc;
                }
            } else if (msg.cmd == CMD_PAUSE) {
                s_fade_state = FADE_OUT;
                s_fade_count = 0;
                vTaskDelay(pdMS_TO_TICKS(35));
                s_is_paused = true;
                i2s_zero_dma_buffer(I2S_NUM_0);
                if (s_current_track_idx >= 0) {
                    nvs_save_sd_track_index((uint16_t)s_current_track_idx);
                }
                LOG_I("MP3", "Playback PAUSED");
            } else if (msg.cmd == CMD_RESUME) {
                s_fade_state = FADE_IN;
                s_fade_count = 0;
                s_is_paused = false;
                LOG_I("MP3", "Playback RESUMED");
            } else if (msg.cmd == CMD_STOP) {
                close_current_stream();
            }
        }

        // 2. Decode audio loop
        if (s_is_playing && !s_is_paused) {
            bool running = false;
            if (s_mp3_gen && s_mp3_gen->isRunning()) {
                running = s_mp3_gen->loop();
                if (!running) s_mp3_gen->stop();
            } else if (s_wav_gen && s_wav_gen->isRunning()) {
                running = s_wav_gen->loop();
                if (!running) s_wav_gen->stop();
            }

            if (s_samples_played > 88200 && s_rtc_crash_count > 0) {
                s_rtc_crash_count = 0;
                s_consecutive_errors = 0;
            }

            // End of File (natural track finish) -> Auto Next Track
            if (!running && s_is_playing) {
                LOG_I("MP3", "Track finished. Auto advancing to next track...");
                int tc = sd_card_get_track_count();
                if (tc > 0) {
                    int next_idx = (s_current_track_idx + 1) % tc;
                    open_track(next_idx);
                } else {
                    close_current_stream();
                }
            }

            // Corrupt file or decode timeout check (1.5 seconds without decoded samples)
            if (millis() - s_last_decode_ts > 1500 && s_is_playing) {
                LOG_W("MP3", "Decode stalled for 1.5s -> Auto skipping corrupted track");
                display_toast("FILE ERROR!");
                s_consecutive_errors++;
                int tc = sd_card_get_track_count();
                if (tc > 1 && s_consecutive_errors < tc) {
                    open_track((s_current_track_idx + 1) % tc);
                } else {
                    display_toast("NO PLAYABLE FILES");
                    close_current_stream();
                    s_consecutive_errors = 0;
                }
                vTaskDelay(pdMS_TO_TICKS(300));
            }

            vTaskDelay(pdMS_TO_TICKS(1));
        } else {
            vTaskDelay(pdMS_TO_TICKS(15));
        }
    }
}

// ---------------------------------------------------------------------------
// Public API Implementation
// ---------------------------------------------------------------------------
void mp3_player_init(QueueHandle_t audio_queue) {
    s_vis_queue = audio_queue;
    if (!s_audio_cmd_queue) {
        s_audio_cmd_queue = xQueueCreate(10, sizeof(AudioMsg));
    }
    s_current_volume = nvs_load_volume();
    s_current_track_idx = (int)nvs_load_sd_track_index();
    LOG_I("MP3", "MP3 Player Subsystem Initialized (Default track: %d, Volume: %d)",
          s_current_track_idx, s_current_volume);
}

void mp3_player_start() {
    if (s_is_started) return;
    s_current_volume = nvs_load_volume();
    LOG_I("MP3", "Starting MP3 Player engine (Volume: %d)...", s_current_volume);

    if (!s_audio_cmd_queue) {
        s_audio_cmd_queue = xQueueCreate(4, sizeof(AudioMsg));
    }

    // 1. Install & Configure I2S Driver for DAC PCM5102A on I2S_NUM_0 (optimized DMA memory)
    static const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S | I2S_COMM_FORMAT_STAND_MSB),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 3,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true
    };

    static const i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = DAC_PIN_BCK,   // GPIO 18
        .ws_io_num = DAC_PIN_LCK,    // GPIO 19
        .data_out_num = DAC_PIN_DIN, // GPIO 23
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        LOG_E("MP3", "Failed to install I2S driver: %d", err);
        return;
    }
    i2s_set_pin(I2S_NUM_0, &pin_config);

    if (!s_audio_out) {
        s_audio_out = new AudioOutputVisualizerDAC();
    }

    // 2. Create MP3 decoding task on Core 0 (Optimized Stack 4KB, Priority 2)
    if (!s_mp3_task_handle) {
        xTaskCreatePinnedToCore(
            mp3_player_task, "mp3_player_task", 4096, nullptr, 2, &s_mp3_task_handle, 0
        );
    }

    s_is_started = true;
    s_is_paused = false;

    // 3. Play remembered track (with auto crash recovery)
    int tc = sd_card_get_track_count();
    if (tc > 0) {
        if (s_rtc_crash_count > 0 && s_rtc_last_crashed_idx >= 0 && tc > 1) {
            LOG_W("MP3", "Previous crash detected on track %d (crash count %d) -> Auto skipping corrupted track!",
                  s_rtc_last_crashed_idx + 1, s_rtc_crash_count);
            display_toast("FILE ERROR -> SKIPPED");
            s_current_track_idx = (s_rtc_last_crashed_idx + 1) % tc;
            nvs_save_sd_track_index((uint16_t)s_current_track_idx);
            s_rtc_crash_count = 0;
        }

        if (s_current_track_idx < 0 || s_current_track_idx >= tc) {
            s_current_track_idx = 0;
        }
        mp3_player_play_track(s_current_track_idx);
    }
}

void mp3_player_stop() {
    if (!s_is_started) return;
    LOG_I("MP3", "Stopping MP3 Player engine with graceful stop and verified cleanup...");

    // 1. Soft fade out volume to prevent pop/click
    s_fade_state = FADE_OUT;
    s_fade_count = 0;
    vTaskDelay(pdMS_TO_TICKS(40));

    // 2. Send CMD_STOP to audio task and await clean stream closure
    if (s_audio_cmd_queue) {
        AudioMsg msg = { .cmd = CMD_STOP, .track_idx = 0 };
        xQueueSend(s_audio_cmd_queue, &msg, pdMS_TO_TICKS(100));

        // Poll until stream is closed (s_is_playing becomes false) or timeout 400ms
        uint32_t wait_start = millis();
        while (s_is_playing && (millis() - wait_start < 400)) {
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

    // 3. We no longer delete the task to avoid heap fragmentation and memory leaks.
    // The task will remain in an idle state waiting for new commands.
    // 4. Cleanly close and free audio decoders, file buffers, and file handles
    close_current_stream();

    // 5. Delete audio output driver
    if (s_audio_out) {
        delete s_audio_out;
        s_audio_out = nullptr;
    }

    // 6. Zero DMA buffer and uninstall I2S driver
    i2s_zero_dma_buffer(I2S_NUM_0);
    vTaskDelay(pdMS_TO_TICKS(50));
    i2s_driver_uninstall(I2S_NUM_0);

    s_is_started = false;
    s_is_playing = false;
    s_is_paused = false;
    LOG_I("MP3", "MP3 Player fully STOPPED & VERIFIED (FreeHeap=%u, MaxBlock=%u)",
          (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());
}

void mp3_player_play_track(int index) {
    LOG_I("MP3", "Play track requested: index=%d", index);
    if (s_audio_cmd_queue) {
        AudioMsg msg = { .cmd = CMD_PLAY_TRACK, .track_idx = index };
        xQueueSend(s_audio_cmd_queue, &msg, portMAX_DELAY);
    }
}

void mp3_player_next_track() {
    LOG_I("MP3", "Next track requested");
    if (s_audio_cmd_queue) {
        AudioMsg msg = { .cmd = CMD_NEXT_TRACK, .track_idx = 0 };
        xQueueSend(s_audio_cmd_queue, &msg, portMAX_DELAY);
    }
}

void mp3_player_prev_track() {
    LOG_I("MP3", "Prev track requested");
    if (s_audio_cmd_queue) {
        AudioMsg msg = { .cmd = CMD_PREV_TRACK, .track_idx = 0 };
        xQueueSend(s_audio_cmd_queue, &msg, portMAX_DELAY);
    }
}

void mp3_player_toggle_play_pause() {
    if (s_is_paused) {
        mp3_player_resume();
    } else {
        mp3_player_pause();
    }
}

void mp3_player_pause() {
    if (!s_is_playing || s_is_paused) return;
    if (s_audio_cmd_queue) {
        AudioMsg msg = { .cmd = CMD_PAUSE, .track_idx = 0 };
        xQueueSend(s_audio_cmd_queue, &msg, portMAX_DELAY);
    }
}

void mp3_player_resume() {
    if (!s_is_playing || !s_is_paused) return;
    if (s_audio_cmd_queue) {
        AudioMsg msg = { .cmd = CMD_RESUME, .track_idx = 0 };
        xQueueSend(s_audio_cmd_queue, &msg, portMAX_DELAY);
    }
}

void mp3_player_adjust_volume(int32_t delta) {
    int32_t new_vol = (int32_t)s_current_volume + delta;
    if (new_vol < 0) new_vol = 0;
    if (new_vol > 127) new_vol = 127;
    mp3_player_set_volume((uint8_t)new_vol);
}

void mp3_player_set_volume(uint8_t volume) {
    if (volume > 127) volume = 127;
    s_current_volume = volume;
    nvs_save_volume(s_current_volume);
    LOG_D("MP3", "Volume set to %d (%.0f%%)", s_current_volume, (float)s_current_volume * 100.0f / 127.0f);
}

uint8_t mp3_player_get_volume() {
    return s_current_volume;
}

bool mp3_player_is_playing() {
    return s_is_playing && !s_is_paused;
}

bool mp3_player_is_paused() {
    return s_is_paused;
}

bool mp3_player_is_active() {
    return s_is_started && (s_current_track_idx >= 0);
}

int mp3_player_get_current_track_index() {
    return s_current_track_idx;
}

const PlaylistItem* mp3_player_get_current_track() {
    return sd_card_get_track(s_current_track_idx);
}

uint32_t mp3_player_get_current_pos_sec() {
    if (s_current_sample_rate == 0) return 0;
    return s_samples_played / s_current_sample_rate;
}

uint32_t mp3_player_get_total_duration_sec() {
    return s_track_total_duration_sec;
}

uint8_t mp3_player_get_progress_percent() {
    if (s_track_total_duration_sec == 0) return 0;
    uint32_t cur = mp3_player_get_current_pos_sec();
    uint32_t pct = ((uint64_t)cur * 100) / s_track_total_duration_sec;
    if (pct > 100) pct = 100;
    return (uint8_t)pct;
}
