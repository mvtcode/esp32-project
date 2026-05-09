/**
 * i2s_mic.cpp — INMP441 microphone driver (I2S)
 * IoT Voice Command System
 *
 * INMP441 xuất dữ liệu 32-bit frames, dữ liệu 24-bit nằm ở MSB.
 * Driver đọc 32-bit, shift phải 14 bit để lấy int16_t phù hợp với ESP-SR.
 */
#include "i2s_mic.h"
#include "../hardware_config.h"
#include <Arduino.h>
#include <driver/i2s.h>

// ─── Private constants ────────────────────────────────────────────────────────
static constexpr i2s_port_t I2S_PORT = I2S_NUM_0;

// ─── Lifecycle ───────────────────────────────────────────────────────────────

bool i2s_mic_init() {
    const i2s_config_t cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate          = I2S_SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = I2S_DMA_BUF_COUNT,
        .dma_buf_len          = I2S_DMA_BUF_LEN,
        .use_apll             = false,
        .tx_desc_auto_clear   = false,
        .fixed_mclk           = 0
    };

    const i2s_pin_config_t pins = {
        .bck_io_num   = MIC_SCK,
        .ws_io_num    = MIC_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num  = MIC_SD
    };

    esp_err_t err = i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[MIC] i2s_driver_install failed: %d\n", err);
        return false;
    }

    err = i2s_set_pin(I2S_PORT, &pins);
    if (err != ESP_OK) {
        Serial.printf("[MIC] i2s_set_pin failed: %d\n", err);
        return false;
    }

    i2s_zero_dma_buffer(I2S_PORT);
    Serial.println("[MIC] INMP441 initialized OK");
    return true;
}

void i2s_mic_deinit() {
    i2s_driver_uninstall(I2S_PORT);
}

// ─── Audio read ──────────────────────────────────────────────────────────────

int i2s_mic_read(int16_t* buf, int num_samples) {
    if (!buf || num_samples <= 0) return 0;

    // Cấp phát tạm thời trên stack cho 32-bit samples
    // Giới hạn kích thước để tránh stack overflow
    const int MAX_BATCH = 512;
    int total_read = 0;

    while (total_read < num_samples) {
        int batch = num_samples - total_read;
        if (batch > MAX_BATCH) batch = MAX_BATCH;

        int32_t raw[MAX_BATCH];
        size_t bytes_read = 0;

        esp_err_t err = i2s_read(
            I2S_PORT,
            raw,
            batch * sizeof(int32_t),
            &bytes_read,
            portMAX_DELAY
        );

        if (err != ESP_OK || bytes_read == 0) break;

        int samples_read = (int)(bytes_read / sizeof(int32_t));

        // Chuyển đổi 32-bit → 16-bit:
        // INMP441: data 24-bit nằm ở [31:8], shift right 14 để vừa int16_t
        for (int i = 0; i < samples_read; i++) {
            buf[total_read + i] = (int16_t)(raw[i] >> 14);
        }

        total_read += samples_read;
    }

    return total_read;
}
