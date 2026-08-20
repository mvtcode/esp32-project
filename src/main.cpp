#include <Arduino.h>
#include <WiFi.h>
#include "i2s_mic.h"
#include "display.h"
#include "button.h"
#include "encoder.h"
#include "nvs_storage.h"
#include "bt_audio.h"
#include "wifi_app.h"

/**
 * ESP32-WROOM — Dual-channel Sound Visualizer V2
 *
 * Pinout & Controls:
 *   OLED SH1106 1.3"  : SDA=GPIO21, SCL=GPIO22 (Hardware I2C)
 *   INMP441 Mic (L/R) : SCK=GPIO26, WS=GPIO25, SD=GPIO27
 *   PCM5102A DAC      : BCK=GPIO18, LCK=GPIO19, DIN=GPIO23
 *   Encoder EC11      : CLK=GPIO32, DT=GPIO33 (Volume adjust with AVRCP sync)
 *   Button PUSH (PSH) : GPIO4  (Switch MIC <-> BT mode)
 *   Button BACK (BAK) : GPIO13 (Play/Pause AVRCP)
 *   Button PLUS (CON) : GPIO14 (Next display mode / Long: Auto-cycle)
 *   Button BOOT       : GPIO0  (Short: WiFi reset, Long >3s: BT pair)
 */

static QueueHandle_t s_audio_queue = nullptr;
static AudioMode     s_current_mode = AUDIO_MODE_MIC;
static volatile bool s_mic_task_active = false;

