#include "effects.h"

// -----------------------------------------------------------------------
// MODE 37 — CHLADNI (Cymatics 2D Acoustic Resonance Nodal Patterns)
// -----------------------------------------------------------------------

#define CHLADNI_NUM_PARTICLES 90

struct ChladniParticle {
    float x, y; // 0.0 to 1.0
};

static ChladniParticle s_particles[CHLADNI_NUM_PARTICLES];
static int s_chladni_n = 2;
static int s_chladni_m = 3;
static uint32_t s_last_mode_switch = 0;
static float s_chladni_vol = 0.0f;

static inline float chladni_val(float x, float y, int n, int m) {
    return sinf((float)n * PI * x) * sinf((float)m * PI * y) - sinf((float)m * PI * x) * sinf((float)n * PI * y);
}

void effect_chladni_on_enter() {
    s_chladni_n = 2;
    s_chladni_m = 3;
    s_last_mode_switch = millis();
    s_chladni_vol = 0.0f;

    // Randomize initial particle locations on plate
    for (int i = 0; i < CHLADNI_NUM_PARTICLES; i++) {
        s_particles[i].x = (float)(random(10, 90)) / 100.0f;
        s_particles[i].y = (float)(random(10, 90)) / 100.0f;
    }
}

void effect_chladni_on_exit() {}

void effect_chladni_render(const int32_t *left, const int32_t *right, size_t n) {
    // 1. Audio volume measurement & mode switching
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += abs(left[i]) + abs(right[i]);
    }
    float vol = (float)sum / (float)(n * (s_peak_l + s_peak_r + 1));
    s_chladni_vol = s_chladni_vol * 0.7f + vol * 0.3f;

    // Switch Chladni harmonic modes periodically or on strong beats
    uint32_t now = millis();
    if (now - s_last_mode_switch > 4000 || (vol > 0.75f && now - s_last_mode_switch > 1500)) {
        s_last_mode_switch = now;
        static const uint8_t MODES[][2] = {
            { 1, 2 }, { 2, 3 }, { 3, 1 }, { 3, 3 }, { 2, 4 }, { 4, 1 }, { 3, 5 }
        };
        static int mode_idx = 0;
        mode_idx = (mode_idx + 1) % 7;
        s_chladni_n = MODES[mode_idx][0];
        s_chladni_m = MODES[mode_idx][1];
    }

    // 2. Plate Geometry (Center Square on OLED)
    const int plate_x = 34;
    const int plate_y = 2;
    const int plate_w = 60;
    const int plate_h = 60;

    SafeDraw::drawFrame(plate_x, plate_y, plate_w, plate_h);
    SafeDraw::drawFrame(plate_x - 2, plate_y - 2, plate_w + 4, plate_h + 4);

    // 3. Update and render particles
    float step_eps = 0.02f;
    float jitter_scale = 0.008f + s_chladni_vol * 0.025f;

    for (int i = 0; i < CHLADNI_NUM_PARTICLES; i++) {
        float px = s_particles[i].x;
        float py = s_particles[i].y;

        // Gradient of |w(x, y)| to find nodal lines (where w ≈ 0)
        float w_curr = fabsf(chladni_val(px, py, s_chladni_n, s_chladni_m));
        float w_dx   = fabsf(chladni_val(px + step_eps, py, s_chladni_n, s_chladni_m));
        float w_dy   = fabsf(chladni_val(px, py + step_eps, s_chladni_n, s_chladni_m));

        float grad_x = (w_dx - w_curr) / step_eps;
        float grad_y = (w_dy - w_curr) / step_eps;

        // Move towards zero vibration node (gradient descent) + audio vibration noise
        float jitter_x = ((float)random(-100, 100) / 100.0f) * jitter_scale;
        float jitter_y = ((float)random(-100, 100) / 100.0f) * jitter_scale;

        px -= grad_x * 0.012f - jitter_x;
        py -= grad_y * 0.012f - jitter_y;

        // Keep inside plate bounds
        if (px < 0.02f) px = 0.02f + (float)random(0, 10) * 0.01f;
        if (px > 0.98f) px = 0.98f - (float)random(0, 10) * 0.01f;
        if (py < 0.02f) py = 0.02f + (float)random(0, 10) * 0.01f;
        if (py > 0.98f) py = 0.98f - (float)random(0, 10) * 0.01f;

        s_particles[i].x = px;
        s_particles[i].y = py;

        int sx = plate_x + (int)(px * (float)plate_w);
        int sy = plate_y + (int)(py * (float)plate_h);

        SafeDraw::drawPixel(sx, sy);
    }

    // 4. Side HUD with mode indicators & VU mini bars
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 8, "CYMATICS");
    SafeDraw::drawStr(2, 20, "CHLADNI");

    char mode_str[10];
    snprintf(mode_str, sizeof(mode_str), "N:%d M:%d", s_chladni_n, s_chladni_m);
    SafeDraw::drawStr(2, 34, mode_str);

    // Left channel mini VU
    int v_l = (int)(s_chladni_vol * 20.0f);
    SafeDraw::drawFrame(2, 40, 20, 5);
    if (v_l > 0) SafeDraw::drawBox(3, 41, v_l > 18 ? 18 : v_l, 3);

    SafeDraw::drawStr(100, 8, "PLATE");
    SafeDraw::drawStr(100, 20, "NODES");

    // Right channel mini VU
    SafeDraw::drawFrame(104, 40, 20, 5);
    if (v_l > 0) SafeDraw::drawBox(105, 41, v_l > 18 ? 18 : v_l, 3);
}
