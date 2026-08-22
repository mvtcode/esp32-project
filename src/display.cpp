#include "display.h"
#include "effects/effects.h"
#include "effects/safe_draw.h"
#include "wifi_app.h"
#include "lunar_calendar.h"
#include "log.h"
#include "beat_detector.h"
#include "sd_card.h"
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
// Mode management & Global UI state
// -----------------------------------------------------------------------
static DisplayMode s_mode          = MODE_WAVEFORM;
static uint32_t   s_label_ts       = 0;
static const uint32_t LABEL_DUR_MS = 1500;
static volatile bool  s_switching  = false; // Guard: prevent render during mode switch

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

// -----------------------------------------------------------------------
// MP3 Player UI State & Screens (s1: Normal, s2: Visualizer, s3: Playlist)
// -----------------------------------------------------------------------
static Mp3Screen s_mp3_screen         = MP3_SCREEN_NORMAL;
static int       s_mp3_selected_idx   = 0;
static char      s_mp3_current_title[96] = "";
static int       s_mp3_current_idx    = 0;
static int       s_mp3_total_tracks   = 0;
static bool      s_mp3_playing        = false;
static bool      s_mp3_paused         = false;
static uint32_t  s_mp3_cur_sec        = 0;
static uint32_t  s_mp3_tot_sec        = 0;
static uint8_t   s_mp3_pct            = 0;

static int s_scroll_accum = 0;

void display_set_mp3_screen(Mp3Screen screen) {
    s_mp3_screen = screen;
    s_scroll_accum = 0;
}

Mp3Screen display_get_mp3_screen() {
    return s_mp3_screen;
}

void display_mp3_playlist_scroll(int delta) {
    int total = sd_card_get_track_count();
    if (total <= 0) return;

    s_scroll_accum += delta;
    int step = s_scroll_accum / 4;
    if (step != 0) {
        s_scroll_accum %= 4;
        s_mp3_selected_idx += step;
        if (s_mp3_selected_idx < 0) s_mp3_selected_idx = 0;
        if (s_mp3_selected_idx >= total) s_mp3_selected_idx = total - 1;
    }
}

int display_mp3_playlist_get_focus() {
    return s_mp3_selected_idx;
}

void display_mp3_playlist_set_focus(int index) {
    s_mp3_selected_idx = index;
}

void display_set_mp3_status(bool is_playing, bool is_paused, const char *title, int index, int total, uint32_t cur_sec, uint32_t tot_sec, uint8_t pct, uint8_t volume) {
    s_mp3_playing = is_playing;
    s_mp3_paused = is_paused;
    if (title) {
        strncpy(s_mp3_current_title, title, sizeof(s_mp3_current_title) - 1);
        s_mp3_current_title[sizeof(s_mp3_current_title) - 1] = '\0';
    }
    s_mp3_current_idx = index;
    s_mp3_total_tracks = total;
    s_mp3_cur_sec = cur_sec;
    s_mp3_tot_sec = tot_sec;
    s_mp3_pct = pct;
    s_display_vol = volume > 127 ? 127 : volume;
}

