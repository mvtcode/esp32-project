#include "voice_manager.h"
#include "hardware_config.h"
#include <Arduino.h>
#include <driver/i2s.h>

// I2S Configuration
#define I2S_PORT I2S_NUM_0

static voice_command_cb_t _command_cb = NULL;

void voice_init() {
    // 1. Initialize I2S for Microphone
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1,
        .data_in_num = I2S_SD
    };

    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
    i2s_zero_dma_buffer(I2S_PORT);

    Serial.println("I2S Microphone Initialized");

    // 2. Placeholder for ESP-SR initialization
    // In a real scenario, this would load models from the 'model' partition
    // and start the WakeNet/MultiNet tasks.
}

void voice_update() {
    // This would be handled by internal ESP-SR tasks, 
    // but we can poll for results or handle events here.
}

void voice_set_command_callback(voice_command_cb_t cb) {
    _command_cb = cb;
}
