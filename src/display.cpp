#include "display.h"
#include "effects/effects.h"
#include "wifi_app.h"
#include "lunar_calendar.h"
#include "log.h"
#include "beat_detector.h"
#include <WiFi.h>

// -----------------------------------------------------------------------
// U8g2 instance — SH1106 128x64, Hardware I2C, full framebuffer
// Wire is configured in display_init() before u8g2.begin().
// -----------------------------------------------------------------------
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// -----------------------------------------------------------------------
// Shared Audio reference state & buffers (Fixed scale, no auto-magnification)
// -----------------------------------------------------------------------
int32_t s_peak_l = (int32_t)AUDIO_NOMINAL_PEAK;
int32_t s_peak_r = (int32_t)AUDIO_NOMINAL_PEAK;

// Shared FFT instance across spectrum-based effects
float s_fft_real[FRAME_SIZE];
float s_fft_imag[FRAME_SIZE];
ArduinoFFT<float> s_fft(s_fft_real, s_fft_imag, FRAME_SIZE, (float)SAMPLE_RATE);

// -----------------------------------------------------------------------
// Mode management
// -----------------------------------------------------------------------
static DisplayMode s_mode          = MODE_WAVEFORM;
static uint32_t   s_label_ts       = 0;
static const uint32_t LABEL_DUR_MS = 1500;
static volatile bool  s_switching  = false; // Guard: prevent render during mode switch

// -----------------------------------------------------------------------
// Audio Level Reference Updates (Fixed Reference with Soft Headroom Limiter)
// -----------------------------------------------------------------------
static int32_t compute_peak(const int32_t *buf, size_t n) {
    int32_t pk = 0;
    for (size_t i = 0; i < n; i++) {
        int32_t v = buf[i] < 0 ? -buf[i] : buf[i];
        if (v > pk) pk = v;
    }
    return pk;
}

static void update_agc(const int32_t *left, const int32_t *right, size_t n) {
    int32_t cl = compute_peak(left,  n);
    int32_t cr = compute_peak(right, n);
    // Fixed scale mode: maintains linear 1:1 true volume scaling; expands headroom only if signal exceeds nominal
    s_peak_l = cl > (int32_t)AUDIO_NOMINAL_PEAK ? cl : (int32_t)AUDIO_NOMINAL_PEAK;
    s_peak_r = cr > (int32_t)AUDIO_NOMINAL_PEAK ? cr : (int32_t)AUDIO_NOMINAL_PEAK;
}

// -----------------------------------------------------------------------
// Mode label overlay — shown for LABEL_DUR_MS after every mode switch
// -----------------------------------------------------------------------
static void draw_mode_label() {
    if (millis() - s_label_ts >= LABEL_DUR_MS) return;

    u8g2.setFont(u8g2_font_6x10_tf);
    const char *name = EFFECTS[s_mode].name;
    int str_w = u8g2.getStrWidth(name);
    int x = (SCREEN_W - str_w) / 2;
    int y = 38;

    // Clear background box (white on black → invert region)
    u8g2.setDrawColor(0);
    u8g2.drawBox(x - 5, y - 12, str_w + 10, 14);
    u8g2.setDrawColor(1);
    u8g2.drawFrame(x - 5, y - 12, str_w + 10, 14);
    u8g2.drawStr(x, y, name);
}

// -----------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------

void display_init() {
    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(I2C_CLOCK);
    delay(200);
    u8g2.begin();

    // Hardware X-offset calibration (shifts display 1px to the left to perfectly center on OLED glass)
    u8g2.getU8x8()->x_offset = OLED_X_OFFSET;

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(22, 22, "Sound Viz");
    u8g2.drawStr(22, 38, "ESP32-WROOM");
    u8g2.drawHLine(0, 44, SCREEN_W);
    u8g2.setFont(u8g2_font_04b_03_tr);
    u8g2.drawStr(28, 56, "initialising...");
    u8g2.sendBuffer();

    LOG_I("OLED", "Init OK - HW_I2C %d kHz, %d modes", I2C_CLOCK / 1000, MODE_COUNT);
}

void display_error(const char *msg) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(2, 20, "!! ERROR !!");
    u8g2.drawStr(2, 38, msg);
    u8g2.drawFrame(0, 0, SCREEN_W, SCREEN_H);
    u8g2.sendBuffer();
}