// -----------------------------------------------------------------------
// Screen s1: Normal Player Screen (Marquee Title, Progress, Track #, Vol)
// -----------------------------------------------------------------------
static void draw_mp3_normal_screen() {
    int total = sd_card_get_track_count();
    if (total == 0) {
        gfx.setFont(u8g2_font_6x10_tf);
        const char *msg = "NO MP3 FILES";
        gfx.drawStr((SCREEN_W - gfx.getStrWidth(msg)) / 2, 36, msg);
        return;
    }

    // 1. Header Line: Status (PLAY/PAUSE) on Left, Track index on Right
    gfx.setFont(u8g2_font_5x8_tr);
    const char *status_tag = s_mp3_paused ? "|| PAUSED" : "> PLAYING";
    gfx.drawStr(2, 9, status_tag);

    char idx_str[16];
    snprintf(idx_str, sizeof(idx_str), "[%02d/%02d]", s_mp3_current_idx + 1, total);
    int iw = gfx.getStrWidth(idx_str);
    gfx.drawStr(SCREEN_W - iw - 2, 9, idx_str);

    gfx.drawHLine(2, 12, SCREEN_W - 4);

    // 2. Track Title with Smooth Marquee auto-scrolling
    gfx.setFont(u8g2_font_6x10_tf);
    const char *title = s_mp3_current_title[0] ? s_mp3_current_title : "Unknown Track";
    int str_w = gfx.getStrWidth(title);
    int max_w = SCREEN_W - 8;

    if (str_w <= max_w) {
        // Centered if short enough
        gfx.drawStr((SCREEN_W - str_w) / 2, 26, title);
    } else {
        // Marquee scrolling:
        // Cycle 1: Start at x=4 (first characters visible), pause ~1.8s, scroll left until text exits.
        // Cycle 2+: Text smoothly enters from the right edge (x=SCREEN_W) and scrolls across!
        static uint32_t s_mq_ts = 0;
        static int s_mq_x = 4;
        static int s_mq_pause = 50; // initial pause (~1.8s)
        static int s_mq_last_idx = -1;

        // Reset to Cycle 1 on track change
        if (s_mp3_current_idx != s_mq_last_idx) {
            s_mq_last_idx = s_mp3_current_idx;
            s_mq_x = 4;
            s_mq_pause = 50;
        }

        uint32_t now = millis();
        if (now - s_mq_ts >= 35) {
            s_mq_ts = now;
            if (s_mq_pause > 0) {
                s_mq_pause--;
            } else {
                s_mq_x--;
                // When entire string has exited the screen on the left, start next cycle from right edge!
                if (s_mq_x < -str_w) {
                    s_mq_x = SCREEN_W;
                }
            }
        }
        
        // Clip to title display region to prevent artifacts
        u8g2.setClipWindow(2, 13, SCREEN_W - 2, 29);
        gfx.drawStr(s_mq_x, 26, title);
        u8g2.setMaxClipWindow();
    }

    // 3. Time & Progress Bar
    char tstr[32];
    if (s_mp3_tot_sec >= 3600 || s_mp3_cur_sec >= 3600) {
        // Hour format: HH:MM:SS / HH:MM:SS for tracks > 60 minutes
        int cur_h = (int)(s_mp3_cur_sec / 3600);
        int cur_m = (int)((s_mp3_cur_sec % 3600) / 60);
        int cur_s = (int)(s_mp3_cur_sec % 60);

        if (s_mp3_tot_sec > 0) {
            int tot_h = (int)(s_mp3_tot_sec / 3600);
            int tot_m = (int)((s_mp3_tot_sec % 3600) / 60);
            int tot_s = (int)(s_mp3_tot_sec % 60);
            snprintf(tstr, sizeof(tstr), "%02d:%02d:%02d/%02d:%02d:%02d", 
                     cur_h, cur_m, cur_s, tot_h, tot_m, tot_s);
        } else {
            snprintf(tstr, sizeof(tstr), "%02d:%02d:%02d/--:--:--", 
                     cur_h, cur_m, cur_s);
        }
    } else {
        // Standard minute format: MM:SS / MM:SS
        if (s_mp3_tot_sec > 0) {
            snprintf(tstr, sizeof(tstr), "%02d:%02d / %02d:%02d", 
                     (int)(s_mp3_cur_sec / 60), (int)(s_mp3_cur_sec % 60),
                     (int)(s_mp3_tot_sec / 60), (int)(s_mp3_tot_sec % 60));
        } else {
            snprintf(tstr, sizeof(tstr), "%02d:%02d / --:--", 
                     (int)(s_mp3_cur_sec / 60), (int)(s_mp3_cur_sec % 60));
        }
    }
    gfx.setFont(u8g2_font_5x8_tr);
    int tw = gfx.getStrWidth(tstr);
    gfx.drawStr((SCREEN_W - tw) / 2, 38, tstr);

    // Progress Bar Frame & Fill
    int p_x = 4;
    int p_y = 42;
    int p_w = SCREEN_W - 8;
    gfx.drawFrame(p_x, p_y, p_w, 4);
    if (s_mp3_pct > 0) {
        int fill = (p_w - 2) * s_mp3_pct / 100;
        if (fill > 0) gfx.drawBox(p_x + 1, p_y + 1, fill, 2);
    }

    // 4. Bottom Footer: Volume Value & Level Bar
    gfx.drawHLine(4, 50, SCREEN_W - 8);
    gfx.setFont(u8g2_font_5x8_tr);
    int vol_pct = (int)((float)s_display_vol * 100.0f / 127.0f + 0.5f);
    char vstr[16];
    snprintf(vstr, sizeof(vstr), "VOL: %d%%", vol_pct);
    gfx.drawStr(4, 60, vstr);

    // Mini volume bar on the right side
    int vb_x = 64;
    int vb_y = 54;
    int vb_w = SCREEN_W - vb_x - 4;
    gfx.drawFrame(vb_x, vb_y, vb_w, 6);
    int vfill = (vb_w - 2) * s_display_vol / 127;
    if (vfill > 0) gfx.drawBox(vb_x + 1, vb_y + 1, vfill, 4);
}

