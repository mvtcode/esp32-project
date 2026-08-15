#include "i2s_mic.h"
#include "driver/i2s.h"     // Legacy I2S API — available in Arduino-ESP32 v2.x & v3.x

static const i2s_port_t PORT = (i2s_port_t)I2S_MIC_PORT;

// -----------------------------------------------------------------------
// i2s_mic_init
// -----------------------------------------------------------------------
bool i2s_mic_init() {
    // --- Driver configuration ---
    i2s_config_t cfg = {};
    cfg.mode                = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
    cfg.sample_rate         = SAMPLE_RATE;
    cfg.bits_per_sample     = I2S_BITS_PER_SAMPLE_32BIT;
    cfg.channel_format      = I2S_CHANNEL_FMT_RIGHT_LEFT; // Stereo: [L, R] interleaved
    cfg.communication_format= I2S_COMM_FORMAT_STAND_I2S;  // Standard Philips format
    cfg.intr_alloc_flags    = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_desc_num        = 4;          // Number of DMA buffers in ring
    cfg.dma_frame_num       = FRAME_SIZE; // Samples per DMA buffer (frames per buffer)
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
        return false;
    }

    Serial.printf("[I2S] OK — %d Hz, 32-bit stereo | SCK=GPIO%d  WS=GPIO%d  SD=GPIO%d\n",
                  SAMPLE_RATE, I2S_PIN_SCK, I2S_PIN_WS, I2S_PIN_SD);
    return true;
}

// -----------------------------------------------------------------------
// i2s_mic_read
// -----------------------------------------------------------------------
bool i2s_mic_read(int32_t *left, int32_t *right, size_t n) {
    // Interleaved stereo raw buffer: [L0, R0, L1, R1, ... ]
    // Size = n pairs × 2 channels × 4 bytes
    static int32_t raw[FRAME_SIZE * 2];

    size_t bytes_wanted = n * 2 * sizeof(int32_t);
    size_t bytes_read   = 0;

    esp_err_t err = i2s_read(PORT, raw, bytes_wanted, &bytes_read, portMAX_DELAY);
    if (err != ESP_OK) return false;

    size_t pairs = bytes_read / (2 * sizeof(int32_t));
    for (size_t i = 0; i < pairs && i < n; i++) {
        // INMP441 outputs 24-bit audio MSB-justified in a 32-bit slot.
        // Shift right 8 to extract the meaningful 24-bit signed value.
        //
        // Channel order with I2S_CHANNEL_FMT_RIGHT_LEFT:
        //   raw[i*2 + 0] = LEFT  channel (WS=LOW,  Mic-L L/R=GND)
        //   raw[i*2 + 1] = RIGHT channel (WS=HIGH, Mic-R L/R=3V3)
        //
        // If the channels appear swapped, swap the indices below.
        left[i]  = raw[i * 2]     >> 8;
        right[i] = raw[i * 2 + 1] >> 8;
    }

    return true;
}
