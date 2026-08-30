#include <Arduino.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include "i2s_mic.h"
#include "display.h"
#include "button.h"
#include "encoder.h"
#include "nvs_storage.h"
#include "bt_audio.h"
#include "wifi_app.h"
#include "sd_card.h"
#include "mp3_player.h"
#include "beat_detector.h"
#include "games/game_manager.h"
#include "log.h"

/**
 * ESP32-WROOM — Dual-channel Sound Visualizer V2 + MicroSD MP3 Player
 *
 * Pinout & Controls:
 *   OLED SH1106 1.3"  : SDA=GPIO21, SCL=GPIO22 (Hardware I2C)
 *   INMP441 Mic (L/R) : SCK=GPIO26, WS=GPIO25, SD=GPIO27
 *   PCM5102A DAC      : BCK=GPIO18, LCK=GPIO19, DIN=GPIO23
 *   MicroSD Card SPI  : CS=GPIO5, SCK=GPIO16, MOSI=GPIO17, MISO=GPIO34
 *   Encoder EC11      : CLK=GPIO32, DT=GPIO33 (Volume & Menu Navigation)
 *   Button PUSH (PSH) : GPIO4  (Mode switch / Open Playlist Menu)
 *   Button BACK (BAK) : GPIO13 (Play/Pause / Prev track / Menu Back)
 *   Button PLUS (CON) : GPIO14 (Next effect / Next track / Menu Select)
 *   Button BOOT       : GPIO0  (WiFi Config AP / BT Re-pairing)
 */

static QueueHandle_t s_audio_queue = nullptr;
static AudioMode     s_current_mode = AUDIO_MODE_MIC;
static volatile bool s_mic_task_active = false;
static volatile bool s_mic_is_reading = false;