// -----------------------------------------------------------------------
// Screen s3: Playlist Menu Screen (Focus indicator & Play Icon)
// -----------------------------------------------------------------------
static void draw_mp3_playlist_screen() {
    int total = sd_card_get_track_count();
    if (total == 0) {
        gfx.setFont(u8g2_font_6x10_tf);
        const char *msg = "NO MP3 FILES";
        gfx.drawStr((SCREEN_W - gfx.getStrWidth(msg)) / 2, 36, msg);
        return;
    }

    if (s_mp3_selected_idx < 0) s_mp3_selected_idx = 0;
    if (s_mp3_selected_idx >= total) s_mp3_selected_idx = total - 1;

    // 1. Header Banner
    gfx.drawBox(0, 0, SCREEN_W, 11);
    gfx.setDrawColor(0);
    gfx.setFont(u8g2_font_5x8_tr);
    char hdr[32];
    snprintf(hdr, sizeof(hdr), "PLAYLIST [%d/%d]", s_mp3_selected_idx + 1, total);
    gfx.drawStr(3, 9, hdr);
    gfx.setDrawColor(1);

    // 2. Viewport calculation (4 visible lines)
    const int VISIBLE_ITEMS = 4;
    const int LINE_H = 12;
    const int START_Y = 22;

    int top_idx = s_mp3_selected_idx - (VISIBLE_ITEMS / 2);
    if (top_idx < 0) top_idx = 0;
    if (top_idx > total - VISIBLE_ITEMS) top_idx = total - VISIBLE_ITEMS;
    if (top_idx < 0) top_idx = 0;

    for (int i = 0; i < VISIBLE_ITEMS && (top_idx + i) < total; i++) {
        int item_idx = top_idx + i;
        const PlaylistItem *track = sd_card_get_track(item_idx);
        if (!track) continue;

        int y = START_Y + i * LINE_H;
        bool is_selected = (item_idx == s_mp3_selected_idx);
        bool is_currently_playing = (item_idx == s_mp3_current_idx);

        if (is_selected) {
            gfx.drawRBox(0, y - 9, SCREEN_W - 6, LINE_H - 1, 1);
            gfx.setDrawColor(0);
        }

        char line_str[64];
        const char *play_icon = is_currently_playing ? ">" : " ";
        snprintf(line_str, sizeof(line_str), "%s%d. %s", play_icon, item_idx + 1, track->title);
        gfx.setFont(u8g2_font_5x8_tr);
        gfx.drawStr(3, y - 1, line_str);

        if (is_selected) {
            gfx.setDrawColor(1);
        }
    }

    // 3. Right vertical scrollbar
    int sb_x = SCREEN_W - 3;
    gfx.drawVLine(sb_x, 13, SCREEN_H - 14);
    if (total > VISIBLE_ITEMS) {
        int avail_h = SCREEN_H - 14;
        int bar_h = (VISIBLE_ITEMS * avail_h) / total;
        if (bar_h < 4) bar_h = 4;
        int bar_y = 13 + (s_mp3_selected_idx * (avail_h - bar_h)) / (total - 1);
        gfx.drawBox(sb_x - 1, bar_y, 3, bar_h);
    }
}

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

    gfx.setFont(u8g2_font_6x10_tf);
    const char *name = EFFECTS[s_mode].name;
    int str_w = gfx.getStrWidth(name);
    int x = (SCREEN_W - str_w) / 2;
    int y = 38;

    // Clear background box (white on black → invert region)
    gfx.setDrawColor(0);
    gfx.drawBox(x - 5, y - 12, str_w + 10, 14);
    gfx.setDrawColor(1);
    gfx.drawFrame(x - 5, y - 12, str_w + 10, 14);
    gfx.drawStr(x, y, name);
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

    gfx.clearBuffer();
    gfx.sendBuffer();

    LOG_I("OLED", "Init OK - HW_I2C %d kHz, %d modes", I2C_CLOCK / 1000, MODE_COUNT);
}

void display_error(const char *msg) {
    gfx.clearBuffer();
    gfx.setFont(u8g2_font_6x10_tf);
    gfx.drawStr(2, 20, "!! ERROR !!");
    gfx.drawStr(2, 38, msg);
    gfx.drawFrame(0, 0, SCREEN_W, SCREEN_H);
    gfx.sendBuffer();
}

void display_toast(const char *msg, uint32_t duration_ms) {
    if (!msg) return;
    strncpy(s_toast_msg, msg, sizeof(s_toast_msg) - 1);
    s_toast_msg[sizeof(s_toast_msg) - 1] = '\0';
    s_toast_dur = duration_ms;
    s_toast_ts = millis();
}