static bool s_auto_cycle = false;
static uint32_t s_auto_cycle_interval = 20000;
static uint32_t s_last_cycle_ts = 0;
static uint32_t s_toast_ts = 0;
static uint32_t s_toast_dur = 2000;
static char s_toast_msg[32] = "";

static uint32_t s_volume_ts = 0;
static uint32_t s_volume_dur = 2000;
static uint8_t  s_display_vol = 80;

static AudioMode s_audio_mode = AUDIO_MODE_MIC;
static bool      s_bt_connected = false;
static bool      s_bt_playing = false;
static uint32_t  s_conn_splash_ts = 0;
static const uint32_t CONN_SPLASH_DUR = 2500;

void display_toast(const char *msg, uint32_t duration_ms) {
    if (!msg) return;
    strncpy(s_toast_msg, msg, sizeof(s_toast_msg) - 1);
    s_toast_msg[sizeof(s_toast_msg) - 1] = '\0';
    s_toast_dur = duration_ms;
    s_toast_ts = millis();
}

void display_show_volume(uint8_t volume, uint32_t duration_ms) {
    s_display_vol = volume > 127 ? 127 : volume;
    s_volume_dur = duration_ms;
    s_volume_ts = millis();
}

void display_set_brightness(uint8_t percent) {
    uint8_t contrast = (uint8_t)map(constrain(percent, 10, 100), 10, 100, 20, 255);
    u8g2.setContrast(contrast);
}

void display_set_audio_mode(AudioMode mode, bool is_connected, bool is_playing) {
    // Detect rising edge of Bluetooth connection
    if (mode == AUDIO_MODE_BT && s_audio_mode == AUDIO_MODE_BT && !s_bt_connected && is_connected) {
        s_conn_splash_ts = millis();
        Serial.println("[Display] Bluetooth Connected Splash Triggered!");
    }
    s_audio_mode = mode;
    s_bt_connected = is_connected;
    s_bt_playing = is_playing;
}

void display_set_mode(DisplayMode m, bool show_label) {
    if (m >= MODE_COUNT) m = (DisplayMode)0;
    if (m == s_mode) return;

    s_switching = true;

    // 1. Teardown old mode
    if (s_mode < MODE_COUNT && EFFECTS[s_mode].on_exit) {
        EFFECTS[s_mode].on_exit();
    }

    s_mode = m;

    // 2. Init new mode
    if (s_mode < MODE_COUNT && EFFECTS[s_mode].on_enter) {
        EFFECTS[s_mode].on_enter();
    }

    s_label_ts  = show_label ? millis() : 0;         // 3. show mode label conditionally
    s_last_cycle_ts = millis();                      // 4. reset auto-cycle timer on ANY mode switch
    s_switching = false;
    LOG_D("Mode", "» %s", EFFECTS[m].name);
}

void display_next_mode(bool show_label) {
    display_set_mode((DisplayMode)((s_mode + 1) % MODE_COUNT), show_label);
}

DisplayMode display_get_mode() { return s_mode; }

void display_set_auto_cycle(bool enable, uint32_t interval_ms) {
    s_auto_cycle = enable;
    if (interval_ms > 0) s_auto_cycle_interval = interval_ms;
    s_last_cycle_ts = millis();
    display_toast(enable ? "AUTO CYCLE: ON" : "AUTO CYCLE: OFF");
}

bool display_get_auto_cycle() { return s_auto_cycle; }

// -----------------------------------------------------------------------
// Dedicated Clock & Weather Screen (Standby / Standalone Mode)
// -----------------------------------------------------------------------
static void draw_wifi_setup_screen() {
    // 1. Header Boxed Banner
    u8g2.drawRBox(0, 0, SCREEN_W, 12, 2);
    u8g2.setDrawColor(0); // Inverted text on header
    u8g2.setFont(u8g2_font_6x10_tf);
    const char *title = "CAU HINH WIFI AP";
    u8g2.drawStr((SCREEN_W - u8g2.getStrWidth(title)) / 2, 9, title);
    u8g2.setDrawColor(1); // Normal drawing color

    // 2. AP SSID
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(4, 23, "1. Ket noi WiFi:");
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(12, 34, AP_SSID_NAME);

    // 3. Web URL / IP
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(4, 46, "2. Mo trinh duyet vao:");
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(12, 58, "http://192.168.4.1");

    // 4. Subtle Animated Signal Icon on right
    uint32_t step = (millis() / 250) % 3;
    int rx = 114;
    int ry = 30;
    u8g2.drawCircle(rx, ry, 2, U8G2_DRAW_ALL);
    if (step >= 1) u8g2.drawCircle(rx, ry, 5, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT);
    if (step >= 2) u8g2.drawCircle(rx, ry, 8, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT);
}

