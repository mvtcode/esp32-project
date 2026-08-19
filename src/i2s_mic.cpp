#include "i2s_mic.h"
#include "driver/i2s.h"     // Legacy I2S API — available in Arduino-ESP32 v2.x & v3.x

static const i2s_port_t PORT = (i2s_port_t)I2S_MIC_PORT;

static volatile bool s_i2s_initialized = false;

// Audio processing settings
static float   s_gain       = MIC_DEFAULT_GAIN;
static int32_t s_noise_gate = MIC_DEFAULT_NOISE_GATE;

// DC-blocker filter states (1st order IIR high-pass filter @ ~80Hz)
static float s_prev_in_l  = 0.0f;
static float s_prev_out_l = 0.0f;
static float s_prev_in_r  = 0.0f;
static float s_prev_out_r = 0.0f;

void i2s_mic_set_gain(float gain) {
    if (gain < 0.1f) gain = 0.1f;
    if (gain > 32.0f) gain = 32.0f;
    s_gain = gain;
}

float i2s_mic_get_gain() {
    return s_gain;
}

void i2s_mic_set_noise_gate(int32_t threshold) {
    if (threshold < 0) threshold = 0;
    s_noise_gate = threshold;
}

int32_t i2s_mic_get_noise_gate() {
    return s_noise_gate;
}

// -----------------------------------------------------------------------
// i2s_mic_init
// -----------------------------------------------------------------------
bool i2s_mic_init() {
    if (s_i2s_initialized) return true;

    // --- Driver configuration ---
    i2s_config_t cfg = {};
    cfg.mode                = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
    cfg.sample_rate         = SAMPLE_RATE;
    cfg.bits_per_sample     = I2S_BITS_PER_SAMPLE_32BIT;
    cfg.channel_format      = I2S_CHANNEL_FMT_RIGHT_LEFT; // Stereo: [L, R] interleaved
    cfg.communication_format= I2S_COMM_FORMAT_STAND_I2S;  // Standard Philips format
    cfg.intr_alloc_flags    = ESP_INTR_FLAG_LEVEL1;
#if defined(ESP_IDF_VERSION_MAJOR) && ESP_IDF_VERSION_MAJOR >= 5
    cfg.dma_desc_num        = 4;          // Number of DMA buffers in ring
    cfg.dma_frame_num       = FRAME_SIZE; // Samples per DMA buffer (frames per buffer)
#else
    cfg.dma_buf_count       = 4;
    cfg.dma_buf_len         = FRAME_SIZE;
#endif
    cfg.use_apll            = false;
    cfg.tx_desc_auto_clear  = false;
    cfg.fixed_mclk          = 0;

    esp_err_t err = i2s_driver_install(PORT, &cfg, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[I2S] driver_install failed: %s\n", esp_err_to_name(err));
        return false;
    }

    // --- Pin mapping ---
    i2s_pin_config_t pins = {};
    pins.mck_io_num   = I2S_PIN_NO_CHANGE;  // MCLK not needed by INMP441
    pins.bck_io_num   = I2S_PIN_SCK;
    pins.ws_io_num    = I2S_PIN_WS;
    pins.data_out_num = I2S_PIN_NO_CHANGE;  // RX only
    pins.data_in_num  = I2S_PIN_SD;

    err = i2s_set_pin(PORT, &pins);
    if (err != ESP_OK) {
        Serial.printf("[I2S] set_pin failed: %s\n", esp_err_to_name(err));
        i2s_driver_uninstall(PORT);
        s_i2s_initialized = false;
        return false;
    }

    s_i2s_initialized = true;
    Serial.printf("[I2S] OK — %d Hz, 16-bit stereo | SCK=GPIO%d  WS=GPIO%d  SD=GPIO%d | Gain=%.1fx Gate=%d\n",
                  SAMPLE_RATE, I2S_PIN_SCK, I2S_PIN_WS, I2S_PIN_SD, s_gain, s_noise_gate);
    return true;
}

// -----------------------------------------------------------------------
// i2s_mic_deinit
// -----------------------------------------------------------------------
bool i2s_mic_deinit() {
    if (!s_i2s_initialized) return true;
    s_i2s_initialized = false;
    esp_err_t err = i2s_driver_uninstall(PORT);
    if (err == ESP_OK) {
        Serial.println("[I2S] Mic driver uninstalled");
        return true;
    }
    return false;
}

// -----------------------------------------------------------------------
// i2s_mic_read
// -----------------------------------------------------------------------
bool i2s_mic_read(int32_t *left, int32_t *right, size_t n) {
    if (!s_i2s_initialized) return false;

    // Interleaved stereo raw buffer: [L0, R0, L1, R1, ... ]
    // Size = n pairs × 2 channels × 4 bytes
    static int32_t raw[FRAME_SIZE * 2];

    size_t bytes_wanted = n * 2 * sizeof(int32_t);
    size_t bytes_read   = 0;

    esp_err_t err = i2s_read(PORT, raw, bytes_wanted, &bytes_read, pdMS_TO_TICKS(100));
    if (err != ESP_OK || bytes_read == 0) return false;

    size_t pairs = bytes_read / (2 * sizeof(int32_t));
    int32_t peak_l = 0;
    int32_t peak_r = 0;
    int64_t sum_abs_l = 0;
    int64_t sum_abs_r = 0;

    const float R = 0.995f; // DC-blocker coefficient (cut-off ~12 Hz @ 16kHz)

    for (size_t i = 0; i < pairs && i < n; i++) {
        // 1. Take 16-bit MSB from 32-bit slot for Left and Right channels
        float in_l = (float)((int16_t)(raw[i * 2]     >> 16));
        float in_r = (float)((int16_t)(raw[i * 2 + 1] >> 16));

        // 2. Textbook IIR DC-blocking high-pass filter: y[n] = x[n] - x[n-1] + R * y[n-1]
        float out_l = in_l - s_prev_in_l + R * s_prev_out_l;
        s_prev_in_l  = in_l;
        s_prev_out_l = out_l;

        float out_r = in_r - s_prev_in_r + R * s_prev_out_r;
        s_prev_in_r  = in_r;
        s_prev_out_r = out_r;

        int32_t val_l = (int32_t)out_l;
        int32_t val_r = (int32_t)out_r;

        // Apply gain (default 1.0f)
        if (s_gain != 1.0f) {
            val_l = (int32_t)((float)val_l * s_gain);
            val_r = (int32_t)((float)val_r * s_gain);
        }

        // 3. 16-bit bounds clamping
        if (val_l > 32767)  val_l = 32767;
        if (val_l < -32768) val_l = -32768;
        if (val_r > 32767)  val_r = 32767;
        if (val_r < -32768) val_r = -32768;

        left[i]  = val_l;
        right[i] = val_r;

        int32_t abs_l = val_l < 0 ? -val_l : val_l;
        int32_t abs_r = val_r < 0 ? -val_r : val_r;

        if (abs_l > peak_l) peak_l = abs_l;
        if (abs_r > peak_r) peak_r = abs_r;
    }

    // 4. Symmetrical Noise Gate for Left and Right channels
    // Mutes to 0 only when in silence (peak below threshold), opens instantly with full sensitivity for any real sound
    if (peak_l < s_noise_gate) {
        for (size_t i = 0; i < n; i++) left[i] = 0;
    }
    if (peak_r < s_noise_gate) {
        for (size_t i = 0; i < n; i++) right[i] = 0;
    }

    return true;
}
