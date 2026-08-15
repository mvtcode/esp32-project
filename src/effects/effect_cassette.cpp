#include "effects.h"

// -----------------------------------------------------------------------
// MODE 10 — MVT CASSETTE (Vintage Cassette Tape with Spinning Spools & Tape Wave)
// -----------------------------------------------------------------------
static float s_cassette_angle = 0.0f;

void effect_cassette_on_enter() {
    s_cassette_angle = 0.0f;
}

void effect_cassette_on_exit() {
    s_cassette_angle = 0.0f;
}

void effect_cassette_render(const int32_t *left, const int32_t *right, size_t n) {
    // 1. Audio reactivity & spool spinning
    int32_t pk = s_peak_l > s_peak_r ? s_peak_l : s_peak_r;
    float norm_pk = (float)pk / 32767.0f;
    s_cassette_angle += 0.08f + norm_pk * 0.20f;
    if (s_cassette_angle > 6.283185f) s_cassette_angle -= 6.283185f;

    // 2. Outer Cassette Shell
    SafeDraw::drawRFrame(8, 2, 112, 60, 3);          // Outer body
    SafeDraw::drawRFrame(12, 6, 104, 52, 2);         // Inner rim
    SafeDraw::drawBox(20, 52, 88, 10);               // Bottom tape head trapezoid base
    SafeDraw::setDrawColor(0);
    SafeDraw::drawBox(26, 56, 76, 6);
    SafeDraw::setDrawColor(1);

    // Cassette Label Window
    SafeDraw::drawRFrame(28, 12, 72, 38, 2);

    // 3. Two Spools (Left at X=46, Right at X=82, Y=31)
    const int sp_lx = 46, sp_rx = 82, sp_y = 31, R_SPOOL = 9, R_HUB = 3;
    SafeDraw::drawCircle(sp_lx, sp_y, R_SPOOL);
    SafeDraw::drawCircle(sp_rx, sp_y, R_SPOOL);
    SafeDraw::drawDisc(sp_lx, sp_y, R_HUB);
    SafeDraw::drawDisc(sp_rx, sp_y, R_HUB);

    // 3 Spool Teeth per wheel
    for (int t = 0; t < 3; t++) {
        float a = s_cassette_angle + t * (6.283185f / 3.0f);
        int dx1 = (int)(cosf(a) * 4 + 0.5f);
        int dy1 = (int)(sinf(a) * 4 + 0.5f);
        int dx2 = (int)(cosf(a) * (R_SPOOL - 1) + 0.5f);
        int dy2 = (int)(sinf(a) * (R_SPOOL - 1) + 0.5f);
        SafeDraw::drawLine(sp_lx + dx1, sp_y + dy1, sp_lx + dx2, sp_y + dy2);
        SafeDraw::drawLine(sp_rx + dx1, sp_y + dy1, sp_rx + dx2, sp_y + dy2);
    }

    // 4. Center Magnetic Tape Waveform Window (X=57..71)
    SafeDraw::drawFrame(56, 22, 16, 18);
    for (int x = 57; x < 71; x++) {
        int idx = (x - 57) * (int)(n - 1) / 14;
        int32_t mix = (left[idx] + right[idx]) / 2;
        int y = sp_y + (int)((float)mix / (float)s_peak_l * 7.0f);
        if (y < 23) y = 23;
        if (y > 39) y = 39;
        SafeDraw::drawPixel(x, y);
    }

    // 5. Vintage Branding
    SafeDraw::setFont(u8g2_font_04b_03_tr);
    SafeDraw::drawStr(34, 20, "MVT");
    SafeDraw::drawStr(76, 20, "Hi-Fi");
    SafeDraw::drawStr(50, 48, "STEREO 60");

    // Screws at 4 corners
    SafeDraw::drawPixel(11, 5);
    SafeDraw::drawPixel(116, 5);
    SafeDraw::drawPixel(11, 58);
    SafeDraw::drawPixel(116, 58);
}