static void draw_clock_weather_screen(const int32_t * /*left*/, const int32_t * /*right*/, size_t /*n*/) {
    if (wifi_app_is_ap_mode()) {
        draw_wifi_setup_screen();
        return;
    }

    char time_str[32];
    wifi_app_get_time_str(time_str, sizeof(time_str));
    WeatherData w = wifi_app_get_weather();
    SolarDate sd = wifi_app_get_solar_date();

    // 1. Top Header: Left = Solar Date, Right = WiFi Signal strength
    static const char *DAYS_OF_WEEK[] = { "CN", "T2", "T3", "T4", "T5", "T6", "T7" };
    char date_str[32];
    if (sd.valid) {
        snprintf(date_str, sizeof(date_str), "%s %02d/%02d/%04d", 
                 DAYS_OF_WEEK[sd.day_of_week % 7], sd.day, sd.month, sd.year);
    } else {
        snprintf(date_str, sizeof(date_str), "DONG BO GIO...");
    }

    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(2, 9, date_str);

    // WiFi 5-bar signal icon on top right
    if (wifi_app_is_connected()) {
        int rssi = WiFi.RSSI();
        int bars = 1;
        if (rssi >= -55)      bars = 5;
        else if (rssi >= -65) bars = 4;
        else if (rssi >= -75) bars = 3;
        else if (rssi >= -85) bars = 2;
        else                  bars = 1;

        for (int b = 0; b < 5; b++) {
            int bx = 112 + b * 3;
            int bh = 2 + b * 2; // heights: 2, 4, 6, 8, 10
            if (b < bars) {
                u8g2.drawVLine(bx, 10 - bh, bh);
                u8g2.drawVLine(bx + 1, 10 - bh, bh);
            } else {
                u8g2.drawPixel(bx, 9);
                u8g2.drawPixel(bx + 1, 9);
            }
        }
    } else if (wifi_app_is_connecting()) {
        // Smooth animated 5-bar scan/wave loading effect
        uint32_t step = (millis() / 150) % 7;
        for (int b = 0; b < 5; b++) {
            int bx = 112 + b * 3;
            int bh = 2 + b * 2;
            if (b <= (int)step && step < 5) {
                u8g2.drawVLine(bx, 10 - bh, bh);
                u8g2.drawVLine(bx + 1, 10 - bh, bh);
            } else {
                u8g2.drawPixel(bx, 9);
                u8g2.drawPixel(bx + 1, 9);
            }
        }
    } else {
        u8g2.setFont(u8g2_font_5x8_tr);
        u8g2.drawStr(108, 9, "OFF");
    }

    u8g2.drawHLine(2, 12, SCREEN_W - 4);

    // 2. Digital Time (large centered font)
    u8g2.setFont(u8g2_font_courB18_tf);
    int tw = u8g2.getStrWidth(time_str);
    u8g2.drawStr((SCREEN_W - tw) / 2, 33, time_str);

    // 3. Weather & Temperature / Humidity
    u8g2.setFont(u8g2_font_6x10_tf);
    char wstr[32];
    snprintf(wstr, sizeof(wstr), "%.1f C   %d%% Hum", w.temp_c, w.humidity);
    u8g2.drawStr((SCREEN_W - u8g2.getStrWidth(wstr)) / 2, 47, wstr);

    // 4. Bottom Footer: Lunar Date (Ngày Âm Lịch)
    u8g2.drawHLine(4, 52, SCREEN_W - 8);

    char lunar_str[48];
    if (sd.valid) {
        int ld, lm, ly;
        solarToLunar(sd.year, sd.month, sd.day, ld, lm, ly);
        const char *year_name = getLunarYearName(ly);
        snprintf(lunar_str, sizeof(lunar_str), "AL: %02d/%02d (%s)", ld, lm, year_name);
    } else {
        snprintf(lunar_str, sizeof(lunar_str), "Am Lich: Dang tai...");
    }

    u8g2.setFont(u8g2_font_6x10_tf);
    int lw = u8g2.getStrWidth(lunar_str);
    u8g2.drawStr((SCREEN_W - lw) / 2, 62, lunar_str);
}

