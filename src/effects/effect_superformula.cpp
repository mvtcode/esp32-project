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
    const int cx = SCREEN_W / 2; // 64
    const int cy = SCREEN_H / 2; // 32

    // 1. Enhanced Audio Energy Measurement
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += abs(left[i]) + abs(right[i]);
    }
    float raw_vol = (float)sum / (float)(n * (s_peak_l + s_peak_r + 1));
    float vol = raw_vol * 2.2f;
    if (vol > 1.0f) vol = 1.0f;
    s_sf_vol = s_sf_vol * 0.70f + vol * 0.30f;

    // Fast dynamic rotation boosted by audio
    s_sf_rot += 0.025f + s_sf_vol * 0.08f;
    if (s_sf_rot > 6.2831853f) s_sf_rot -= 6.2831853f;

    // Morph parameters: m smoothly shifts between 3, 4, 5, 6, 7, 8 petals
    float target_m = 3.0f + 5.0f * (0.5f + 0.5f * sinf(s_sf_rot * 0.35f));
    s_sf_m = s_sf_m * 0.93f + target_m * 0.07f;

    // n1, n2, n3 control curvature and sharp starburst pinching on beat
    float target_n1 = 0.6f + s_sf_vol * 3.8f;
    s_sf_n1 = s_sf_n1 * 0.75f + target_n1 * 0.25f;

    float n2 = 1.6f + s_sf_vol * 1.5f;
    float n3 = 1.6f + s_sf_vol * 1.5f;
    float scale = 23.0f + s_sf_vol * 9.0f;

    float inv_pk_l = (s_peak_l > 0) ? (1.0f / (float)s_peak_l) : 0.0f;
    float inv_pk_r = (s_peak_r > 0) ? (1.0f / (float)s_peak_r) : 0.0f;

    const int STEPS = 80;
    int prev_x = -1, prev_y = -1;
    int first_x = -1, first_y = -1;

    // 2. Primary Outer Live-Morph Superformula (with Live Audio Waveform Modulation)
    for (int i = 0; i <= STEPS; i++) {
        float theta = ((float)i / (float)STEPS) * 6.2831853f;
        float base_r = eval_superformula(theta, s_sf_m, s_sf_n1, n2, n3, 1.0f, 1.0f) * scale;

        // Stereo audio waveform ripple along perimeter
        int sample_idx = (i * (int)n) / STEPS;
        if (sample_idx >= (int)n) sample_idx = n - 1;
        float wave = 0.0f;
        if (i < STEPS / 2) {
            wave = (float)left[sample_idx] * inv_pk_l;
        } else {
            wave = (float)right[sample_idx] * inv_pk_r;
        }
        float r = base_r + wave * (4.0f + s_sf_vol * 4.0f);
        if (r < 4.0f) r = 4.0f;
        if (r > 31.0f) r = 31.0f; // Screen clamp

        float render_angle = theta + s_sf_rot;
        int px = cx + (int)(r * cosf(render_angle));
        int py = cy + (int)(r * sinf(render_angle));

        if (i == 0) {
            first_x = px;
            first_y = py;
        } else {
            SafeDraw::drawLine(prev_x, prev_y, px, py);
        }
        prev_x = px;
        prev_y = py;

        // Radial star spokes on bass bursts or harmonic nodes
        if (s_sf_vol > 0.35f && (i % 8 == 0)) {
            int inner_spoke_x = cx + (int)((r * 0.4f) * cosf(render_angle));
            int inner_spoke_y = cy + (int)((r * 0.4f) * sinf(render_angle));
            SafeDraw::drawLine(inner_spoke_x, inner_spoke_y, px, py);
        }
    }
    if (first_x >= 0 && prev_x >= 0) {
        SafeDraw::drawLine(prev_x, prev_y, first_x, first_y);
    }

    // 3. Secondary Inner Counter-Rotating Concentric Geometry
    float in_scale = scale * 0.52f;
    prev_x = -1; prev_y = -1;
    first_x = -1; first_y = -1;
    for (int i = 0; i <= STEPS; i += 2) {
        float theta = ((float)i / (float)STEPS) * 6.2831853f;
        float r = eval_superformula(theta, s_sf_m, s_sf_n1 * 1.4f, n2, n3, 1.0f, 1.0f) * in_scale;
        float render_angle = theta - s_sf_rot * 1.6f;

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

    // 4. Center Audio Core & Energy Ring
    int core_r = 1 + (int)(s_sf_vol * 4.5f);
    SafeDraw::drawDisc(cx, cy, core_r);
    if (s_sf_vol > 0.25f) {
        SafeDraw::drawCircle(cx, cy, 7 + (int)(s_sf_vol * 4.0f));
    }

    // 5. Corner HUD
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 6, "SUPERFORM");
    SafeDraw::drawStr(104, 6, "GIELIS");
}
