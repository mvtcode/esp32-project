#include "effects.h"

// -----------------------------------------------------------------------
// MODE 0 — WAVEFORM  (dual channel, L top / R bottom)
// -----------------------------------------------------------------------
static void draw_channel(const int32_t *buf, size_t n,
                         int cy, int half_h, int32_t peak) {
    for (int x = 0; x < SCREEN_W && x < (int)n; x++) {
        int y = cy - (int)((float)buf[x] / peak * half_h);
        if (y < cy - half_h) y = cy - half_h;
        if (y > cy + half_h) y = cy + half_h;
        SafeDraw::drawPixel(x, y);
    }
}

void effect_waveform_render(const int32_t *left, const int32_t *right, size_t n) {
    draw_channel(left,  n, CH_L_CENTER, CH_HALF_H, s_peak_l);
    SafeDraw::drawHLine(0, SEP_Y, SCREEN_W);
    draw_channel(right, n, CH_R_CENTER, CH_HALF_H, s_peak_r);
    SafeDraw::setFont(u8g2_font_04b_03_tr);
    SafeDraw::drawStr(1, 10, "L");
    SafeDraw::drawStr(1, 62, "R");
}