static void draw_hourglass_icon(int hx, int hy, uint8_t percent) {
    // 1. Top and Bottom Brass Plates
    gfx.drawHLine(hx - 7, hy - 10, 15);
    gfx.drawHLine(hx - 6, hy - 9, 13);
    gfx.drawHLine(hx - 7, hy + 10, 15);
    gfx.drawHLine(hx - 6, hy + 9, 13);

    // 2. Glass Bulb Walls (Funnel Contour)
    gfx.drawLine(hx - 6, hy - 8, hx - 1, hy);
    gfx.drawLine(hx + 6, hy - 8, hx + 1, hy);
    gfx.drawLine(hx - 1, hy, hx - 6, hy + 8);
    gfx.drawLine(hx + 1, hy, hx + 6, hy + 8);

    // 3. Sand inside Top Bulb (decreases as percent increases)
    int top_sand_h = ((100 - percent) * 6) / 100;
    for (int dy = 0; dy < top_sand_h; dy++) {
        int y = hy - 3 - dy;
        int w = 3 + dy; // narrower at neck, wider towards top
        gfx.drawHLine(hx - w / 2, y, w);
    }

    // 4. Animated Falling Sand Grains
    if (percent < 100) {
        static uint8_t s_drop_tick = 0;
        s_drop_tick = (s_drop_tick + 1) % 3;
        gfx.drawPixel(hx, hy);
        gfx.drawPixel(hx, hy + 2 + s_drop_tick);
    }

    // 5. Sand accumulated inside Bottom Bulb (increases as percent increases)
    int btm_sand_h = (percent * 6) / 100;
    for (int dy = 0; dy < btm_sand_h; dy++) {
        int y = hy + 8 - dy;
        int w = 7 - dy; // wider at base, narrower at crest
        if (w < 1) w = 1;
        gfx.drawHLine(hx - w / 2, y, w);
    }
}

void display_show_loading(const char *title, const char *detail, uint8_t percent) {
    if (percent > 100) percent = 100;

    gfx.clearBuffer();

    // 1. Top Header Boxed Banner
    gfx.drawRBox(0, 0, SCREEN_W, 12, 2);
    gfx.setDrawColor(0); // Inverted text on black banner
    gfx.setFont(u8g2_font_6x10_tf);
    const char *hdr = "» CHUYEN CHE DO «";
    gfx.drawStr((SCREEN_W - gfx.getStrWidth(hdr)) / 2, 9, hdr);
    gfx.setDrawColor(1);

    // 2. Animated Hourglass Icon (Left Side: X=18, Y=28)
    draw_hourglass_icon(18, 28, percent);

    // 3. Mode Title (Right Side: X=36, Y=24)
    if (title && title[0] != '\0') {
        gfx.setFont(u8g2_font_6x10_tf);
        gfx.drawStr(36, 24, title);
    }

    // 4. Detail Step Status (Right Side: X=36, Y=37)
    if (detail && detail[0] != '\0') {
        gfx.setFont(u8g2_font_5x8_tr);
        gfx.drawStr(36, 37, detail);
    }

    // 5. High-Tech Rounded Progress Bar (Bottom Left: X=8, Y=47, W=86, H=7)
    int bar_x = 8;
    int bar_y = 47;
    int bar_w = 86;
    int bar_h = 7;
    gfx.drawFrame(bar_x, bar_y, bar_w, bar_h);
    int fill_w = ((bar_w - 4) * percent) / 100;
    if (fill_w > 0) {
        gfx.drawBox(bar_x + 2, bar_y + 2, fill_w, bar_h - 4);
    }

    // 6. Percentage Text with Animated Orbiting Spinner (Bottom Right: X=98..124, Y=54)
    char pct_str[8];
    snprintf(pct_str, sizeof(pct_str), "%d%%", (int)percent);
    gfx.setFont(u8g2_font_04b_03_tr);
    gfx.drawStr(98, 54, pct_str);

    // Micro rotating spinner dot on bottom right
    int sx = 120;
    int sy = 50;
    static uint8_t s_spin_phase = 0;
    s_spin_phase = (s_spin_phase + 1) % 4;
    gfx.drawCircle(sx, sy, 3, U8G2_DRAW_ALL);
    if (s_spin_phase == 0) gfx.drawPixel(sx, sy - 2);
    else if (s_spin_phase == 1) gfx.drawPixel(sx + 2, sy);
    else if (s_spin_phase == 2) gfx.drawPixel(sx, sy + 2);
    else gfx.drawPixel(sx - 2, sy);

    gfx.sendBuffer();
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
    gfx.drawRBox(0, 0, SCREEN_W, 12, 2);
    gfx.setDrawColor(0); // Inverted text on header
    gfx.setFont(u8g2_font_6x10_tf);
    const char *title = "CAU HINH WIFI AP";
    gfx.drawStr((SCREEN_W - gfx.getStrWidth(title)) / 2, 9, title);
    gfx.setDrawColor(1); // Normal drawing color

    // 2. AP SSID
    gfx.setFont(u8g2_font_5x8_tr);
    gfx.drawStr(4, 23, "1. Ket noi WiFi:");
    gfx.setFont(u8g2_font_6x10_tf);
    gfx.drawStr(12, 34, AP_SSID_NAME);

    // 3. Web URL / IP
    gfx.setFont(u8g2_font_5x8_tr);
    gfx.drawStr(4, 46, "2. Mo trinh duyet vao:");
    gfx.setFont(u8g2_font_6x10_tf);
    gfx.drawStr(12, 58, "http://192.168.4.1");

    // 4. Subtle Animated Signal Icon on right
    uint32_t step = (millis() / 250) % 3;
    int rx = 114;
    int ry = 30;
    gfx.drawCircle(rx, ry, 2, U8G2_DRAW_ALL);
    if (step >= 1) gfx.drawCircle(rx, ry, 5, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT);
    if (step >= 2) gfx.drawCircle(rx, ry, 8, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT);
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

    gfx.setFont(u8g2_font_5x8_tr);
    gfx.drawStr(2, 9, date_str);

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
                gfx.drawVLine(bx, 10 - bh, bh);
                gfx.drawVLine(bx + 1, 10 - bh, bh);
            } else {
                gfx.drawPixel(bx, 9);
                gfx.drawPixel(bx + 1, 9);
            }
        }
    } else if (wifi_app_is_connecting()) {
        // Smooth animated 5-bar scan/wave loading effect
        uint32_t step = (millis() / 150) % 7;
        for (int b = 0; b < 5; b++) {
            int bx = 112 + b * 3;
            int bh = 2 + b * 2;
            if (b <= (int)step && step < 5) {
                gfx.drawVLine(bx, 10 - bh, bh);
                gfx.drawVLine(bx + 1, 10 - bh, bh);
            } else {
                gfx.drawPixel(bx, 9);
                gfx.drawPixel(bx + 1, 9);
            }
        }
    } else {
        gfx.setFont(u8g2_font_5x8_tr);
        gfx.drawStr(108, 9, "OFF");
    }

    gfx.drawHLine(2, 12, SCREEN_W - 4);

    // 2. Digital Time (large centered font)
    gfx.setFont(u8g2_font_courB18_tf);
    int tw = gfx.getStrWidth(time_str);
    gfx.drawStr((SCREEN_W - tw) / 2, 33, time_str);

    // 3. Weather & Temperature / Humidity
    gfx.setFont(u8g2_font_6x10_tf);
    char wstr[32];
    snprintf(wstr, sizeof(wstr), "%.1f C   %d%% Hum", w.temp_c, w.humidity);
    gfx.drawStr((SCREEN_W - gfx.getStrWidth(wstr)) / 2, 47, wstr);

    // 4. Bottom Footer: Lunar Date (Ngày Âm Lịch)
    gfx.drawHLine(4, 52, SCREEN_W - 8);

    char lunar_str[48];
    if (sd.valid) {
        int ld, lm, ly;
        solarToLunar(sd.year, sd.month, sd.day, ld, lm, ly);
        const char *year_name = getLunarYearName(ly);
        snprintf(lunar_str, sizeof(lunar_str), "AL: %02d/%02d (%s)", ld, lm, year_name);
    } else {
        snprintf(lunar_str, sizeof(lunar_str), "Am Lich: Dang tai...");
    }

    gfx.setFont(u8g2_font_6x10_tf);
    int lw = gfx.getStrWidth(lunar_str);
    gfx.drawStr((SCREEN_W - lw) / 2, 62, lunar_str);
}

