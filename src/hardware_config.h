/**
 * hardware_config.h — Central pin & peripheral configuration
 * IoT Voice Command System — ESP32-S3-N16R8
 *
 * Tất cả pin numbers và thông số phần cứng tập trung ở đây.
 * Các module khác KHÔNG được hardcode pin/constants.
 */
#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

// ─── OLED SH1106 1.3" (I2C Software) ────────────────────────────────────────
#define OLED_SDA        8
#define OLED_SCL        9
#define OLED_I2C_ADDR   0x3C   // SH1106 default address
#define OLED_WIDTH      128
#define OLED_HEIGHT     64

// ─── Microphone INMP441 (I2S) ────────────────────────────────────────────────
#define MIC_WS          42     // Word Select (LRCLK)
#define MIC_SD          2      // Serial Data (DOUT)
#define MIC_SCK         41     // Serial Clock (BCLK)

#define I2S_SAMPLE_RATE     16000  // Hz — required by ESP-SR
#define I2S_BITS            32     // INMP441 outputs 32-bit frames
#define I2S_DMA_BUF_COUNT   8
#define I2S_DMA_BUF_LEN    256     // Tăng lên để tránh mất dữ liệu gây rè

// ─── Buttons ─────────────────────────────────────────────────────────────────
#define BTN_UP          10
#define BTN_DOWN        11
#define BTN_ENTER       12
#define BTN_BACK        13
#define BTN_WAKE         0     // Boot button — manual wake trigger

// ─── Status LED ──────────────────────────────────────────────────────────────
#define LED_STATUS      48     // Built-in RGB LED on ESP32-S3 devkits

// ─── Relay / Output GPIOs ────────────────────────────────────────────────────
#define NUM_RELAYS       6
#define RELAY_1          1
#define RELAY_2          3
#define RELAY_3          4
#define RELAY_4          5
#define RELAY_5          6
#define RELAY_6          7

// ─── FreeRTOS Task Config ────────────────────────────────────────────────────
#define VOICE_TASK_CORE     1      // Core 1: voice/audio processing
#define VOICE_TASK_STACK    32768  // Tăng lên 32KB cho an toàn tuyệt đối
#define VOICE_TASK_PRIO     5

#define UI_TASK_CORE        0      // Core 0: UI updates
#define UI_TASK_STACK       8192
#define UI_TASK_PRIO        3

#endif // HARDWARE_CONFIG_H
