#include "effects.h"

// -----------------------------------------------------------------------
// MODE 5 — ANALOG VU  (dual needle meter simulation with ballistics)
// -----------------------------------------------------------------------
static float s_analog_pos_l = 0.0f;
static float s_analog_pos_r = 0.0f;

void effect_analog_vu_on_enter() {
    s_analog_pos_l = 0.0f;
    s_analog_pos_r = 0.0f;
}

void effect_analog_vu_on_exit() {
    s_analog_pos_l = 0.0f;
    s_analog_pos_r = 0.0f;
}

static float rms_analog(const int32_t *buf, size_t n) {
    double sum = 0;
    for (size_t i = 0; i < n; i++) {
        double v = (double)buf[i] / 8388608.0; // 2^23 normalise to [-1,1]
        sum += v * v;
    }
    return (float)sqrt(sum / n);
}

static const float VU_DB_MIN = -60.0f;
static float linear_to_vu_analog(float rms_lin) {
    if (rms_lin < 1e-7f) return 0.0f;
    float db = 20.0f * log10f(rms_lin);
    float norm = (db - VU_DB_MIN) / (-VU_DB_MIN);
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;
    return norm;
}

static void draw_analog_submeter(int x0, int y0, const char *label, float pos) {
    // Outer frame for sub-meter
    int frame_x = x0 - 30;
    SafeDraw::drawRFrame(frame_x, 0, 62, 64, 2);

    // Scale tick angles (in radians from top vertical)
    static const float TICKS[] = {
        -0.628f, -0.419f, -0.209f, 0.0f, 0.209f, 0.419f, 0.628f
    };
    const int NUM_TICKS = 7;

    // Curved scale arc
    int prev_ax = 0, prev_ay = 0;
    for (int i = 0; i <= 24; i++) {
        float a = -0.628f + (1.256f * i / 24.0f);
        int ax = x0 + (int)(sinf(a) * 44.0f + 0.5f);
        int ay = y0 - (int)(cosf(a) * 44.0f + 0.5f);
        if (i > 0) SafeDraw::drawLine(prev_ax, prev_ay, ax, ay);
        prev_ax = ax;
        prev_ay = ay;
    }

    // Scale tick marks
    for (int i = 0; i < NUM_TICKS; i++) {
        float a = TICKS[i];
        float sa = sinf(a);
        float ca = cosf(a);
        int r_in = (i == 3 || i == 6) ? 39 : 41;  // 0dB and +3dB longer
        int r_out = (i == 3 || i == 6) ? 47 : 46;
        int x1 = x0 + (int)(sa * r_in  + 0.5f);
        int y1 = y0 - (int)(ca * r_in  + 0.5f);
        int x2 = x0 + (int)(sa * r_out + 0.5f);
        int y2 = y0 - (int)(ca * r_out + 0.5f);
        SafeDraw::drawLine(x1, y1, x2, y2);
    }

    // Red zone indicator arc (overload region above 0dB, a > 0)
    for (int i = 12; i <= 24; i++) {
        float a = -0.628f + (1.256f * i / 24.0f);
        int ax = x0 + (int)(sinf(a) * 46.0f + 0.5f);
        int ay = y0 - (int)(cosf(a) * 46.0f + 0.5f);
        SafeDraw::drawPixel(ax, ay);
    }

    // Scale markings and labels
    SafeDraw::setFont(u8g2_font_04b_03_tr);
    SafeDraw::drawStr(frame_x + 4, 10, label);
    SafeDraw::drawStr(x0 - 2, 8, "0");
    SafeDraw::drawStr(frame_x + 52, 11, "+");
    SafeDraw::drawStr(x0 - 5, 52, "VU");

    // Needle calculation & drawing
    float needle_angle = -0.628f + pos * 1.256f;
    int tip_x = x0 + (int)(sinf(needle_angle) * 43.0f + 0.5f);
    int tip_y = y0 - (int)(cosf(needle_angle) * 43.0f + 0.5f);
    SafeDraw::drawLine(x0, y0, tip_x, tip_y);
    SafeDraw::drawLine(x0 - 1, y0, tip_x, tip_y);

    // Pivot base disc
    SafeDraw::drawDisc(x0, y0, 2);
}

void effect_analog_vu_render(const int32_t *left, const int32_t *right, size_t n) {
    float rms_l = rms_analog(left,  n);
    float rms_r = rms_analog(right, n);
    if (rms_l > 1.0f) rms_l = 1.0f;
    if (rms_r > 1.0f) rms_r = 1.0f;

    float target_l = linear_to_vu_analog(rms_l);
    float target_r = linear_to_vu_analog(rms_r);

    // Physical needle ballistics: fast rise, smooth damped decay
    if (target_l > s_analog_pos_l) {
        s_analog_pos_l += (target_l - s_analog_pos_l) * 0.45f;
    } else {
        s_analog_pos_l += (target_l - s_analog_pos_l) * 0.12f;
    }

    if (target_r > s_analog_pos_r) {
        s_analog_pos_r += (target_r - s_analog_pos_r) * 0.45f;
    } else {
        s_analog_pos_r += (target_r - s_analog_pos_r) * 0.12f;
    }

    if (s_analog_pos_l < 0.0f) s_analog_pos_l = 0.0f;
    if (s_analog_pos_l > 1.0f) s_analog_pos_l = 1.0f;
    if (s_analog_pos_r < 0.0f) s_analog_pos_r = 0.0f;
    if (s_analog_pos_r > 1.0f) s_analog_pos_r = 1.0f;

    // Draw Left and Right analog meters side by side
    draw_analog_submeter(31, 59, "L", s_analog_pos_l);
    draw_analog_submeter(95, 59, "R", s_analog_pos_r);
}