// -----------------------------------------------------------------------
// Bluetooth Connected Success Screen
// -----------------------------------------------------------------------
static void draw_bt_connected_screen(uint32_t /*now*/) {
    // Outer border
    gfx.drawFrame(2, 2, SCREEN_W - 4, SCREEN_H - 4);
    gfx.drawFrame(4, 4, SCREEN_W - 8, SCREEN_H - 8);

    // Title
    gfx.setFont(u8g2_font_6x10_tf);
    const char *hdr = "[ BLUETOOTH ]";
    int hw = gfx.getStrWidth(hdr);
    gfx.drawStr((SCREEN_W - hw) / 2, 16, hdr);
    gfx.drawHLine(10, 19, SCREEN_W - 20);

    // Checkmark circle (X=26, Y=36)
    int cx = 26, cy = 36;
    gfx.drawCircle(cx, cy, 10);
    // Draw Checkmark
    gfx.drawLine(cx - 5, cy, cx - 1, cy + 4);
    gfx.drawLine(cx - 1, cy + 4, cx + 5, cy - 4);
    gfx.drawLine(cx - 5, cy + 1, cx - 1, cy + 5);
    gfx.drawLine(cx - 1, cy + 5, cx + 5, cy - 3);

    // Text info
    gfx.setFont(u8g2_font_7x14B_tf);
    gfx.drawStr(44, 34, "CONNECTED!");

    gfx.setFont(u8g2_font_5x8_tr);
    gfx.drawStr(44, 46, "Audio Stream Ready");

    // Bottom subtitle
    gfx.setFont(u8g2_font_04b_03_tr);
    const char *sub = "MVT VU METER ACTIVE";
    gfx.drawStr((SCREEN_W - gfx.getStrWidth(sub)) / 2, 57, sub);
}

