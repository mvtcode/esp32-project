/**
 * i2s_mic.h — INMP441 microphone driver qua I2S
 * IoT Voice Command System
 *
 * Khởi tạo I2S port và đọc audio samples 16-bit/16kHz
 * dùng cho ESP-SR voice recognition.
 */
#ifndef I2S_MIC_H
#define I2S_MIC_H

#include <stdint.h>
#include <stdbool.h>

// ─── Lifecycle ───────────────────────────────────────────────────────────────
bool i2s_mic_init();
void i2s_mic_deinit();

// ─── Audio read ──────────────────────────────────────────────────────────────
/**
 * Đọc num_samples mẫu 16-bit từ microphone vào buf.
 * INMP441 output 32-bit frames → driver tự co lại thành 16-bit.
 * @return số sample thực sự đọc được (0 nếu lỗi/timeout)
 */
int i2s_mic_read(int16_t* buf, int num_samples);

#endif // I2S_MIC_H
