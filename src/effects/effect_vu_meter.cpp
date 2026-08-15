#include "effects.h"

// -----------------------------------------------------------------------
// MODE 4 — VU METER  (two vertical bars + peak hold)
// -----------------------------------------------------------------------
static float    s_vu_pk_l  = 0.0f, s_vu_pk_r  = 0.0f;
static uint32_t s_vu_ts_l  = 0,    s_vu_ts_r  = 0;
static const uint32_t VU_HOLD_MS   = 1200; // peak hold duration
static const float    VU_PK_DECAY  = 0.97f; // peak release speed

void effect_vu_meter_on_enter() {
    s_vu_pk_l  = s_vu_pk_r  = 0.0f;
    s_vu_ts_l  = s_vu_ts_r  = millis();
}

void effect_vu_meter_on_exit() {
    s_vu_pk_l  = s_vu_pk_r  = 0.0f;
    s_vu_ts_l  = s_vu_ts_r  = 0;
}

static float rms(const int32_t *buf, size_t n) {
    double sum = 0;
    for (size_t i = 0; i < n; i++) {
        double v = (double)buf[i] / 8388608.0; // 2^23 normalise to [-1,1]
        sum += v * v;
    }
    return (float)sqrt(sum / n);
}

// Convert linear RMS [0,1] → display position [0,1] using dB scale.
// Maps DB_MIN..0 dBFS onto the full bar height.
// Result is clamped to [0, 1].
static const float VU_DB_MIN = -60.0f;  // floor: signals below this = 0%
static float linear_to_vu(float rms_lin) {
    if (rms_lin < 1e-7f) return 0.0f;
    float db = 20.0f * log10f(rms_lin);           // e.g. 0.01 → -40 dB
    float norm = (db - VU_DB_MIN) / (-VU_DB_MIN); // [-60,0] → [0,1]
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;
    return norm;
}

void effect_vu_meter_render(const int32_t *left, const int32_t *right, size_t n) {
    float rms_l = rms(left,  n);
    float rms_r = rms(right, n);
    if (rms_l > 1.0f) rms_l = 1.0f;
    if (rms_r > 1.0f) rms_r = 1.0f;

    // Peak hold logic
    uint32_t now = millis();
    if (rms_l >= s_vu_pk_l) { s_vu_pk_l = rms_l; s_vu_ts_l = now; }
    else if (now - s_vu_ts_l > VU_HOLD_MS) s_vu_pk_l *= VU_PK_DECAY;

    if (rms_r >= s_vu_pk_r) { s_vu_pk_r = rms_r; s_vu_ts_r = now; }
    else if (now - s_vu_ts_r > VU_HOLD_MS) s_vu_pk_r *= VU_PK_DECAY;

    // Bar geometry (two bars side by side, symmetric)
    const int BAR_W  = 52;
    const int BAR_Y0 = 4;   // top of bar area
    const int BAR_B  = 56;  // bottom of bar area
    const int BAR_H  = BAR_B - BAR_Y0;
    const int BAR_LX = 5;   // left bar x
    const int BAR_RX = 71;  // right bar x

    // Outline frames
    SafeDraw::drawFrame(BAR_LX - 1, BAR_Y0 - 1, BAR_W + 2, BAR_H + 2);
    SafeDraw::drawFrame(BAR_RX - 1, BAR_Y0 - 1, BAR_W + 2, BAR_H + 2);

    // Convert linear RMS → dB-normalised [0,1] for display
    float vu_l = linear_to_vu(rms_l);
    float vu_r = linear_to_vu(rms_r);
    float pk_vu_l = linear_to_vu(s_vu_pk_l);
    float pk_vu_r = linear_to_vu(s_vu_pk_r);

    // Filled bars (bottom-up, dB scale)
    int h_l = (int)(vu_l * BAR_H);
    int h_r = (int)(vu_r * BAR_H);
    if (h_l > 0) SafeDraw::drawBox(BAR_LX, BAR_B - h_l, BAR_W, h_l);
    if (h_r > 0) SafeDraw::drawBox(BAR_RX, BAR_B - h_r, BAR_W, h_r);

    // Peak hold markers (single horizontal line above the filled bar)
    int pk_y_l = BAR_B - (int)(pk_vu_l * BAR_H) - 1;
    int pk_y_r = BAR_B - (int)(pk_vu_r * BAR_H) - 1;
    // Clamp so markers never leave the bar frame
    if (pk_y_l < BAR_Y0) pk_y_l = BAR_Y0;
    if (pk_y_r < BAR_Y0) pk_y_r = BAR_Y0;
    // Draw marker only when it sits at or above the top of the filled bar
    if (pk_y_l <= BAR_B - h_l)
        SafeDraw::drawHLine(BAR_LX, pk_y_l, BAR_W);
    if (pk_y_r <= BAR_B - h_r)
        SafeDraw::drawHLine(BAR_RX, pk_y_r, BAR_W);

    // Labels
    SafeDraw::setFont(u8g2_font_6x10_tf);
    SafeDraw::drawStr(BAR_LX + 22, 63, "L");
    SafeDraw::drawStr(BAR_RX + 22, 63, "R");
}