// -----------------------------------------------------------------------
// Bluetooth Pairing & Waiting Standby Screen
// -----------------------------------------------------------------------
static void draw_bt_pairing_screen(uint32_t now) {
    // 1. Title bar
    gfx.setFont(u8g2_font_6x10_tf);
    const char *title = "[BLUETOOTH MODE]";
    int tw = gfx.getStrWidth(title);
    gfx.drawStr((SCREEN_W - tw) / 2, 10, title);
    gfx.drawHLine(4, 13, SCREEN_W - 8);

    // 2. Animated Bluetooth Icon (X=24, Y=33)
    int bx = 24;
    int by = 33;
    // Central stem
    gfx.drawLine(bx, by - 12, bx, by + 12);
    gfx.drawLine(bx + 1, by - 12, bx + 1, by + 12);
    // Upper & Lower wings
    gfx.drawLine(bx, by - 12, bx + 7, by - 5);
    gfx.drawLine(bx + 7, by - 5, bx - 6, by + 4);
    gfx.drawLine(bx - 6, by - 4, bx + 7, by + 5);
    gfx.drawLine(bx + 7, by + 5, bx, by + 12);

    // Animated radiating radar waves around icon
    int phase = (now / 300) % 3;
    for (int i = 0; i < 3; i++) {
        int r = 11 + i * 5;
        if (i <= phase) {
            gfx.drawCircle(bx, by, r, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT);
            gfx.drawCircle(bx, by, r, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_LOWER_LEFT);
        }
    }

    // 3. Connection info
    gfx.setFont(u8g2_font_5x8_tr);
    gfx.drawStr(50, 26, "Pair Name:");
    gfx.setFont(u8g2_font_6x10_tf);
    gfx.drawStr(50, 37, "MVT-Audio");

    // 4. Animated Status line
    static const char *states[] = {
        "Status: Waiting   ",
        "Status: Waiting . ",
        "Status: Waiting ..",
        "Status: Ready >>> "
    };
    int s_idx = (now / 400) % 4;
    gfx.setFont(u8g2_font_5x8_tr);
    gfx.drawStr(50, 48, states[s_idx]);

    // 5. Bottom hint
    gfx.drawHLine(4, 53, SCREEN_W - 8);
    gfx.setFont(u8g2_font_04b_03_tr);
    const char *hint = "DISCOVERABLE  -  CONNECT PHONE";
    gfx.drawStr((SCREEN_W - gfx.getStrWidth(hint)) / 2, 62, hint);
}