// -----------------------------------------------------------------------
// Bluetooth Connected Success Screen
// -----------------------------------------------------------------------
static void draw_bt_connected_screen(uint32_t /*now*/) {
    // Outer border
    u8g2.drawFrame(2, 2, SCREEN_W - 4, SCREEN_H - 4);
    u8g2.drawFrame(4, 4, SCREEN_W - 8, SCREEN_H - 8);

    // Title
    u8g2.setFont(u8g2_font_6x10_tf);
    const char *hdr = "[ BLUETOOTH ]";
    int hw = u8g2.getStrWidth(hdr);
    u8g2.drawStr((SCREEN_W - hw) / 2, 16, hdr);
    u8g2.drawHLine(10, 19, SCREEN_W - 20);

    // Checkmark circle (X=26, Y=36)
    int cx = 26, cy = 36;
    u8g2.drawCircle(cx, cy, 10);
    // Draw Checkmark
    u8g2.drawLine(cx - 5, cy, cx - 1, cy + 4);
    u8g2.drawLine(cx - 1, cy + 4, cx + 5, cy - 4);
    u8g2.drawLine(cx - 5, cy + 1, cx - 1, cy + 5);
    u8g2.drawLine(cx - 1, cy + 5, cx + 5, cy - 3);

    // Text info
    u8g2.setFont(u8g2_font_7x14B_tf);
    u8g2.drawStr(44, 34, "CONNECTED!");

    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(44, 46, "Audio Stream Ready");

    // Bottom subtitle
    u8g2.setFont(u8g2_font_04b_03_tr);
    const char *sub = "MVT VU METER ACTIVE";
    u8g2.drawStr((SCREEN_W - u8g2.getStrWidth(sub)) / 2, 57, sub);
}

// -----------------------------------------------------------------------
// Bluetooth Pairing & Waiting Standby Screen
// -----------------------------------------------------------------------
static void draw_bt_pairing_screen(uint32_t now) {
    // 1. Title bar
    u8g2.setFont(u8g2_font_6x10_tf);
    const char *title = "[BLUETOOTH MODE]";
    int tw = u8g2.getStrWidth(title);
    u8g2.drawStr((SCREEN_W - tw) / 2, 10, title);
    u8g2.drawHLine(4, 13, SCREEN_W - 8);

    // 2. Animated Bluetooth Icon (X=24, Y=33)
    int bx = 24;
    int by = 33;
    // Central stem
    u8g2.drawLine(bx, by - 12, bx, by + 12);
    u8g2.drawLine(bx + 1, by - 12, bx + 1, by + 12);
    // Upper & Lower wings
    u8g2.drawLine(bx, by - 12, bx + 7, by - 5);
    u8g2.drawLine(bx + 7, by - 5, bx - 6, by + 4);
    u8g2.drawLine(bx - 6, by - 4, bx + 7, by + 5);
    u8g2.drawLine(bx + 7, by + 5, bx, by + 12);

    // Animated radiating radar waves around icon
    int phase = (now / 300) % 3;
    for (int i = 0; i < 3; i++) {
        int r = 11 + i * 5;
        if (i <= phase) {
            u8g2.drawCircle(bx, by, r, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT);
            u8g2.drawCircle(bx, by, r, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_LOWER_LEFT);
        }
    }

    // 3. Connection info
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(50, 26, "Pair Name:");
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(50, 37, "MVT-Audio");

    // 4. Animated Status line
    static const char *states[] = {
        "Status: Waiting   ",
        "Status: Waiting . ",
        "Status: Waiting ..",
        "Status: Ready >>> "
    };
    int s_idx = (now / 400) % 4;
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(50, 48, states[s_idx]);

    // 5. Bottom hint
    u8g2.drawHLine(4, 53, SCREEN_W - 8);
    u8g2.setFont(u8g2_font_04b_03_tr);
    const char *hint = "DISCOVERABLE  -  CONNECT PHONE";
    u8g2.drawStr((SCREEN_W - u8g2.getStrWidth(hint)) / 2, 62, hint);
}