// -----------------------------------------------------------------------
// FreeRTOS Mic Task — Core 0
// -----------------------------------------------------------------------
static void mic_task(void * /*arg*/) {
    static AudioFrame frame;
    for (;;) {
        if (s_mic_task_active) {
            s_mic_is_reading = true;
            if (i2s_mic_read(frame.left, frame.right, FRAME_SIZE)) {
                if (s_audio_queue) {
                    xQueueSend(s_audio_queue, &frame, 0);
                }
            } else {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            s_mic_is_reading = false;
        } else {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

// -----------------------------------------------------------------------
// Graceful Audio Mode Switcher
// -----------------------------------------------------------------------
static Mp3Screen s_mp3_prev_screen = MP3_SCREEN_NORMAL;

static void switch_audio_mode(AudioMode target_mode) {
    if (target_mode == s_current_mode) return;

    static const char *TARGET_NAMES[] = {
        "MICROPHONE",
        "BLUETOOTH",
        "CLOCK & WEATHER",
        "MP3 PLAYER",
        "GAME CONSOLE"
    };
    const char *tgt_title = TARGET_NAMES[target_mode % 5];

    LOG_I("Switch", "Transitioning Mode %d -> %d (%s)...", 
          (int)s_current_mode, (int)target_mode, tgt_title);

    // Lưu trạng thái trước khi khởi động lại
    nvs_save_audio_mode(target_mode);
    if (s_current_mode == AUDIO_MODE_SD_MP3 && mp3_player_get_current_track_index() >= 0) {
        nvs_save_sd_track_index((uint16_t)mp3_player_get_current_track_index());
    }
    if (s_current_mode == AUDIO_MODE_SD_MP3) {
        mp3_player_stop(); // Clean stop I2S before reboot
    }

    // Khởi động lại lập tức để tạo cảm giác chuyển mode siêu tốc
    // (Màn hình sẽ chỉ chớp đen trong nháy mắt ~1s thay vì hiện thông báo loading)
    LOG_I("Switch", "Rebooting to guarantee clean RAM for new mode...");
    ESP.restart();
}

// -----------------------------------------------------------------------
// setup() — Core 1
// -----------------------------------------------------------------------
void setup() {
#ifdef ENABLE_SERIAL_LOG
    Serial.begin(115200);
    delay(200);
    Serial.println("=========================================");
    Serial.println("=== ESP32-WROOM Sound Visualizer V2 ===");
    Serial.println("=========================================");
#endif

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
        LOG_E("Boot", "Audio queue creation failed (OOM)");
        display_error("QUEUE FAIL");
        while (true) { delay(1000); }
    }

    // 5. Audio subsystems init
    bt_audio_init(s_audio_queue);
    mp3_player_init(s_audio_queue);

    // 6. Beat detector init
    beat_detector_init();

    // 7. Mount SD Card immediately to secure ~25KB DMA memory BEFORE BT stack fragments the heap
    if (!sd_card_init()) {
        LOG_W("Boot", "SD Card not found at boot. MP3 mode might be unavailable.");
    }

    // 8. Restore and start ONLY the selected Audio Mode
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
    } else if (s_current_mode == AUDIO_MODE_SD_MP3) {
        s_mic_task_active = false;
        encoder_set_enabled(true);
        bt_audio_stop();
        wifi_app_stop();
        
        mp3_player_start(); // Reserve I2S DMA memory first

        if (sd_card_is_mounted() && sd_card_scan_tracks() > 0) {
            display_set_mp3_screen(MP3_SCREEN_NORMAL);
            s_mp3_prev_screen = MP3_SCREEN_NORMAL;
            if (!mp3_player_is_playing()) {
                mp3_player_play_track(nvs_load_sd_track_index() % sd_card_get_track_count());
            }
            display_set_audio_mode(AUDIO_MODE_SD_MP3, true, true);
            display_toast("MODE: MP3 PLAYER");
        } else {
            mp3_player_stop(); // Free I2S so Mic can use it
            s_current_mode = AUDIO_MODE_MIC;
            nvs_save_audio_mode(AUDIO_MODE_MIC);
            encoder_set_enabled(false);
            if (i2s_mic_init()) s_mic_task_active = true;
            display_set_audio_mode(AUDIO_MODE_MIC, false, false);
            display_toast("MODE: MICROPHONE");
        }
    } else if (s_current_mode == AUDIO_MODE_GAME) {
        s_mic_task_active = false;
        encoder_set_enabled(true);
        bt_audio_stop();
        wifi_app_stop();
        mp3_player_stop();
        game_manager_init();
        display_set_audio_mode(AUDIO_MODE_GAME, false, false);
        display_toast("MODE: GAME CONSOLE");
    } else {
        encoder_set_enabled(false);
        bt_audio_stop();
        wifi_app_stop();
        display_set_audio_mode(AUDIO_MODE_MIC, false, false);
        if (i2s_mic_init()) {
            s_mic_task_active = true;
        } else {
            LOG_W("Boot", "I2S mic init failed. Continuing (fault-tolerant).");
        }
        display_toast("MODE: MICROPHONE");
    }

    // 8. Create Mic task on Core 0 AFTER mode is configured
    xTaskCreatePinnedToCore(
        mic_task, "mic_task", 4096, nullptr, 2, nullptr, 0
    );

    LOG_I("Boot", "Setup completed successfully");
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

    // =======================================================================
    // 1. BUTTON PUSH (GPIO 4 - Encoder Push Button)
    // Rule: ALWAYS switches audio mode (MIC -> BT -> CLOCK -> MP3 -> GAME -> MIC).
    // =======================================================================
    if (button_pressed(BTN_PUSH)) {
        AudioMode next_mode;
        if (s_current_mode == AUDIO_MODE_MIC) next_mode = AUDIO_MODE_BT;
        else if (s_current_mode == AUDIO_MODE_BT) next_mode = AUDIO_MODE_CLOCK;
        else if (s_current_mode == AUDIO_MODE_CLOCK) next_mode = AUDIO_MODE_SD_MP3;
        else if (s_current_mode == AUDIO_MODE_SD_MP3) next_mode = AUDIO_MODE_GAME;
        else next_mode = AUDIO_MODE_MIC;
        switch_audio_mode(next_mode);
    }

    // =======================================================================
    // GAME CONSOLE MODE HANDLING (Dedicated 35-40 FPS Game Loop)
    // =======================================================================
    if (s_current_mode == AUDIO_MODE_GAME) {
        bool btn_plus_pressed = button_pressed(BTN_PLUS);
        bool btn_back_pressed = button_pressed(BTN_BACK);
        bool btn_plus_down    = button_is_down(BTN_PLUS);
        int32_t enc_delta     = encoder_get_delta();

        game_manager_update(enc_delta, btn_plus_pressed, btn_back_pressed, btn_plus_down);
        game_manager_render();
        fps_cnt++;

        uint32_t now = millis();
        if (now - fps_ts >= 5000) {
            LOG_I("STATUS", "Mode: GAME CONSOLE | FPS: %.1f", (float)fps_cnt * 1000.0f / (float)(now - fps_ts));
            fps_cnt = 0;
            fps_ts  = now;
        }

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(15)); // ~40 FPS smooth frame rate
        return;
    }

    // =======================================================================
    // 2. BUTTON BACK (GPIO 13)
    // =======================================================================
    if (s_current_mode == AUDIO_MODE_SD_MP3) {
        Mp3Screen mp3_scr = display_get_mp3_screen();
        if (mp3_scr == MP3_SCREEN_PLAYLIST) {
            if (button_pressed(BTN_BACK)) {
                // s3: Return to previous screen (s1 or s2)
                display_set_mp3_screen(s_mp3_prev_screen);
            }
        } else {
            // s1 or s2:
            // NOTE: Use else-if to prevent short press and long press firing simultaneously
            if (button_long_pressed(BTN_BACK)) {
                // Long press >= 600ms: Previous track (check FIRST, takes priority)
                LOG_I("BTN", "BACK long-pressed (Hold -> Prev Track)");
                mp3_player_prev_track();
                display_toast("PREV TRACK");
            } else if (button_pressed(BTN_BACK)) {
                // Click: Play / Pause toggle
                LOG_I("BTN", "BACK pressed (Short -> Play/Pause)");
                mp3_player_toggle_play_pause();
                display_toast(mp3_player_is_paused() ? "PAUSE" : "PLAY");
            }
        }
    } else if (s_current_mode == AUDIO_MODE_BT) {
        if (button_pressed(BTN_BACK)) {
            bt_audio_play_pause();
        }
    } else if (s_current_mode == AUDIO_MODE_CLOCK) {
        if (button_pressed(BTN_BACK)) {
            display_toast("CLOCK MODE");
        }
    } else {
        if (button_pressed(BTN_BACK)) {
            display_toast("MIC MODE");
        }
    }

    // =======================================================================
    // 3. BUTTON CONFIRM / PLUS (GPIO 14)
    // =======================================================================
    if (s_current_mode == AUDIO_MODE_SD_MP3) {
        Mp3Screen mp3_scr = display_get_mp3_screen();
        if (mp3_scr == MP3_SCREEN_PLAYLIST) {
            // s3: Playlist menu
            if (button_pressed(BTN_PLUS)) {
                // Select currently focused track -> switch to s1 (Player Normal) and play it
                int sel = display_mp3_playlist_get_focus();
                mp3_player_play_track(sel);
                display_set_mp3_screen(MP3_SCREEN_NORMAL);
            }
        } else if (mp3_scr == MP3_SCREEN_NORMAL) {
            // s1: Normal Player
            if (button_held_3s(BTN_PLUS)) {
                // Hold 3s: Switch to s2 (Visualizer)
                display_set_mp3_screen(MP3_SCREEN_VISUALIZER);
                display_toast("VISUALIZER");
            } else if (button_long_pressed(BTN_PLUS)) {
                // Hold 1s: Next track
                mp3_player_next_track();
                display_toast("NEXT TRACK");
            } else if (button_pressed(BTN_PLUS)) {
                // Click (< 1s): Switch to s3 (Playlist)
                s_mp3_prev_screen = MP3_SCREEN_NORMAL;
                display_mp3_playlist_set_focus(mp3_player_get_current_track_index());
                display_set_mp3_screen(MP3_SCREEN_PLAYLIST);
            }
        } else if (mp3_scr == MP3_SCREEN_VISUALIZER) {
            // s2: Visualizer
            if (button_held_3s(BTN_PLUS)) {
                // Hold 3s: Switch to s1 (Player Normal)
                display_set_mp3_screen(MP3_SCREEN_NORMAL);
                display_toast("PLAYER");
            } else if (button_long_pressed(BTN_PLUS)) {
                // Hold 1s: Next track
                mp3_player_next_track();
                display_toast("NEXT TRACK");
            } else if (button_pressed(BTN_PLUS)) {
                // Click (< 1s): Switch to s3 (Playlist)
                s_mp3_prev_screen = MP3_SCREEN_VISUALIZER;
                display_mp3_playlist_set_focus(mp3_player_get_current_track_index());
                display_set_mp3_screen(MP3_SCREEN_PLAYLIST);
            }
        }
    } else {
        // Other modes (MIC, BT, CLOCK)
        if (button_pressed(BTN_PLUS)) {
            display_next_mode();
            nvs_save_display_mode((uint8_t)display_get_mode());
            LOG_D("BTN", "PLUS pressed -> Mode: %d", (int)display_get_mode());
        }
        if (button_long_pressed(BTN_PLUS)) {
            bool auto_cycle = !display_get_auto_cycle();
            display_set_auto_cycle(auto_cycle);
            nvs_save_auto_cycle(auto_cycle);
            LOG_D("Mode", "Auto-cycle %s", auto_cycle ? "ON" : "OFF");
        }
    }

    // =======================================================================
    // 4. BUTTON BOOT (GPIO 0)
    // =======================================================================
    if (s_current_mode == AUDIO_MODE_BT) {
        if (button_long_pressed(BTN_BOOT)) {
            LOG_I("BTN", "BT Mode: BOOT long pressed -> BT Re-pairing requested");
            bt_audio_start_repairing();
            display_toast("BT RE-PAIRING...");
        }
    } else if (s_current_mode == AUDIO_MODE_CLOCK) {
        if (button_pressed(BTN_BOOT) || button_long_pressed(BTN_BOOT)) {
            LOG_I("BTN", "CLOCK Mode: BOOT pressed -> Launching WiFi Web Setup AP...");
            wifi_app_start_ap_portal();
        }
    }

    // =======================================================================
    // 5. ROTARY ENCODER EC11 (GPIO 32 & 33)
    // =======================================================================
    int32_t enc_delta = encoder_get_delta();
    if (enc_delta != 0) {
        if (s_current_mode == AUDIO_MODE_BT) {
            bt_audio_adjust_volume(enc_delta);
            display_show_volume(bt_audio_get_volume(), 1500);
            LOG_D("ENC", "BT Volume: %d%%", (int)((float)bt_audio_get_volume() * 100.0f / 127.0f + 0.5f));
        } else if (s_current_mode == AUDIO_MODE_SD_MP3) {
            Mp3Screen mp3_scr = display_get_mp3_screen();
            if (mp3_scr == MP3_SCREEN_PLAYLIST) {
                // s3: Scroll focus up/down in playlist
                display_mp3_playlist_scroll(enc_delta);
            } else if (mp3_scr == MP3_SCREEN_NORMAL) {
                // s1: Adjust volume (UI updates volume live on footer)
                mp3_player_adjust_volume(enc_delta);
            } else if (mp3_scr == MP3_SCREEN_VISUALIZER) {
                // s2: Adjust volume + show center volume popup
                mp3_player_adjust_volume(enc_delta);
                display_show_volume(mp3_player_get_volume(), 1500);
            }
        }
    }

    // --- Update MP3 Player status to display ---
    if (s_current_mode == AUDIO_MODE_SD_MP3) {
        const PlaylistItem *cur = mp3_player_get_current_track();
        display_set_mp3_status(
            mp3_player_is_playing(),
            mp3_player_is_paused(),
            cur ? cur->title : "Track",
            mp3_player_get_current_track_index(),
            sd_card_get_track_count(),
            mp3_player_get_current_pos_sec(),
            mp3_player_get_total_duration_sec(),
            mp3_player_get_progress_percent(),
            mp3_player_get_volume()
        );
    }

    // --- Update display audio status ---
    display_set_audio_mode(
        s_current_mode,
        (s_current_mode == AUDIO_MODE_BT) ? bt_audio_is_connected() : (s_current_mode == AUDIO_MODE_SD_MP3 ? mp3_player_is_active() : false),
        (s_current_mode == AUDIO_MODE_BT) ? bt_audio_is_playing() : (s_current_mode == AUDIO_MODE_SD_MP3 ? mp3_player_is_playing() : false)
    );

    // --- WiFi & OTA background loop (ONLY run when in CLOCK mode) ---
    if (s_current_mode == AUDIO_MODE_CLOCK) {
        wifi_app_loop(false);
    }

    // --- Render: dequeue audio frame (or generate silence if idle/standby) and draw ---
    static uint32_t last_render_ts = 0;

    // Drain stale frames to maintain 0ms real-time audio sync and avoid queue congestion
    if (s_audio_queue) {
        AudioFrame temp;
        while (uxQueueMessagesWaiting(s_audio_queue) > 1) {
            xQueueReceive(s_audio_queue, &temp, 0);
        }
    }

    bool has_audio = (s_audio_queue && xQueueReceive(s_audio_queue, &frame, pdMS_TO_TICKS(5)) == pdTRUE);

    if (has_audio) {
        display_draw_waveform(frame.left, frame.right, FRAME_SIZE);
        fps_cnt++;
        last_render_ts = millis();
    } else {
        // Maintain ~30 FPS continuous UI refresh during silence / standby / idle
        uint32_t now = millis();
        if (now - last_render_ts >= 33) {
            memset(frame.left, 0, sizeof(frame.left));
            memset(frame.right, 0, sizeof(frame.right));
            display_draw_waveform(frame.left, frame.right, FRAME_SIZE);
            fps_cnt++;
            last_render_ts = now;
        }
    }

    // --- Periodic status report every 5 seconds ---
    uint32_t now = millis();
    if (now - fps_ts >= 5000) {
        if (s_current_mode == AUDIO_MODE_SD_MP3) {
            const PlaylistItem *cur = mp3_player_get_current_track();
            uint32_t cur_s = mp3_player_get_current_pos_sec();
            uint32_t tot_s = mp3_player_get_total_duration_sec();
            uint8_t pct    = mp3_player_get_progress_percent();
            int cur_idx    = mp3_player_get_current_track_index();
            int total_trk  = sd_card_get_track_count();
            const char *state = mp3_player_is_paused() ? "PAUSED" : (mp3_player_is_playing() ? "PLAYING" : "STOPPED");

            if (tot_s >= 3600 || cur_s >= 3600) {
                LOG_I("MP3", "[%02d/%02d] '%s' | %s | %02d:%02d:%02d/%02d:%02d:%02d (%d%%) | Vol: %d%%",
                      cur_idx + 1, total_trk,
                      cur ? cur->title : "Unknown",
                      state,
                      (int)(cur_s / 3600), (int)((cur_s % 3600) / 60), (int)(cur_s % 60),
                      (int)(tot_s / 3600), (int)((tot_s % 3600) / 60), (int)(tot_s % 60),
                      (int)pct,
                      (int)((float)mp3_player_get_volume() * 100.0f / 127.0f + 0.5f));
            } else {
                LOG_I("MP3", "[%02d/%02d] '%s' | %s | %02d:%02d/%02d:%02d (%d%%) | Vol: %d%%",
                      cur_idx + 1, total_trk,
                      cur ? cur->title : "Unknown",
                      state,
                      (int)(cur_s / 60), (int)(cur_s % 60),
                      (int)(tot_s / 60), (int)(tot_s % 60),
                      (int)pct,
                      (int)((float)mp3_player_get_volume() * 100.0f / 127.0f + 0.5f));
            }
        } else {
            const char *mname = (s_current_mode == AUDIO_MODE_BT) ? "BT" :
                                ((s_current_mode == AUDIO_MODE_CLOCK) ? "CLOCK" : "MIC");
            LOG_I("STATUS", "Mode: %s | FPS: %.1f | Effect: %d | WiFi: %s",
                          mname,
                          (float)fps_cnt * 1000.0f / (float)(now - fps_ts),
                          (int)display_get_mode(),
                          wifi_app_is_connected() ? "ON" : "OFF");
        }
        fps_cnt = 0;
        fps_ts  = now;
    }

    // Explicitly reset Task Watchdog and yield 2ms to Core 1 IDLE task
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(2));
}
