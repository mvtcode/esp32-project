#pragma once
#include <Arduino.h>

// -----------------------------------------------------------------------
// I2S Pin assignments (confirmed, no conflict with OLED I2C on GPIO 8/9)
// -----------------------------------------------------------------------
#define I2S_MIC_PORT    0       // I2S_NUM_0
#define I2S_PIN_SCK     4       // Bit Clock   (BCLK)
#define I2S_PIN_WS      5       // Word Select (LRCLK)
#define I2S_PIN_SD      6       // Serial Data – shared by both INMP441 mics

// -----------------------------------------------------------------------
// Audio parameters
// -----------------------------------------------------------------------
#define SAMPLE_RATE     16000   // Hz — INMP441 works well at 16 kHz
#define FRAME_SIZE      128         // Samples per channel per frame (~8 ms @ 16kHz)
                                // = 1 sample per pixel column → matches OLED width exactly

/**
 * @brief Initialize the I2S driver for stereo INMP441 microphones.
 *
 * Configuration:
 *   - Mode:   MASTER RX
 *   - Format: Standard I2S (Philips), stereo, 32-bit/sample
 *   - Mics:   Both INMP441 share SCK, WS, SD lines.
 *             Mic-L (L/R=GND) outputs on WS=LOW slot (left channel).
 *             Mic-R (L/R=3V3) outputs on WS=HIGH slot (right channel).
 *
 * INMP441 outputs 24-bit MSB-justified audio in a 32-bit I2S word.
 * i2s_mic_read() extracts the 24-bit value by shifting right 8 bits.
 *
 * @return true on success, false if ESP-IDF driver returns an error.
 */
bool i2s_mic_init();

/**
 * @brief Read one stereo frame from I2S. Blocks until data is ready.
 *
 * @param left   Output buffer for left  channel (must hold >= n int32_t)
 * @param right  Output buffer for right channel (must hold >= n int32_t)
 * @param n      Number of samples to read per channel (use FRAME_SIZE)
 * @return true on success
 *
 * @note If left/right appear swapped, toggle the L/R pin on one mic,
 *       or swap the index mapping in i2s_mic.cpp.
 */
bool i2s_mic_read(int32_t *left, int32_t *right, size_t n);