void display_draw_waveform(const int32_t *left, const int32_t *right, size_t n) {
    uint32_t now = millis();

    // 1. DEDICATED CLOCK & WEATHER MODE (Mode 2)
    if (s_audio_mode == AUDIO_MODE_CLOCK) {
        if (s_switching) return;
        s_last_cycle_ts = now; // Freeze visualizer auto-cycle

        u8g2.clearBuffer();
        draw_clock_weather_screen(left, right, n);

        // Volume popup overlay (takes priority when active)
        if (s_volume_ts > 0 && now - s_volume_ts < s_volume_dur) {
            u8g2.setFont(u8g2_font_6x10_tf);
            char vstr[16];
            int pct = (int)((float)s_display_vol * 100.0f / 127.0f + 0.5f);
            snprintf(vstr, sizeof(vstr), "VOL %d%%", pct);

            int box_w = 90;
            int box_h = 24;
            int bx = (SCREEN_W - box_w) / 2;
            int by = (SCREEN_H - box_h) / 2;

            u8g2.setDrawColor(0);
            u8g2.drawBox(bx, by, box_w, box_h);
            u8g2.setDrawColor(1);
            u8g2.drawFrame(bx, by, box_w, box_h);
            u8g2.drawStr(bx + (box_w - u8g2.getStrWidth(vstr)) / 2, by + 10, vstr);

            int bar_w = box_w - 12;
            int fill_w = (bar_w * s_display_vol) / 127;
            u8g2.drawFrame(bx + 6, by + 14, bar_w, 6);
            if (fill_w > 0) {
                u8g2.drawBox(bx + 6, by + 14, fill_w, 6);
            }
        } else if (s_toast_ts > 0 && now - s_toast_ts < s_toast_dur) {
            u8g2.setFont(u8g2_font_6x10_tf);
            int tw = u8g2.getStrWidth(s_toast_msg);
            int tx = (SCREEN_W - tw) / 2;
            int ty = 20;
            u8g2.setDrawColor(0);
            u8g2.drawBox(tx - 4, ty - 10, tw + 8, 14);
            u8g2.setDrawColor(1);
            u8g2.drawFrame(tx - 4, ty - 10, tw + 8, 14);
            u8g2.drawStr(tx, ty, s_toast_msg);
        }

        u8g2.sendBuffer();
        return;
    }

    // 2. BLUETOOTH PAIRING STANDBY SCREEN (When in BT mode and waiting for device connection)
    if (s_audio_mode == AUDIO_MODE_BT && !s_bt_connected) {
        if (s_switching) return;
        s_last_cycle_ts = now; // Freeze auto-cycle timer while waiting

        u8g2.clearBuffer();
        draw_bt_pairing_screen(now);

        // Allow Volume popup or Toast over Pairing screen if triggered
        if (s_volume_ts > 0 && now - s_volume_ts < s_volume_dur) {
            u8g2.setFont(u8g2_font_6x10_tf);
            char vstr[16];
            int pct = (int)((float)s_display_vol * 100.0f / 127.0f + 0.5f);
            snprintf(vstr, sizeof(vstr), "VOL %d%%", pct);

            int box_w = 90;
            int box_h = 24;
            int bx = (SCREEN_W - box_w) / 2;
            int by = (SCREEN_H - box_h) / 2;

            u8g2.setDrawColor(0);
            u8g2.drawBox(bx, by, box_w, box_h);
            u8g2.setDrawColor(1);
            u8g2.drawFrame(bx, by, box_w, box_h);
            u8g2.drawStr(bx + (box_w - u8g2.getStrWidth(vstr)) / 2, by + 10, vstr);

            int bar_w = box_w - 12;
            int fill_w = (bar_w * s_display_vol) / 127;
            u8g2.drawFrame(bx + 6, by + 14, bar_w, 6);
            if (fill_w > 0) {
                u8g2.drawBox(bx + 6, by + 14, fill_w, 6);
            }
        } else if (s_toast_ts > 0 && now - s_toast_ts < s_toast_dur) {
            u8g2.setFont(u8g2_font_6x10_tf);
            int tw = u8g2.getStrWidth(s_toast_msg);
            int tx = (SCREEN_W - tw) / 2;
            int ty = 20;
            u8g2.setDrawColor(0);
            u8g2.drawBox(tx - 4, ty - 10, tw + 8, 14);
            u8g2.setDrawColor(1);
            u8g2.drawFrame(tx - 4, ty - 10, tw + 8, 14);
            u8g2.drawStr(tx, ty, s_toast_msg);
        }

        u8g2.sendBuffer();
        return;
    }

    // 3. BLUETOOTH JUST CONNECTED SPLASH SCREEN (2.5s)
    if (s_audio_mode == AUDIO_MODE_BT && s_bt_connected && (now - s_conn_splash_ts < CONN_SPLASH_DUR)) {
        if (s_switching) return;
        s_last_cycle_ts = now;

        u8g2.clearBuffer();
        draw_bt_connected_screen(now);
        u8g2.sendBuffer();
        return;
    }

    // 4. ACTIVE AUDIO VISUALIZER (MIC mode OR BT connected with music): Auto-cycle only when actively visualizing
    if (s_auto_cycle && (now - s_last_cycle_ts > s_auto_cycle_interval)) {
        display_next_mode(false); // Don't show mode label when auto-cycling
        s_last_cycle_ts = now;
    }

    if (s_switching) return;        // skip render entirely while switching modes

    update_agc(left, right, n);
    audio_compute_bands(left, right, n, g_frame_bands); // pre-compute once per frame
    beat_detector_update(g_frame_bands);                // update BPM & beat state

    u8g2.clearBuffer();

    if (s_mode < MODE_COUNT && EFFECTS[s_mode].render) {
        EFFECTS[s_mode].render(left, right, n);
    }
    draw_mode_label();

    // Beat flash overlay: invert a thin border for EXACTLY 1 frame when beat fires.
    // s_beat_flashed guards against g_beat.beat_now staying true across multiple frames,
    // which would cause persistent rectangles around the screen corners.
    static bool s_beat_flashed = false;
    if (g_beat.beat_now && !s_beat_flashed && s_mode != MODE_BEAT_METER && g_beat.confidence > 0.5f) {
        u8g2.setDrawColor(2); // XOR mode — inverts pixels
        u8g2.drawFrame(0, 0, SCREEN_W, SCREEN_H);
        u8g2.drawFrame(1, 1, SCREEN_W - 2, SCREEN_H - 2);
        u8g2.setDrawColor(1); // restore normal mode
        s_beat_flashed = true;
    } else if (!g_beat.beat_now) {
        s_beat_flashed = false; // reset when beat ends, ready for next beat
    }

    // Volume popup overlay (takes priority when active)
    if (s_volume_ts > 0 && now - s_volume_ts < s_volume_dur) {
        u8g2.setFont(u8g2_font_6x10_tf);
        char vstr[16];
        int pct = (int)((float)s_display_vol * 100.0f / 127.0f + 0.5f);
        snprintf(vstr, sizeof(vstr), "VOL %d%%", pct);

        int box_w = 90;
        int box_h = 24;
        int bx = (SCREEN_W - box_w) / 2;
        int by = (SCREEN_H - box_h) / 2;

        u8g2.setDrawColor(0);
        u8g2.drawBox(bx, by, box_w, box_h);
        u8g2.setDrawColor(1);
        u8g2.drawFrame(bx, by, box_w, box_h);
        u8g2.drawStr(bx + (box_w - u8g2.getStrWidth(vstr)) / 2, by + 10, vstr);

        // Progress bar inside volume box
        int bar_w = box_w - 12;
        int fill_w = (bar_w * s_display_vol) / 127;
        u8g2.drawFrame(bx + 6, by + 14, bar_w, 6);
        if (fill_w > 0) {
            u8g2.drawBox(bx + 6, by + 14, fill_w, 6);
        }
    }
    // Toast notification banner
    else if (s_toast_ts > 0 && now - s_toast_ts < s_toast_dur) {
        u8g2.setFont(u8g2_font_6x10_tf);
        int tw = u8g2.getStrWidth(s_toast_msg);
        int tx = (SCREEN_W - tw) / 2;
        int ty = 20;
        u8g2.setDrawColor(0);
        u8g2.drawBox(tx - 4, ty - 10, tw + 8, 14);
        u8g2.setDrawColor(1);
        u8g2.drawFrame(tx - 4, ty - 10, tw + 8, 14);
        u8g2.drawStr(tx, ty, s_toast_msg);
    }

    u8g2.sendBuffer();
}

