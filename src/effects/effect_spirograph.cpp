#include "effects.h"

// -----------------------------------------------------------------------
// MODE 33 — SPIROGRAPH (Audio-Reactive Hypotrochoid Parametric Flower)
// -----------------------------------------------------------------------

static float s_spiro_angle = 0.0f;
static float s_spiro_k = 3.0f; // Petal ratio
static float s_smooth_vol = 0.0f;

void effect_spirograph_on_enter() {
    s_spiro_angle = 0.0f;
    s_spiro_k = 3.0f;
    s_smooth_vol = 0.0f;
}

void effect_spirograph_on_exit() {}

void effect_spirograph_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 63, cy = 31;

    // 1. Calculate RMS / peak energy for animation speed & expansion
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += abs(left[i]) + abs(right[i]);
    }
    float vol = (float)sum / (float)(n * (s_peak_l + s_peak_r + 1));
    s_smooth_vol = s_smooth_vol * 0.7f + vol * 0.3f;
    if (s_smooth_vol > 1.0f) s_smooth_vol = 1.0f;

    // Advance rotation with speed proportional to audio energy
    s_spiro_angle += 0.03f + s_smooth_vol * 0.08f;
    if (s_spiro_angle > 6.2831853f * 100.0f) s_spiro_angle = 0.0f;

    // Slowly morph petal count over time (e.g. from 3 to 7 petals)
    float base_k = 3.0f + 2.0f * (0.5f + 0.5f * sinf(s_spiro_angle * 0.2f));
    s_spiro_k = s_spiro_k * 0.95f + base_k * 0.05f;

    // Outer & inner radii modulated by audio
    float R = 18.0f + s_smooth_vol * 10.0f;
    float r = R / s_spiro_k;
    float d = (8.0f + s_smooth_vol * 12.0f);

    // 2. Draw Hypotrochoid Curve
    const int STEPS = 80;
    const float max_t = 6.2831853f * 2.0f; // 2 turns
    int prev_x = -1, prev_y = -1;
    int first_x = -1, first_y = -1;

    for (int i = 0; i <= STEPS; i++) {
        float t = ((float)i / (float)STEPS) * max_t;
        float diff = R - r;
        float ratio = diff / r;

        // Apply global rotation
        float current_t = t + s_spiro_angle;

        float x = diff * cosf(current_t) + d * cosf(ratio * current_t);
        float y = diff * sinf(current_t) - d * sinf(ratio * current_t);

        int px = cx + (int)x;
        int py = cy + (int)y;

        if (px < 0) px = 0;
        if (px > 127) px = 127;
        if (py < 0) py = 0;
        if (py > 63) py = 63;

        if (i == 0) {
            first_x = px;
            first_y = py;
        } else {
            SafeDraw::drawLine(prev_x, prev_y, px, py);
        }
        prev_x = px;
        prev_y = py;
    }
    // Close the loop
    if (first_x >= 0 && prev_x >= 0) {
        SafeDraw::drawLine(prev_x, prev_y, first_x, first_y);
    }

    // 3. Center pulsating orb & corner details
    int core_r = 1 + (int)(s_smooth_vol * 4.0f);
    SafeDraw::drawDisc(cx, cy, core_r);

    // Corner HUD
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 6, "SPIRO");
    SafeDraw::drawStr(110, 6, "GEAR");
}
