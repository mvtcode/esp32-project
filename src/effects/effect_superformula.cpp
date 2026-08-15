#include "effects.h"

// -----------------------------------------------------------------------
// MODE 34 — SUPERFORMULA (Johan Gielis Bio-Morphing Mathematical Geometry)
// -----------------------------------------------------------------------

static float s_sf_rot = 0.0f;
static float s_sf_m = 5.0f;
static float s_sf_n1 = 1.0f;
static float s_sf_vol = 0.0f;

void effect_superformula_on_enter() {
    s_sf_rot = 0.0f;
    s_sf_m = 5.0f;
    s_sf_n1 = 1.0f;
    s_sf_vol = 0.0f;
}

void effect_superformula_on_exit() {}

// Fast superformula evaluation
static inline float eval_superformula(float theta, float m, float n1, float n2, float n3, float a, float b) {
    float t1 = fabsf(cosf(m * theta * 0.25f) / a);
    t1 = powf(t1, n2);

    float t2 = fabsf(sinf(m * theta * 0.25f) / b);
    t2 = powf(t2, n3);

    float sum = t1 + t2;
    if (sum < 0.00001f) return 0.0f;
    return powf(sum, -1.0f / n1);
}

void effect_superformula_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 63, cy = 31;

    // Audio energy
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += abs(left[i]) + abs(right[i]);
    }
    float vol = (float)sum / (float)(n * (s_peak_l + s_peak_r + 1));
    s_sf_vol = s_sf_vol * 0.75f + vol * 0.25f;

    // Rotation & shape dynamics
    s_sf_rot += 0.02f + s_sf_vol * 0.06f;

    // Morph parameters: m shifts between 3, 4, 5, 6, 7, 8
    float target_m = 3.0f + 5.0f * (0.5f + 0.5f * sinf(s_sf_rot * 0.15f));
    s_sf_m = s_sf_m * 0.96f + target_m * 0.04f;

    // n1 pinches to star with loud beats
    float target_n1 = 0.8f + s_sf_vol * 3.5f;
    s_sf_n1 = s_sf_n1 * 0.8f + target_n1 * 0.2f;

    float n2 = 1.7f + s_sf_vol * 1.2f;
    float n3 = 1.7f + s_sf_vol * 1.2f;
    float scale = 22.0f + s_sf_vol * 8.0f;

    const int STEPS = 72;
    int prev_x = -1, prev_y = -1;
    int first_x = -1, first_y = -1;

    for (int i = 0; i <= STEPS; i++) {
        float theta = ((float)i / (float)STEPS) * 6.2831853f;
        float r = eval_superformula(theta, s_sf_m, s_sf_n1, n2, n3, 1.0f, 1.0f) * scale;
        if (r > 30.0f) r = 30.0f; // Screen clamp radius

        float render_angle = theta + s_sf_rot;
        int px = cx + (int)(r * cosf(render_angle));
        int py = cy + (int)(r * sinf(render_angle));

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
    if (first_x >= 0 && prev_x >= 0) {
        SafeDraw::drawLine(prev_x, prev_y, first_x, first_y);
    }

    // Inner concentric mini morph
    float in_scale = scale * 0.45f;
    prev_x = -1; prev_y = -1;
    first_x = -1; first_y = -1;
    for (int i = 0; i <= STEPS; i += 2) {
        float theta = ((float)i / (float)STEPS) * 6.2831853f;
        float r = eval_superformula(theta, s_sf_m, s_sf_n1 * 1.5f, n2, n3, 1.0f, 1.0f) * in_scale;
        float render_angle = theta - s_sf_rot * 1.5f;

        int px = cx + (int)(r * cosf(render_angle));
        int py = cy + (int)(r * sinf(render_angle));

        if (i == 0) {
            first_x = px; first_y = py;
        } else {
            SafeDraw::drawLine(prev_x, prev_y, px, py);
        }
        prev_x = px; prev_y = py;
    }
    if (first_x >= 0 && prev_x >= 0) {
        SafeDraw::drawLine(prev_x, prev_y, first_x, first_y);
    }

    // Center core
    SafeDraw::drawPixel(cx, cy);

    // Corner HUD
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 6, "SUPER");
    SafeDraw::drawStr(108, 6, "GIELIS");
}
