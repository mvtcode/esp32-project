#include <Arduino.h>
#include <driver/i2s.h>
#include "hardware_config.h"

// Cấu hình trực tiếp tại đây, không dùng file i2s_mic.cpp nữa
#define I2S_PORT I2S_NUM_0

bool is_recording = false;
int16_t raw_buffer[512];

void init_i2s_raw() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S, // Philips
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 128,
        .use_apll = false
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = MIC_SCK,
        .ws_io_num = MIC_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = MIC_SD
    };

    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
    i2s_zero_dma_buffer(I2S_PORT);
}

void setup() {
    Serial.begin(921600);
    pinMode(BTN_ENTER, INPUT_PULLUP);
    delay(2000);
    
    Serial.println("\n======================================");
    Serial.println("[SUPER-RAW] MIC TEST (NO EXTERNAL FILES)");
    Serial.println("======================================");

    init_i2s_raw();
    Serial.println("[TEST] I2S Hardware Initialized.");
    Serial.println("[TEST] Press Enter or BTN_ENTER to start.");
}

void loop() {
    bool toggle = false;
    if (Serial.available()) {
        while(Serial.available()) Serial.read();
        toggle = true;
    }
    if (digitalRead(BTN_ENTER) == LOW) {
        delay(50);
        while(digitalRead(BTN_ENTER) == LOW);
        toggle = true;
    }

    if (toggle) {
        is_recording = !is_recording;
        if (is_recording) Serial.println("\n---AUDIO_START:SUPER_RAW---");
        else Serial.println("\n---AUDIO_END---");
    }

    if (is_recording) {
        int32_t samples[128];
        size_t bytes_read = 0;
        static float dc_offset = 0;
        static int16_t peak = 0;
        
        esp_err_t err = i2s_read(I2S_PORT, samples, sizeof(samples), &bytes_read, portMAX_DELAY);
        
        if (err == ESP_OK && bytes_read > 0) {
            int count = bytes_read / 4;
            for (int i = 0; i < count; i++) {
                // Lấy 16 bit MSB
                int16_t s16 = (int16_t)(samples[i] >> 16);

                // Lọc DC nhẹ nhàng (đưa sóng về tâm 0)
                dc_offset = 0.999 * dc_offset + 0.001 * s16;
                int32_t val = s16 - (int32_t)dc_offset;

                // Giới hạn để không bị tràn số
                if (val > 32767) val = 32767;
                if (val < -32768) val = -32768;

                if (abs(val) > peak) peak = abs(val);

                Serial.printf("%04X", (uint16_t)((int16_t)val));
                
                static int line_count = 0;
                if (++line_count >= 64) {
                    Serial.println();
                    line_count = 0;
                }
            }
        }
    } else {
        delay(10);
    }
}
