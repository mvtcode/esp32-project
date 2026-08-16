#include <Arduino.h>
#include "i2s_mic.h"
#include "display.h"
#include "button.h"

/**
 * ESP32-S3 Super Mini — Dual-channel Sound Visualizer
 *
 * Hardware:
 *   OLED SH1106 1.3"  : SDA=GPIO8, SCL=GPIO9  (Hardware I2C, 800 kHz)
 *   INMP441 Left  mic : SCK=GPIO4, WS=GPIO5, SD=GPIO6, L/R=GND
 *   INMP441 Right mic : SCK=GPIO4, WS=GPIO5, SD=GPIO6, L/R=3V3
 *   Mode button       : GPIO0 (BOOT button, active LOW, no extra wiring)
 *
 * Architecture (FreeRTOS dual-core):
 *   Core 0 — mic_task : I2S read → AudioFrame → queue
 *   Core 1 — loop()   : button check → queue → display_draw_waveform()
 */

// -----------------------------------------------------------------------
// AudioFrame: one stereo frame passed between cores via queue
// -----------------------------------------------------------------------
struct AudioFrame {
    int32_t left[FRAME_SIZE];
    int32_t right[FRAME_SIZE];
};

static QueueHandle_t s_audio_queue;

// -----------------------------------------------------------------------
// FreeRTOS task — Core 0
// -----------------------------------------------------------------------
static void mic_task(void * /*arg*/) {
    static AudioFrame frame;
    for (;;) {
        if (i2s_mic_read(frame.left, frame.right, FRAME_SIZE)) {
            xQueueSend(s_audio_queue, &frame, /*timeout=*/0);
        }
    }
}

// -----------------------------------------------------------------------
// setup() — Core 1
// -----------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("=== ESP32-S3 Sound Visualizer ===");
    Serial.printf("  Frame: %d samples @ %d Hz = %.1f ms\n",
                  FRAME_SIZE, SAMPLE_RATE, 1000.0f * FRAME_SIZE / SAMPLE_RATE);
    Serial.println("  BOOT button (GPIO0) = next mode");

    // 1. Display (shows splash while I2S initialises)
    display_init();

    // 2. Mode button — GPIO 0 (BOOT, built-in, no extra wiring)
    button_init(BTN_GPIO);

    // 3. I2S microphones
    if (!i2s_mic_init()) {
        Serial.println("[FATAL] I2S mic init failed. Halting.");
        display_error("I2S INIT FAIL");
        while (true) { delay(1000); }
    }

    // 4. Audio queue
    s_audio_queue = xQueueCreate(2, sizeof(AudioFrame));
    if (!s_audio_queue) {
        Serial.println("[FATAL] Queue create failed (OOM).");
        display_error("QUEUE FAIL");
        while (true) { delay(1000); }
    }

    // 5. Mic task on Core 0
    xTaskCreatePinnedToCore(
        mic_task, "mic_task", 4096, nullptr, 2, nullptr, 0
    );

    Serial.println("[boot] Ready — press BOOT to cycle modes");
}

// -----------------------------------------------------------------------
// loop() — Core 1
// -----------------------------------------------------------------------
void loop() {
    static AudioFrame frame;
    static uint32_t   fps_ts  = 0;
    static uint32_t   fps_cnt = 0;

    // --- Button handling ---
    button_update();
    if (button_pressed()) {
        display_next_mode();
    }
    if (button_long_pressed()) {
        bool auto_cycle = !display_get_auto_cycle();
        display_set_auto_cycle(auto_cycle);
        Serial.printf("[Mode] Auto-cycle %s\n", auto_cycle ? "ON" : "OFF");
    }

    // --- Render: dequeue and draw ---
    if (xQueueReceive(s_audio_queue, &frame, pdMS_TO_TICKS(5)) == pdTRUE) {
        display_draw_waveform(frame.left, frame.right, FRAME_SIZE);
        fps_cnt++;
    }

    // --- FPS report every 5 seconds ---
    uint32_t now = millis();
    if (now - fps_ts >= 5000) {
        Serial.printf("[FPS] %.1f  [Mode] %d\n",
                      (float)fps_cnt * 1000.0f / (float)(now - fps_ts),
                      (int)display_get_mode());
        fps_cnt = 0;
        fps_ts  = now;
    }
}
