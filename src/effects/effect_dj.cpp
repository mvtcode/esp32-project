#include "effects.h"

// -----------------------------------------------------------------------
// MODE 17 — MVT DJ DECK (Cyber DJ Turntable & Stereo Mixer)
// -----------------------------------------------------------------------
static float s_angle_l = 0.0f;
static float s_angle_r = 0.0f;
static float s_crossfader_pos = 64.0f; // Center: 64

void effect_dj_on_enter() {
    s_angle_l = 0.0f;
    s_angle_r = 0.0f;
    s_crossfader_pos = 64.0f;
}

void effect_dj_on_exit() {}

void effect_dj_render(const int32_t *left, const int32_t *right, size_t n) {
    // 1. Calculate RMS energy & stereo panning
    int64_t sum_l = 0, sum_r = 0;
    for (size_t i = 0; i < n; i++) {
        int32_t sl = left[i] < 0 ? -left[i] : left[i];
        int32_t sr = right[i] < 0 ? -right[i] : right[i];
        sum_l += sl;
        sum_r += sr;
    }
    float norm_l = (float)sum_l / ((float)n * (float)s_peak_l);
    float norm_r = (float)sum_r / ((float)n * (float)s_peak_r);
    if (norm_l > 1.0f) norm_l = 1.0f;
    if (norm_r > 1.0f) norm_r = 1.0f;

    // Turntables spin speed reactive to sound
    s_angle_l += 0.05f + norm_l * 0.18f;
    s_angle_r += 0.05f + norm_r * 0.18f;
    if (s_angle_l > 6.28318f) s_angle_l -= 6.28318f;
    if (s_angle_r > 6.28318f) s_angle_r -= 6.28318f;

    // Crossfader tracking stereo balance smoothly
    float target_fader = 64.0f + (norm_r - norm_l) * 18.0f;
    s_crossfader_pos += (target_fader - s_crossfader_pos) * 0.25f;

    // 2. Top mini waveform / live audio banner (y=0..9)
    SafeDraw::drawLine(0, 9, 127, 9);
    int step = (int)(n / 64);
    if (step < 1) step = 1;
    int prev_wy = 4;
    for (int col = 0; col < 64; col++) {
        int idx = col * step;
        int32_t smp = (left[idx] + right[idx]) / 2;
        int wy = 4 - (int)((smp * 4) / s_peak_l);
        if (wy < 0) wy = 0;
        if (wy > 8) wy = 8;
        if (col > 0) {
            SafeDraw::drawLine((col - 1) * 2, prev_wy, col * 2, wy);
        }
        prev_wy = wy;
    }

    // 3. Left Turntable (Deck A) - Center (28, 38)
    const int dl_x = 28, dl_y = 38, r_deck = 22;
    SafeDraw::drawCircle(dl_x, dl_y, r_deck);
    SafeDraw::drawCircle(dl_x, dl_y, 16);
    SafeDraw::drawCircle(dl_x, dl_y, 9);
    SafeDraw::drawDisc(dl_x, dl_y, 3); // Center spindle
    // Vinyl groove marker / strobe dot
    int mark_lx = dl_x + (int)(cosf(s_angle_l) * 14.0f);
    int mark_ly = dl_y + (int)(sinf(s_angle_l) * 14.0f);
    SafeDraw::drawLine(dl_x, dl_y, mark_lx, mark_ly);
    SafeDraw::drawDisc(mark_lx, mark_ly, 1);
    // Tonearm
    SafeDraw::drawLine(4, 14, 10, 24);
    SafeDraw::drawLine(10, 24, dl_x + (int)(cosf(s_angle_l * 0.3f) * 6.0f), dl_y - 12);

    // 4. Right Turntable (Deck B) - Center (100, 38)
    const int dr_x = 100, dr_y = 38;
    SafeDraw::drawCircle(dr_x, dr_y, r_deck);
    SafeDraw::drawCircle(dr_x, dr_y, 16);
    SafeDraw::drawCircle(dr_x, dr_y, 9);
    SafeDraw::drawDisc(dr_x, dr_y, 3); // Center spindle
    // Vinyl groove marker
    int mark_rx = dr_x + (int)(cosf(-s_angle_r) * 14.0f);
    int mark_ry = dr_y + (int)(sinf(-s_angle_r) * 14.0f);
    SafeDraw::drawLine(dr_x, dr_y, mark_rx, mark_ry);
    SafeDraw::drawDisc(mark_rx, mark_ry, 1);
    // Tonearm
    SafeDraw::drawLine(124, 14, 118, 24);
    SafeDraw::drawLine(118, 24, dr_x - (int)(cosf(s_angle_r * 0.3f) * 6.0f), dr_y - 12);

    // 5. Center Mixer Console (x=56..72)
    // Dual vertical LED meters
    int h_l = (int)(norm_l * 32.0f);
    int h_r = (int)(norm_r * 32.0f);
    SafeDraw::drawFrame(57, 13, 5, 36);
    SafeDraw::drawFrame(66, 13, 5, 36);
    if (h_l > 0) SafeDraw::drawBox(58, 48 - h_l, 3, h_l);
    if (h_r > 0) SafeDraw::drawBox(67, 48 - h_r, 3, h_r);

    // Crossfader at bottom
    SafeDraw::drawLine(48, 57, 80, 57);
    int fx = (int)s_crossfader_pos;
    if (fx < 48) fx = 48;
    if (fx > 78) fx = 78;
    SafeDraw::drawBox(fx - 2, 54, 5, 7);

    // Labels
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(1, 63, "A");
    SafeDraw::drawStr(123, 63, "B");
    // SafeDraw::drawStr(59, 63, "MVT");
}
