#include "i2s_mic.h"
#include "../hardware_config.h"
#include <Arduino.h>
#include <driver/i2s.h>

static constexpr i2s_port_t I2S_PORT = I2S_NUM_0;

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
        .use_apll             = false
    };

    const i2s_pin_config_t pins = {
        .bck_io_num   = MIC_SCK,
        .ws_io_num    = MIC_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num  = MIC_SD
    };

    if (i2s_driver_install(I2S_PORT, &cfg, 0, NULL) != ESP_OK) return false;
    if (i2s_set_pin(I2S_PORT, &pins) != ESP_OK) return false;

    i2s_zero_dma_buffer(I2S_PORT);
    return true;
}

void i2s_mic_deinit() {
    i2s_driver_uninstall(I2S_PORT);
}

int i2s_mic_read(int16_t* buf, int num_samples) {
    if (!buf || num_samples <= 0) return 0;

    size_t bytes_read = 0;
    static int32_t raw_data[1024]; 
    static float dc_offset = 0; // BẮT BUỘC PHẢI CÓ để khử nhiễu nền
    
    int to_read = (num_samples > 1024) ? 1024 : num_samples;
    esp_err_t err = i2s_read(I2S_PORT, raw_data, to_read * sizeof(int32_t), &bytes_read, pdMS_TO_TICKS(100));

    if (err == ESP_OK && bytes_read > 0) {
        int count = bytes_read / 4;
        for (int i = 0; i < count; i++) {
            // Dùng >> 16 chuẩn xác như bản Test bạn đã ưng ý
            int16_t s16 = (int16_t)(raw_data[i] >> 16);
            
            // Khử DC Offset: Làm mịn hơn để tránh tiếng dè (humming)
            dc_offset = 0.9995f * dc_offset + 0.0005f * s16;
            int32_t val = (int32_t)s16 - (int32_t)dc_offset;

            // Khuếch đại nhẹ x8 để âm thanh to rõ hơn mà không gây méo
            val = val * 8;

            // Giới hạn để chống tràn số 16-bit
            if (val > 32767) val = 32767;
            if (val < -32768) val = -32768;

            buf[i] = (int16_t)val;
        }
        return count;
    }
    return 0;
}
