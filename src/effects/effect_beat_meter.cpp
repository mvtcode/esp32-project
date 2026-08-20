#include "effects.h"
#include "../beat_detector.h"

// -----------------------------------------------------------------------
// MODE 65 — BEAT METER
// Realtime BPM display + 3-band energy bars + beat history dots + flash pulse
// -----------------------------------------------------------------------

// Animated pulse ring radius (expands on each beat, then decays)
static float s_pulse_radius = 0.0f;

// Beat history dots (circular buffer of last 16 beat timestamps)
#define DOT_HISTORY 16
static uint32_t s_beat_ts[DOT_HISTORY];
static uint8_t  s_dot_idx = 0;

// Smoothed band energy for display bars (prevent flickering)
static float s_bass_smooth  = 0.0f;
static float s_mid_smooth   = 0.0f;
static float s_rms_smooth   = 0.0f;

void effect_beat_meter_on_enter() {
    s_pulse_radius = 0.0f;
    s_dot_idx = 0;
    memset(s_beat_ts, 0, sizeof(s_beat_ts));
    s_bass_smooth = s_mid_smooth = s_rms_smooth = 0.0f;
}

void effect_beat_meter_on_exit() {
    s_pulse_radius = 0.0f;
}

void effect_beat_meter_render(const int32_t * /*left*/, const int32_t * /*right*/, size_t /*n*/) {
    const BeatInfo& beat = g_beat;
    uint32_t now = millis();

    // ── Smooth energy bars ──────────────────────────────────────────────
    float alpha = 0.25f;
    s_bass_smooth = s_bass_smooth * (1.0f - alpha) + g_frame_bands.bass  * alpha;
    s_mid_smooth  = s_mid_smooth  * (1.0f - alpha) + g_frame_bands.mid   * alpha;
    s_rms_smooth  = s_rms_smooth  * (1.0f - alpha) + g_frame_bands.rms   * alpha;

    // ── Beat pulse animation ────────────────────────────────────────────
    if (beat.beat_now) {
        s_pulse_radius = 6.0f;                 // reset to inner ring
        s_beat_ts[s_dot_idx] = now;
        s_dot_idx = (s_dot_idx + 1) % DOT_HISTORY;
    }
    if (s_pulse_radius > 0.0f) {
        s_pulse_radius += 2.8f;                // expand outward each frame
        if (s_pulse_radius > 32.0f) s_pulse_radius = 0.0f;
    }

    // ── Layout constants ────────────────────────────────────────────────
    // Left panel (x 0-68): BPM display + pulse ring
    // Right panel (x 70-127): 3 energy bars + beat dots
    const int DIVIDER_X = 69;

    // ═══════════════════════════════════════════════════════════════════
    // LEFT PANEL — BPM + Pulse Circle
    // ═══════════════════════════════════════════════════════════════════
    int cx = 34, cy = 32;

    // Outer static reference ring
    SafeDraw::drawCircle(cx, cy, 28, U8G2_DRAW_ALL);

    // Animated pulse ring (expands from center on each beat)
    if (s_pulse_radius > 0.0f) {
        int r = (int)s_pulse_radius;
        if (r >= 1 && r <= 27) {
            SafeDraw::drawCircle(cx, cy, r, U8G2_DRAW_ALL);
        }
    }

    // Musical note icon (♪) in center — drawn as pixel art
    // stem + flag
    SafeDraw::drawVLine(cx - 2, cy - 10, 8);
    SafeDraw::drawHLine(cx - 2, cy - 10, 6);
    SafeDraw::drawVLine(cx + 4, cy - 10, 5);
    // note head (filled oval)
    SafeDraw::drawBox(cx - 5, cy - 2, 5, 3);

    // BPM value — large and centered
    char bpm_str[8];
    if (beat.bpm > 0.0f && beat_detector_is_locked()) {
        snprintf(bpm_str, sizeof(bpm_str), "%d", (int)(beat.bpm + 0.5f));
    } else {
        snprintf(bpm_str, sizeof(bpm_str), "---");
    }

    SafeDraw::setFont(u8g2_font_7x14B_tf);
    int bw = SafeDraw::getStrWidth(bpm_str);
    SafeDraw::drawStr(cx - bw / 2, cy + 22, bpm_str);

    // "BPM" label below
    SafeDraw::setFont(u8g2_font_04b_03_tr);
    SafeDraw::drawStr(cx - 6, cy + 31, "BPM");

    // Confidence bar (thin arc at bottom of ring, shows how rhythmically locked)
    if (beat.confidence > 0.1f) {
        int bar_w = (int)(beat.confidence * 50.0f);
        int bar_x = cx - 25;
        SafeDraw::drawHLine(bar_x, cy + 26, 50);       // full outline
        if (bar_w > 0) {
            SafeDraw::drawHLine(bar_x, cy + 27, bar_w); // filled = confidence
        }
    }

    // ═══════════════════════════════════════════════════════════════════
    // RIGHT PANEL — 3 Band Bars + Beat Dot History
    // ═══════════════════════════════════════════════════════════════════

    // Vertical divider
    SafeDraw::drawVLine(DIVIDER_X, 0, 64);

    // Band bar parameters
    const int BAR_X0  = 73;   // start x of first bar
    const int BAR_GAP = 17;   // gap between bars
    const int BAR_W   = 11;   // width of each bar
    const int BAR_TOP = 2;
    const int BAR_BOT = 46;   // bar area bottom (leave room for labels)
    const int BAR_H   = BAR_BOT - BAR_TOP;

    float energies[3] = { s_bass_smooth, s_mid_smooth, s_rms_smooth };
    const char *labels[3] = { "SUB", "MID", "RMS" };

    for (int b = 0; b < 3; b++) {
        int bx = BAR_X0 + b * BAR_GAP;
        float e = energies[b];
        if (e > 1.0f) e = 1.0f;

        int fill_h = (int)(e * BAR_H);

        // Outline frame
        SafeDraw::drawFrame(bx - 1, BAR_TOP - 1, BAR_W + 2, BAR_H + 2);

        // Segmented fill (every 4px gap = segment look like LED bar)
        for (int y = BAR_BOT - fill_h; y < BAR_BOT; y++) {
            if ((BAR_BOT - y) % 4 != 0) {    // skip every 4th row = segment gap
                SafeDraw::drawHLine(bx, y, BAR_W);
            }
        }

        // Band label
        SafeDraw::setFont(u8g2_font_04b_03_tr);
        SafeDraw::drawStr(bx - 1, BAR_BOT + 7, labels[b]);
    }

    // ── Beat Dot History ────────────────────────────────────────────────
    // 8 dots in a row at bottom right — lights up on each beat, fades with age
    const int DOT_Y   = 58;
    const int DOT_X0  = 72;
    const int DOT_STEP = 7;

    SafeDraw::setFont(u8g2_font_6x10_tf);
    for (int i = 0; i < 8; i++) {
        int dot_x = DOT_X0 + i * DOT_STEP;
        // Find most recent beat in history buffer (reverse order)
        int hist_i = ((int)s_dot_idx - 1 - i + DOT_HISTORY) % DOT_HISTORY;
        uint32_t ts = s_beat_ts[hist_i];

        if (ts > 0) {
            uint32_t age_ms = now - ts;
            if (age_ms < 4000) {
                // Solid for fresh beats, hollow for older
                if (age_ms < 600) {
                    SafeDraw::drawBox(dot_x, DOT_Y - 2, 4, 4);    // solid dot
                } else {
                    SafeDraw::drawFrame(dot_x, DOT_Y - 2, 4, 4);  // hollow dot
                }
            } else {
                // Very old — just a pixel
                SafeDraw::drawPixel(dot_x + 1, DOT_Y);
            }
        } else {
            SafeDraw::drawPixel(dot_x + 1, DOT_Y);
        }
    }

    // Band that triggered last beat (shown as small indicator next to BPM)
    if (beat.bpm > 0.0f) {
        const char *band_names[] = { "SUB", "MID", "MIX" };
        SafeDraw::setFont(u8g2_font_04b_03_tr);
        SafeDraw::drawStr(cx - 7, cy + 9, band_names[beat.band < 3 ? beat.band : 0]);
    }
}
