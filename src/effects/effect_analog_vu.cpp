#include "effects.h"

// -----------------------------------------------------------------------
// MODE 5 — ANALOG VU & CASSETTE TAPE (dual meter + spinning tape reels)
// -----------------------------------------------------------------------
static float s_analog_pos_l = 0.0f;
static float s_analog_pos_r = 0.0f;
static float s_lp_l = 0.0f;
static float s_lp_r = 0.0f;
static float s_tape_angle = 0.0f;

void effect_analog_vu_on_enter() {
    s_analog_pos_l = 0.0f;
    s_analog_pos_r = 0.0f;
    s_lp_l = 0.0f;
    s_lp_r = 0.0f;
    s_tape_angle = 0.0f;
}

void effect_analog_vu_on_exit() {
    s_analog_pos_l = 0.0f;
    s_analog_pos_r = 0.0f;
    s_lp_l = 0.0f;
    s_lp_r = 0.0f;
    s_tape_angle = 0.0f;
}

// Bass-weighted RMS calculation: prioritizes low-frequency (<350Hz) kick & bass energy
// while maintaining full-spectrum awareness and increased overall sensitivity.
static float rms_analog_bass_weighted(const int32_t *buf, size_t n, float &lp_state) {
    // 1st order IIR Low-Pass filter cutoff ~350Hz @ 16kHz sample rate (alpha ≈ 0.14)
    const float LP_ALPHA = 0.14f;
    double sum_sq = 0.0;
    for (size_t i = 0; i < n; i++) {
        float x = (float)buf[i];
        lp_state += LP_ALPHA * (x - lp_state); // low-pass component

        // Emphasize bass (60% bass with 2.2x boost + 40% full-band) with 1.35x sensitivity gain
        float weighted = (0.40f * x + 0.60f * (lp_state * 2.2f)) * 1.35f;
        double v = (double)weighted / (double)AUDIO_NOMINAL_PEAK;
        sum_sq += v * v;
    }
    return (float)sqrt(sum_sq / n);
}

// Convert RMS [0, 1] → VU needle deflection [0, 1] with expanded dynamic range (-42 dB floor)
static const float VU_DB_MIN = -42.0f;
static float linear_to_vu_analog(float rms_lin) {
    if (rms_lin < 0.0005f) return 0.0f;
    float db = 20.0f * log10f(rms_lin);
    float norm = (db - VU_DB_MIN) / (-VU_DB_MIN);
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;
    return norm;
}

static void draw_analog_submeter(int x0, int y0, const char *label, float pos) {
    // Outer frame for sub-meter (height: 44px, Y: 0..43)
    int frame_x = x0 - 30;
    SafeDraw::drawRFrame(frame_x, 0, 62, 44, 2);

    // Scale tick angles (in radians from top vertical): classic 7-point scale
    static const float TICKS[] = {
        -0.628f, -0.419f, -0.209f, 0.0f, 0.209f, 0.419f, 0.628f
    };
    const int NUM_TICKS = 7;

    // Curved scale arc (R = 33.0)
    int prev_ax = 0, prev_ay = 0;
    for (int i = 0; i <= 24; i++) {
        float a = -0.628f + (1.256f * i / 24.0f);
        int ax = x0 + (int)(sinf(a) * 33.0f + 0.5f);
        int ay = y0 - (int)(cosf(a) * 33.0f + 0.5f);
        if (i > 0) SafeDraw::drawLine(prev_ax, prev_ay, ax, ay);
        prev_ax = ax;
        prev_ay = ay;
    }

    // Scale tick marks (clear radial rule divisions)
    for (int i = 0; i < NUM_TICKS; i++) {
        float a = TICKS[i];
        float sa = sinf(a);
        float ca = cosf(a);
        // 0dB (i=3) and +3dB (i=6) longer major ticks
        int r_in = (i == 3 || i == 6) ? 29 : 31;
        int r_out = (i == 3 || i == 6) ? 37 : 36;
        int x1 = x0 + (int)(sa * r_in  + 0.5f);
        int y1 = y0 - (int)(ca * r_in  + 0.5f);
        int x2 = x0 + (int)(sa * r_out + 0.5f);
        int y2 = y0 - (int)(ca * r_out + 0.5f);
        SafeDraw::drawLine(x1, y1, x2, y2);
    }

    // Scale markings and labels (VU text placed at top right replacing +)
    SafeDraw::setFont(u8g2_font_04b_03_tr);
    SafeDraw::drawStr(frame_x + 4, 8, label);
    SafeDraw::drawStr(x0 - 2, 7, "0");
    SafeDraw::drawStr(frame_x + 49, 8, "VU");

    // Needle calculation & drawing (needle length = 32, pivot at y0=41 inside frame)
    float needle_angle = -0.628f + pos * 1.256f;
    int tip_x = x0 + (int)(sinf(needle_angle) * 32.0f + 0.5f);
    int tip_y = y0 - (int)(cosf(needle_angle) * 32.0f + 0.5f);
    SafeDraw::drawLine(x0, y0, tip_x, tip_y);
    SafeDraw::drawLine(x0 - 1, y0, tip_x, tip_y);

    // Pivot base disc (fully inside frame)
    SafeDraw::drawDisc(x0, y0, 2);
}

