#include "effects.h"

// -----------------------------------------------------------------------
// MODE 16 — TWIN HEARTS (Dual Solid Hearts + Connecting Waveform Lifeline)
// -----------------------------------------------------------------------
static void draw_solid_heart_scaled(int hx, int hy, float scale) {
    int r     = 4 + (int)(scale * 1.6f + 0.5f);   // 5 to 6 px
    int dx    = 4 + (int)(scale * 1.4f + 0.5f);   // 5 px
    int dy    = 3 + (int)(scale * 1.0f + 0.5f);   // 4 px
    int tip_y = 8 + (int)(scale * 3.5f + 0.5f);   // 11 to 12 px

    SafeDraw::drawDisc(hx - dx, hy - dy, r);
    SafeDraw::drawDisc(hx + dx, hy - dy, r);
    SafeDraw::drawBox(hx - dx - r, hy - dy, 2 * (dx + r) + 1, dy + 1);
    SafeDraw::drawTriangle(hx - dx - r, hy, hx + dx + r, hy, hx, hy + tip_y);
}

void effect_twin_hearts_render(const int32_t *left, const int32_t *right, size_t n) {
    const int hx_l = 24;
    const int hx_r = 104;
    const int cy   = 31;

    // 1. Channel Peaks for Left and Right Heart Scaling
    int32_t max_l = 0, max_r = 0;
    for (size_t i = 0; i < n; i++) {
        int32_t al = abs(left[i]);
        int32_t ar = abs(right[i]);
        if (al > max_l) max_l = al;
        if (ar > max_r) max_r = ar;
    }
    float norm_l = (float)max_l / (float)(s_peak_l > 100 ? s_peak_l : 100);
    float norm_r = (float)max_r / (float)(s_peak_r > 100 ? s_peak_r : 100);
    if (norm_l > 1.0f) norm_l = 1.0f;
    if (norm_r > 1.0f) norm_r = 1.0f;

    float scale_l = 0.75f + powf(norm_l, 0.40f) * 0.45f;
    float scale_r = 0.75f + powf(norm_r, 0.40f) * 0.45f;

    // 2. Pulse echo rings when beating hard (clearance outside heart)
    if (norm_l > 0.55f) {
        int ring_r = (int)(15.0f * scale_l + 0.5f);
        SafeDraw::drawCircle(hx_l, cy + 1, ring_r);
    }
    if (norm_r > 0.55f) {
        int ring_r = (int)(15.0f * scale_r + 0.5f);
        SafeDraw::drawCircle(hx_r, cy + 1, ring_r);
    }

    // 3. Draw Dual Solid Hearts (1.2x scale, positioned inward)
    draw_solid_heart_scaled(hx_l, cy, scale_l);
    draw_solid_heart_scaled(hx_r, cy, scale_r);

    // 4. Connecting Shortened Waveform Lifeline between Hearts (X = 36 to X = 91)
    const int x_start = 36;
    const int x_end   = 91;
    int prev_x = x_start;
    int prev_y = cy;

    for (int x = x_start; x <= x_end; x++) {
        float rel = (float)(x - x_start) / (float)(x_end - x_start);
        float win = sinf(rel * 3.14159265f);  // Envelope: 0 at hearts, 1.0 at center

        int idx = (x - x_start) * (int)(n - 1) / (x_end - x_start);
        int32_t mix = (left[idx] + right[idx]) / 2;
        float wave_val = ((float)mix / (float)(s_peak_l > 100 ? s_peak_l : 100)) * 20.0f;

        int y = cy - (int)(wave_val * win);
        if (y < 4) y = 4;
        if (y > 58) y = 58;

        SafeDraw::drawLine(prev_x, prev_y, x, y);
        SafeDraw::drawPixel(x, y + 1);

        prev_x = x;
        prev_y = y;
    }
    SafeDraw::drawLine(prev_x, prev_y, x_end + 1, cy);

    // 5. Center Typography & Channel Labels
    SafeDraw::setFont(u8g2_font_04b_03_tr);
    SafeDraw::drawStr(56, 12, "MVT");
    SafeDraw::drawStr(40, 61, "STEREO BEAT");

    SafeDraw::drawStr(4, 61, "L");
    SafeDraw::drawStr(121, 61, "R");
}