void display_draw_waveform(const int32_t *left, const int32_t *right, size_t n) {
    uint32_t now = millis();

    // 1. DEDICATED CLOCK & WEATHER MODE (Mode 2)
    if (s_audio_mode == AUDIO_MODE_CLOCK) {
        if (s_switching) return;
        s_last_cycle_ts = now; // Freeze visualizer auto-cycle

        gfx.clearBuffer();
        draw_clock_weather_screen(left, right, n);

        // Volume popup overlay (takes priority when active)
        if (s_volume_ts > 0 && now - s_volume_ts < s_volume_dur) {
            gfx.setFont(u8g2_font_6x10_tf);
            char vstr[16];
            int pct = (int)((float)s_display_vol * 100.0f / 127.0f + 0.5f);
            snprintf(vstr, sizeof(vstr), "VOL: %d%%", pct);

            int box_w = 90;
            int box_h = 24;
            int bx = (SCREEN_W - box_w) / 2;
            int by = (SCREEN_H - box_h) / 2;

            gfx.setDrawColor(0);
            gfx.drawBox(bx, by, box_w, box_h);
            gfx.setDrawColor(1);
            gfx.drawFrame(bx, by, box_w, box_h);
            gfx.drawStr(bx + (box_w - gfx.getStrWidth(vstr)) / 2, by + 10, vstr);

            int bar_w = box_w - 12;
            int fill_w = (bar_w * s_display_vol) / 127;
            gfx.drawFrame(bx + 6, by + 14, bar_w, 6);
            if (fill_w > 0) {
                gfx.drawBox(bx + 6, by + 14, fill_w, 6);
            }
        } else if (s_toast_ts > 0 && now - s_toast_ts < s_toast_dur) {
            gfx.setFont(u8g2_font_6x10_tf);
            int tw = gfx.getStrWidth(s_toast_msg);
            int tx = (SCREEN_W - tw) / 2;
            int ty = 20;
            gfx.setDrawColor(0);
            gfx.drawBox(tx - 4, ty - 10, tw + 8, 14);
            gfx.setDrawColor(1);
            gfx.drawFrame(tx - 4, ty - 10, tw + 8, 14);
            gfx.drawStr(tx, ty, s_toast_msg);
        }

        gfx.sendBuffer();
        return;
    }

    // 2. BLUETOOTH PAIRING STANDBY SCREEN (When in BT mode and waiting for device connection)
    if (s_audio_mode == AUDIO_MODE_BT && !s_bt_connected) {
        if (s_switching) return;
        s_last_cycle_ts = now; // Freeze auto-cycle timer while waiting

        gfx.clearBuffer();
        draw_bt_pairing_screen(now);

        // Allow Volume popup or Toast over Pairing screen if triggered
        if (s_volume_ts > 0 && now - s_volume_ts < s_volume_dur) {
            gfx.setFont(u8g2_font_6x10_tf);
            char vstr[16];
            int pct = (int)((float)s_display_vol * 100.0f / 127.0f + 0.5f);
            snprintf(vstr, sizeof(vstr), "VOL: %d%%", pct);

            int box_w = 90;
            int box_h = 24;
            int bx = (SCREEN_W - box_w) / 2;
            int by = (SCREEN_H - box_h) / 2;

            gfx.setDrawColor(0);
            gfx.drawBox(bx, by, box_w, box_h);
            gfx.setDrawColor(1);
            gfx.drawFrame(bx, by, box_w, box_h);
            gfx.drawStr(bx + (box_w - gfx.getStrWidth(vstr)) / 2, by + 10, vstr);

            int bar_w = box_w - 12;
            int fill_w = (bar_w * s_display_vol) / 127;
            gfx.drawFrame(bx + 6, by + 14, bar_w, 6);
            if (fill_w > 0) {
                gfx.drawBox(bx + 6, by + 14, fill_w, 6);
            }
        } else if (s_toast_ts > 0 && now - s_toast_ts < s_toast_dur) {
            gfx.setFont(u8g2_font_6x10_tf);
            int tw = gfx.getStrWidth(s_toast_msg);
            int tx = (SCREEN_W - tw) / 2;
            int ty = 20;
            gfx.setDrawColor(0);
            gfx.drawBox(tx - 4, ty - 10, tw + 8, 14);
            gfx.setDrawColor(1);
            gfx.drawFrame(tx - 4, ty - 10, tw + 8, 14);
            gfx.drawStr(tx, ty, s_toast_msg);
        }

        gfx.sendBuffer();
        return;
    }

    // 3. BLUETOOTH JUST CONNECTED SPLASH SCREEN (2.5s)
    if (s_audio_mode == AUDIO_MODE_BT && s_bt_connected && (now - s_conn_splash_ts < CONN_SPLASH_DUR)) {
        if (s_switching) return;
        s_last_cycle_ts = now;

        gfx.clearBuffer();
        draw_bt_connected_screen(now);
        gfx.sendBuffer();
        return;
    }

    // 4. MP3 PLAYER SCREENS (s1: Normal, s3: Playlist)
    if (s_audio_mode == AUDIO_MODE_SD_MP3 && s_mp3_screen == MP3_SCREEN_PLAYLIST) {
        if (s_switching) return;
        s_last_cycle_ts = now; // Freeze auto-cycle timer in menu

        gfx.clearBuffer();
        draw_mp3_playlist_screen();

        // Toast over Menu if triggered
        if (s_toast_ts > 0 && now - s_toast_ts < s_toast_dur) {
            gfx.setFont(u8g2_font_6x10_tf);
            int tw = gfx.getStrWidth(s_toast_msg);
            int tx = (SCREEN_W - tw) / 2;
            int ty = 20;
            gfx.setDrawColor(0);
            gfx.drawBox(tx - 4, ty - 10, tw + 8, 14);
            gfx.setDrawColor(1);
            gfx.drawFrame(tx - 4, ty - 10, tw + 8, 14);
            gfx.drawStr(tx, ty, s_toast_msg);
        }

        gfx.sendBuffer();
        return;
    }

    if (s_audio_mode == AUDIO_MODE_SD_MP3 && s_mp3_screen == MP3_SCREEN_NORMAL) {
        if (s_switching) return;
        s_last_cycle_ts = now; // Freeze visualizer auto-cycle in normal player view

        gfx.clearBuffer();
        draw_mp3_normal_screen();

        // Volume popup overlay (takes priority when active)
        if (s_volume_ts > 0 && now - s_volume_ts < s_volume_dur) {
            gfx.setFont(u8g2_font_6x10_tf);
            char vstr[16];
            int pct = (int)((float)s_display_vol * 100.0f / 127.0f + 0.5f);
            snprintf(vstr, sizeof(vstr), "VOL: %d%%", pct);

            int box_w = 90;
            int box_h = 24;
            int bx = (SCREEN_W - box_w) / 2;
            int by = (SCREEN_H - box_h) / 2;

            gfx.setDrawColor(0);
            gfx.drawBox(bx, by, box_w, box_h);
            gfx.setDrawColor(1);
            gfx.drawFrame(bx, by, box_w, box_h);
            gfx.drawStr(bx + (box_w - gfx.getStrWidth(vstr)) / 2, by + 10, vstr);

            int bar_w = box_w - 12;
            int fill_w = (bar_w * s_display_vol) / 127;
            gfx.drawFrame(bx + 6, by + 14, bar_w, 6);
            if (fill_w > 0) {
                gfx.drawBox(bx + 6, by + 14, fill_w, 6);
            }
        } else if (s_toast_ts > 0 && now - s_toast_ts < s_toast_dur) {
            gfx.setFont(u8g2_font_6x10_tf);
            int tw = gfx.getStrWidth(s_toast_msg);
            int tx = (SCREEN_W - tw) / 2;
            int ty = 20;
            gfx.setDrawColor(0);
            gfx.drawBox(tx - 4, ty - 10, tw + 8, 14);
            gfx.setDrawColor(1);
            gfx.drawFrame(tx - 4, ty - 10, tw + 8, 14);
            gfx.drawStr(tx, ty, s_toast_msg);
        }

        gfx.sendBuffer();
        return;
    }

    // 5. ACTIVE AUDIO VISUALIZER (MIC / BT / MP3 s2 Visualizer): Auto-cycle only when actively visualizing
    if (s_auto_cycle && (now - s_last_cycle_ts > s_auto_cycle_interval)) {
        display_next_mode(false); // Don't show mode label when auto-cycling
        s_last_cycle_ts = now;
    }

    if (s_switching) return;        // skip render entirely while switching modes

    update_agc(left, right, n);
    audio_compute_bands(left, right, n, g_frame_bands); // pre-compute once per frame
    beat_detector_update(g_frame_bands);                // update BPM & beat state

    gfx.clearBuffer();

    if (s_mode < MODE_COUNT && EFFECTS[s_mode].render) {
        EFFECTS[s_mode].render(left, right, n);
    }

    draw_mode_label();

    // Beat flash overlay: invert a thin border for EXACTLY 1 frame when beat fires.
    // s_beat_flashed guards against g_beat.beat_now staying true across multiple frames,
    // which would cause persistent rectangles around the screen corners.
    static bool s_beat_flashed = false;
    if (g_beat.beat_now && !s_beat_flashed && s_mode != MODE_BEAT_METER && g_beat.confidence > 0.5f) {
        gfx.setDrawColor(2); // XOR mode — inverts pixels
        gfx.drawFrame(0, 0, SCREEN_W, SCREEN_H);
        gfx.drawFrame(1, 1, SCREEN_W - 2, SCREEN_H - 2);
        gfx.setDrawColor(1); // restore normal mode
        s_beat_flashed = true;
    } else if (!g_beat.beat_now) {
        s_beat_flashed = false; // reset when beat ends, ready for next beat
    }

    // Volume popup overlay (takes priority when active)
    if (s_volume_ts > 0 && now - s_volume_ts < s_volume_dur) {
        gfx.setFont(u8g2_font_6x10_tf);
        char vstr[16];
        int pct = (int)((float)s_display_vol * 100.0f / 127.0f + 0.5f);
        snprintf(vstr, sizeof(vstr), "VOL %d%%", pct);

        int box_w = 90;
        int box_h = 24;
        int bx = (SCREEN_W - box_w) / 2;
        int by = (SCREEN_H - box_h) / 2;

        gfx.setDrawColor(0);
        gfx.drawBox(bx, by, box_w, box_h);
        gfx.setDrawColor(1);
        gfx.drawFrame(bx, by, box_w, box_h);
        gfx.drawStr(bx + (box_w - gfx.getStrWidth(vstr)) / 2, by + 10, vstr);

        // Progress bar inside volume box
        int bar_w = box_w - 12;
        int fill_w = (bar_w * s_display_vol) / 127;
        gfx.drawFrame(bx + 6, by + 14, bar_w, 6);
        if (fill_w > 0) {
            gfx.drawBox(bx + 6, by + 14, fill_w, 6);
        }
    }
    // Toast notification banner
    else if (s_toast_ts > 0 && now - s_toast_ts < s_toast_dur) {
        gfx.setFont(u8g2_font_6x10_tf);
        int tw = gfx.getStrWidth(s_toast_msg);
        int tx = (SCREEN_W - tw) / 2;
        int ty = 20;
        gfx.setDrawColor(0);
        gfx.drawBox(tx - 4, ty - 10, tw + 8, 14);
        gfx.setDrawColor(1);
        gfx.drawFrame(tx - 4, ty - 10, tw + 8, 14);
        gfx.drawStr(tx, ty, s_toast_msg);
    }

    gfx.sendBuffer();
}

