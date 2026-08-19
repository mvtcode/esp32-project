#include "display.h"
#include "effects/effects.h"

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

    Serial.printf("[OLED] Init OK — HW_I2C %d kHz, %d modes\n", I2C_CLOCK / 1000, MODE_COUNT);
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

static bool s_is_bt_mode = false;
static bool s_bt_connected = false;
static bool s_bt_playing = false;

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

void display_set_audio_mode(bool is_bt, bool is_connected, bool is_playing) {
    s_is_bt_mode = is_bt;
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
    Serial.printf("[Mode] » %s\n", EFFECTS[m].name);
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
    u8g2.drawStr(50, 37, "MVT VU METER");

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
    if (s_auto_cycle && (millis() - s_last_cycle_ts > s_auto_cycle_interval)) {
        display_next_mode(false); // Don't show mode label when auto-cycling
        s_last_cycle_ts = millis();
    }

    if (s_switching) return;        // skip render entirely while switching modes

    update_agc(left, right, n);
    audio_compute_bands(left, right, n, g_frame_bands); // pre-compute once per frame

    u8g2.clearBuffer();

    // 1. If in Bluetooth mode and waiting for device connection -> Show Pairing Standby Screen
    if (s_is_bt_mode && !s_bt_connected) {
        draw_bt_pairing_screen(millis());
    } else {
        // 2. Active visualizer (MIC mode OR BT connected with music)
        if (s_mode < MODE_COUNT && EFFECTS[s_mode].render) {
            EFFECTS[s_mode].render(left, right, n);
        }
        draw_mode_label();
    }

    uint32_t now = millis();

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