// -----------------------------------------------------------------------
// FreeRTOS Mic Task — Core 0
// -----------------------------------------------------------------------
static void mic_task(void * /*arg*/) {
    static AudioFrame frame;
    for (;;) {
        if (s_mic_task_active) {
            if (i2s_mic_read(frame.left, frame.right, FRAME_SIZE)) {
                if (s_audio_queue) {
                    xQueueSend(s_audio_queue, &frame, 0);
                }
            } else {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// Graceful Audio Mode Switcher
// -----------------------------------------------------------------------
static void switch_audio_mode(AudioMode target_mode) {
    if (target_mode == s_current_mode) return;

    Serial.printf("[Switch] Transitioning %s -> %s...\n",
                  s_current_mode == AUDIO_MODE_BT ? "BT" : (s_current_mode == AUDIO_MODE_CLOCK ? "CLOCK" : "MIC"),
                  target_mode == AUDIO_MODE_BT ? "BT" : (target_mode == AUDIO_MODE_CLOCK ? "CLOCK" : "MIC"));

    if (target_mode == AUDIO_MODE_BT) {
        // 1. Suspend mic task and deinit I2S mic
        s_mic_task_active = false;
        delay(40);
        i2s_mic_deinit();

        // 2. Clear audio queue
        if (s_audio_queue) xQueueReset(s_audio_queue);

        // 3. Stop WiFi completely -> 100% 2.4GHz RF dedicated to BT Classic
        wifi_app_stop();

        // 4. Start Bluetooth A2DP Sink & Enable Rotary Encoder
        bt_audio_start();
        encoder_set_enabled(true);
        display_set_audio_mode(AUDIO_MODE_BT, bt_audio_is_connected(), false);

        s_current_mode = AUDIO_MODE_BT;
        nvs_save_audio_mode(AUDIO_MODE_BT);
        display_toast("MODE: BLUETOOTH");
    } 
    else if (target_mode == AUDIO_MODE_CLOCK) {
        // 1. Stop Bluetooth A2DP Sink & Disable Rotary Encoder
        encoder_set_enabled(false);
        bt_audio_stop();

        // 2. Suspend mic task and deinit I2S mic
        s_mic_task_active = false;
        delay(40);
        i2s_mic_deinit();

        // 3. Clear audio queue
        if (s_audio_queue) xQueueReset(s_audio_queue);

        // 4. Start WiFi for NTP clock & weather
        wifi_app_init();

        display_set_audio_mode(AUDIO_MODE_CLOCK, false, false);

        s_current_mode = AUDIO_MODE_CLOCK;
        nvs_save_audio_mode(AUDIO_MODE_CLOCK);
        display_toast("MODE: CLOCK & WEATHER");
    }
    else { // AUDIO_MODE_MIC
        // 1. Stop Bluetooth A2DP Sink & Disable Rotary Encoder
        encoder_set_enabled(false);
        bt_audio_stop();

        // 2. Stop WiFi completely (no background web/OTA/NTP)
        wifi_app_stop();

        // 3. Clear audio queue
        if (s_audio_queue) xQueueReset(s_audio_queue);

        // 4. Re-init I2S mic and resume mic task
        if (i2s_mic_init()) {
            s_mic_task_active = true;
        } else {
            Serial.println("[WARN] Mic re-init failed (no hardware attached?)");
        }
        display_set_audio_mode(AUDIO_MODE_MIC, false, false);

        s_current_mode = AUDIO_MODE_MIC;
        nvs_save_audio_mode(AUDIO_MODE_MIC);
        display_toast("MODE: MICROPHONE");
    }
}

// -----------------------------------------------------------------------
// setup() — Core 1
// -----------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("=========================================");
    Serial.println("=== ESP32-WROOM Sound Visualizer V2 ===");
    Serial.println("=========================================");

    // 1. NVS storage init
    nvs_storage_init();

    // 2. Display init and restore preferences
    display_init();
    display_set_mode((DisplayMode)nvs_load_display_mode(), false);
    display_set_auto_cycle(nvs_load_auto_cycle());

    // 3. Multi-button & Encoder system
    buttons_init();
    encoder_init();

    // 4. Audio queue create
    s_audio_queue = xQueueCreate(4, sizeof(AudioFrame));
    if (!s_audio_queue) {
        Serial.println("[FATAL] Audio queue creation failed (OOM)");
        display_error("QUEUE FAIL");
        while (true) { delay(1000); }
    }

    // 5. Bluetooth audio subsystem init
    bt_audio_init(s_audio_queue);

    // 6. Restore and start ONLY the selected Audio Mode
    s_current_mode = nvs_load_audio_mode();
    if (s_current_mode == AUDIO_MODE_BT) {
        s_mic_task_active = false;
        wifi_app_stop();
        bt_audio_start();
        encoder_set_enabled(true);
        display_set_audio_mode(AUDIO_MODE_BT, false, false);
        display_toast("MODE: BLUETOOTH");
    } else if (s_current_mode == AUDIO_MODE_CLOCK) {
        s_mic_task_active = false;
        encoder_set_enabled(false);
        bt_audio_stop();
        wifi_app_init();
        display_set_audio_mode(AUDIO_MODE_CLOCK, false, false);
        display_toast("MODE: CLOCK & WEATHER");
    } else {
        encoder_set_enabled(false);
        bt_audio_stop();
        wifi_app_stop();
        display_set_audio_mode(AUDIO_MODE_MIC, false, false);
        if (i2s_mic_init()) {
            s_mic_task_active = true;
        } else {
            Serial.println("[WARN] I2S mic init failed. Continuing (fault-tolerant).");
        }
        display_toast("MODE: MICROPHONE");
    }

    // 7. Create Mic task on Core 0 AFTER mode is configured
    xTaskCreatePinnedToCore(
        mic_task, "mic_task", 4096, nullptr, 2, nullptr, 0
    );

    Serial.println("[boot] Setup completed successfully");
}

// -----------------------------------------------------------------------
// loop() — Core 1
// -----------------------------------------------------------------------
void loop() {
    static AudioFrame frame;
    static uint32_t   fps_ts  = 0;
    static uint32_t   fps_cnt = 0;

    // --- Button & Encoder handling ---
    buttons_update();

    // Button PLUS (GPIO 14) -> Next OLED mode / Long press: Auto-cycle
    if (button_pressed(BTN_PLUS)) {
        display_next_mode();
        nvs_save_display_mode((uint8_t)display_get_mode());
        Serial.printf("[BTN] PLUS pressed -> Mode: %d\n", (int)display_get_mode());
    }
    if (button_long_pressed(BTN_PLUS)) {
        bool auto_cycle = !display_get_auto_cycle();
        display_set_auto_cycle(auto_cycle);
        nvs_save_auto_cycle(auto_cycle);
        Serial.printf("[Mode] Auto-cycle %s\n", auto_cycle ? "ON" : "OFF");
    }

    // Button PUSH (GPIO 4) -> Cycle Mode: MIC -> BT -> CLOCK -> MIC
    if (button_pressed(BTN_PUSH)) {
        AudioMode next_mode;
        if (s_current_mode == AUDIO_MODE_MIC) next_mode = AUDIO_MODE_BT;
        else if (s_current_mode == AUDIO_MODE_BT) next_mode = AUDIO_MODE_CLOCK;
        else next_mode = AUDIO_MODE_MIC;
        switch_audio_mode(next_mode);
    }

    // Button BACK (GPIO 13) -> Play/Pause via AVRCP (when in BT mode)
    if (button_pressed(BTN_BACK)) {
        if (s_current_mode == AUDIO_MODE_BT) {
            bt_audio_play_pause();
        } else if (s_current_mode == AUDIO_MODE_CLOCK) {
            display_toast("CLOCK MODE");
        } else {
            display_toast("MIC MODE");
        }
    }

    // Button BOOT (GPIO 0) -> Mode-dependent actions:
    // - In BT Mode: Long press (>1s) -> BT Re-pairing
    // - In CLOCK Mode: Press -> Launch WiFi Config Portal AP Mode
    if (s_current_mode == AUDIO_MODE_BT) {
        if (button_long_pressed(BTN_BOOT)) {
            Serial.println("[BTN] BT Mode: BOOT long pressed -> BT Re-pairing requested");
            bt_audio_start_repairing();
            display_toast("BT RE-PAIRING...");
        }
    } else if (s_current_mode == AUDIO_MODE_CLOCK) {
        if (button_pressed(BTN_BOOT) || button_long_pressed(BTN_BOOT)) {
            Serial.println("[BTN] CLOCK Mode: BOOT pressed -> Launching WiFi Web Setup AP...");
            wifi_app_start_ap_portal();
        }
    }

    // Rotary Encoder rotation -> Volume change & sync ONLY in Bluetooth mode
    if (s_current_mode == AUDIO_MODE_BT) {
        int32_t enc_delta = encoder_get_delta();
        if (enc_delta != 0) {
            bt_audio_adjust_volume(enc_delta);
            display_show_volume(bt_audio_get_volume());
            Serial.printf("[ENC] Volume adjust: %d%%\n", (int)((float)bt_audio_get_volume() * 100.0f / 127.0f));
        }
    }

    // Update display audio status
    display_set_audio_mode(
        s_current_mode,
        bt_audio_is_connected(),
        bt_audio_is_playing()
    );

    // --- WiFi & OTA background loop (ONLY run when in CLOCK mode) ---
    if (s_current_mode == AUDIO_MODE_CLOCK) {
        wifi_app_loop(false);
    }

    // --- Render: dequeue audio frame (or generate silence if idle/BT standby) and draw ---
    static uint32_t last_render_ts = 0;
    bool has_audio = (xQueueReceive(s_audio_queue, &frame, pdMS_TO_TICKS(5)) == pdTRUE);

    if (has_audio) {
        display_draw_waveform(frame.left, frame.right, FRAME_SIZE);
        fps_cnt++;
        last_render_ts = millis();
    } else {
        // Maintain ~30 FPS continuous UI refresh during silence / BT standby / idle
        uint32_t now = millis();
        if (now - last_render_ts >= 33) {
            memset(frame.left, 0, sizeof(frame.left));
            memset(frame.right, 0, sizeof(frame.right));
            display_draw_waveform(frame.left, frame.right, FRAME_SIZE);
            fps_cnt++;
            last_render_ts = now;
        }
    }

    // --- FPS report every 5 seconds ---
    uint32_t now = millis();
    if (now - fps_ts >= 5000) {
        Serial.printf("[STATUS] Mode: %s | FPS: %.1f | Effect: %d | WiFi: %s\n",
                      s_current_mode == AUDIO_MODE_BT ? "BT" : "MIC",
                      (float)fps_cnt * 1000.0f / (float)(now - fps_ts),
                      (int)display_get_mode(),
                      wifi_app_is_connected() ? "ON" : "OFF");
        fps_cnt = 0;
        fps_ts  = now;
    }
}



