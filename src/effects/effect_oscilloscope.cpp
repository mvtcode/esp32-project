#include "effects.h"
#include "../beat_detector.h"

// -----------------------------------------------------------------------
// MODE 66 — OSCILLOSCOPE
// Classic dual-channel triggered oscilloscope display.
// - Trigger: rising zero-crossing on left channel (prevents waveform drift)
// - Grid: 4×2 cell dotted graticule (like a real oscilloscope screen)
// - Top half = Left channel  |  Bottom half = Right channel
// - Beat reactive: waveform thickness doubles on beat
// -----------------------------------------------------------------------

// Decay buffer for persistence effect (phosphor afterglow simulation)
// Each cell fades from 2→1→0 over frames
static uint8_t s_persist[SCREEN_W][SCREEN_H / 2];   // L and R share same size

// Smoothed waveforms for anti-flicker
static int8_t s_wave_l[SCREEN_W];
static int8_t s_wave_r[SCREEN_W];

// Screen layout
static const int OSC_MARGIN   = 3;    // pixel margin from edge
static const int OSC_W        = SCREEN_W - 2 * OSC_MARGIN;  // 122px
static const int OSC_HALF_H   = SCREEN_H / 2;               // 32px per channel
static const int L_CY         = OSC_HALF_H / 2;             // 16 — L center Y in top half
static const int R_CY         = OSC_HALF_H + OSC_HALF_H/2;  // 48 — R center Y in bottom

void effect_oscilloscope_on_enter() {
    memset(s_persist, 0, sizeof(s_persist));
    memset(s_wave_l,  0, sizeof(s_wave_l));
    memset(s_wave_r,  0, sizeof(s_wave_r));
}

void effect_oscilloscope_on_exit() {
    memset(s_persist, 0, sizeof(s_persist));
}

// ── Find trigger point: first rising zero-crossing in the center third of buffer ──
static size_t find_trigger(const int32_t *buf, size_t n) {
    size_t start = n / 3;
    size_t end   = 2 * n / 3;
    for (size_t i = start; i < end - 1; i++) {
        if (buf[i] <= 0 && buf[i + 1] > 0) return i;
    }
    return 0;  // fallback: no trigger found, use start of buffer
}

// ── Draw dotted graticule grid (4 cols × 2 rows per half = 8 divisions) ──
static void draw_graticule() {
    // Horizontal center divider
    for (int x = 0; x < SCREEN_W; x += 2) {
        SafeDraw::drawPixel(x, OSC_HALF_H - 1);
    }
    // Vertical grid lines (3 internal columns, creating 4 cells)
    for (int col = 1; col <= 3; col++) {
        int gx = SCREEN_W * col / 4;
        for (int y = 0; y < SCREEN_H; y += 3) {
            SafeDraw::drawPixel(gx, y);
        }
    }
    // Horizontal center marks (tick marks at center Y of each half)
    for (int x = OSC_MARGIN; x < SCREEN_W; x += 4) {
        SafeDraw::drawPixel(x, L_CY);
        SafeDraw::drawPixel(x, R_CY);
    }
}

void effect_oscilloscope_render(const int32_t *left, const int32_t *right, size_t n) {
    bool on_beat = g_beat.beat_now && g_beat.confidence > 0.25f;

    // Scale reference: use AGC peak or fallback to nominal
    float scale_l = (s_peak_l > 100) ? (float)s_peak_l : AUDIO_NOMINAL_PEAK;
    float scale_r = (s_peak_r > 100) ? (float)s_peak_r : AUDIO_NOMINAL_PEAK;

    // ── Find trigger point ──────────────────────────────────────────────
    size_t trig = find_trigger(left, n);

    // ── Sample waveform points mapped to screen x positions ────────────
    float alpha = on_beat ? 0.6f : 0.35f;  // smoother persistence off-beat
    for (int x = 0; x < OSC_W; x++) {
        // Map x → source sample index relative to trigger
        size_t idx = trig + (size_t)((float)x * (float)n / (float)OSC_W / 2.0f);
        if (idx >= n) idx = n - 1;

        // Left channel: map to top half
        float fl = (float)left[idx]  / scale_l;
        fl = constrain(fl, -1.0f, 1.0f);
        int8_t yl = (int8_t)(fl * (float)(OSC_HALF_H / 2 - 2));

        // Right channel: map to bottom half
        float fr = (float)right[idx] / scale_r;
        fr = constrain(fr, -1.0f, 1.0f);
        int8_t yr = (int8_t)(fr * (float)(OSC_HALF_H / 2 - 2));

        // Smooth waveform to reduce jitter
        s_wave_l[x + OSC_MARGIN] = (int8_t)(s_wave_l[x + OSC_MARGIN] * (1.0f - alpha) + yl * alpha);
        s_wave_r[x + OSC_MARGIN] = (int8_t)(s_wave_r[x + OSC_MARGIN] * (1.0f - alpha) + yr * alpha);
    }

    // ── Draw graticule first (behind waveform) ──────────────────────────
    draw_graticule();

    // ── Draw waveforms ──────────────────────────────────────────────────
    // Connect adjacent points with lines for smooth trace
    for (int x = OSC_MARGIN; x < OSC_MARGIN + OSC_W - 1; x++) {
        int y0_l = L_CY - (int)s_wave_l[x];
        int y1_l = L_CY - (int)s_wave_l[x + 1];
        int y0_r = R_CY - (int)s_wave_r[x];
        int y1_r = R_CY - (int)s_wave_r[x + 1];

        // Clamp to channel boundaries
        y0_l = constrain(y0_l, 1, OSC_HALF_H - 2);
        y1_l = constrain(y1_l, 1, OSC_HALF_H - 2);
        y0_r = constrain(y0_r, OSC_HALF_H + 1, SCREEN_H - 2);
        y1_r = constrain(y1_r, OSC_HALF_H + 1, SCREEN_H - 2);

        SafeDraw::drawLine(x, y0_l, x + 1, y1_l);
        SafeDraw::drawLine(x, y0_r, x + 1, y1_r);

        // On-beat: draw thicker trace (extra pixel above/below)
        if (on_beat) {
            if (y0_l > 1)           SafeDraw::drawPixel(x, y0_l - 1);
            if (y0_r < SCREEN_H-2)  SafeDraw::drawPixel(x, y0_r + 1);
        }
    }

    // ── Channel labels ──────────────────────────────────────────────────
    SafeDraw::setFont(u8g2_font_04b_03_tr);
    SafeDraw::drawStr(0, 7,  "L");
    SafeDraw::drawStr(0, 39, "R");

    // ── BPM indicator (tiny, top-right, only when locked) ──────────────
    if (beat_detector_is_locked() && g_beat.bpm > 0.0f) {
        char bpm_buf[10];
        snprintf(bpm_buf, sizeof(bpm_buf), "%d", (int)(g_beat.bpm + 0.5f));
        SafeDraw::setFont(u8g2_font_04b_03_tr);
        int tw = SafeDraw::getStrWidth(bpm_buf);
        SafeDraw::drawStr(SCREEN_W - tw - 1, 7, bpm_buf);
    }
}