static void draw_running_tape_deck(float audio_level) {
    // Update tape reel rotation angle (continuous motion + dynamic audio reactivity)
    s_tape_angle += 0.07f + audio_level * 0.14f;
    if (s_tape_angle > 6.283185f) s_tape_angle -= 6.283185f;

    // Dual Tape Reels centered closer together (Left roll at X=47, Right roll at X=80, Y=54)
    const int sp_lx = 47, sp_rx = 80, sp_y = 54;
    const int R_REEL = 7, R_HUB = 2;

    // Tape ribbon on top connecting both rolls (sợi dây trên đầu)
    SafeDraw::drawLine(sp_lx, sp_y - R_REEL, sp_rx, sp_y - R_REEL);

    // Outer tape roll & inner hub
    SafeDraw::drawCircle(sp_lx, sp_y, R_REEL);
    SafeDraw::drawCircle(sp_rx, sp_y, R_REEL);
    SafeDraw::drawDisc(sp_lx, sp_y, R_HUB);
    SafeDraw::drawDisc(sp_rx, sp_y, R_HUB);

    // 3 Rotating Spool Spokes for each reel
    for (int t = 0; t < 3; t++) {
        float a = s_tape_angle + t * (6.283185f / 3.0f);
        float ca = cosf(a);
        float sa = sinf(a);

        int dx1 = (int)(ca * (R_HUB + 1) + 0.5f);
        int dy1 = (int)(sa * (R_HUB + 1) + 0.5f);
        int dx2 = (int)(ca * (R_REEL - 1) + 0.5f);
        int dy2 = (int)(sa * (R_REEL - 1) + 0.5f);

        SafeDraw::drawLine(sp_lx + dx1, sp_y + dy1, sp_lx + dx2, sp_y + dy2);
        SafeDraw::drawLine(sp_rx + dx1, sp_y + dy1, sp_rx + dx2, sp_y + dy2);
    }
}

void effect_analog_vu_render(const int32_t *left, const int32_t *right, size_t n) {
    float rms_l = rms_analog_bass_weighted(left,  n, s_lp_l);
    float rms_r = rms_analog_bass_weighted(right, n, s_lp_r);
    if (rms_l > 1.0f) rms_l = 1.0f;
    if (rms_r > 1.0f) rms_r = 1.0f;

    float target_l = linear_to_vu_analog(rms_l);
    float target_r = linear_to_vu_analog(rms_r);

    // Physical needle ballistics: fast punchy rise on transients, smooth damped decay
    if (target_l > s_analog_pos_l) {
        s_analog_pos_l += (target_l - s_analog_pos_l) * 0.52f;
    } else {
        s_analog_pos_l += (target_l - s_analog_pos_l) * 0.14f;
    }

    if (target_r > s_analog_pos_r) {
        s_analog_pos_r += (target_r - s_analog_pos_r) * 0.52f;
    } else {
        s_analog_pos_r += (target_r - s_analog_pos_r) * 0.14f;
    }

    if (s_analog_pos_l < 0.0f) s_analog_pos_l = 0.0f;
    if (s_analog_pos_l > 1.0f) s_analog_pos_l = 1.0f;
    if (s_analog_pos_r < 0.0f) s_analog_pos_r = 0.0f;
    if (s_analog_pos_r > 1.0f) s_analog_pos_r = 1.0f;

    // 1. Draw Left and Right analog meters side by side (Upper area: Y = 0..43, pivot at y0=41)
    draw_analog_submeter(31, 41, "L", s_analog_pos_l);
    draw_analog_submeter(96, 41, "R", s_analog_pos_r);

    // 2. Draw running tape deck animation (Lower area: Y = 47..61, centered reels)
    float avg_level = (s_analog_pos_l + s_analog_pos_r) * 0.5f;
    draw_running_tape_deck(avg_level);
}


