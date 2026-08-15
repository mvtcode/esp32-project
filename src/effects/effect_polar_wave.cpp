#include "effects.h"

// -----------------------------------------------------------------------
// MODE 36 — POLAR WAVE (Quantum Arc Reactor & Audio-Reactive Circular Ring)
// -----------------------------------------------------------------------

static float s_polar_rot = 0.0f;
static float s_polar_vol = 0.0f;

void effect_polar_wave_on_enter() {
    s_polar_rot = 0.0f;
    s_polar_vol = 0.0f;
}

void effect_polar_wave_on_exit() {}

void effect_polar_wave_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 63, cy = 31;
    const float base_r = 18.0f;

    // 1. Audio volume measurement
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += abs(left[i]) + abs(right[i]);
    }
    float vol = (float)sum / (float)(n * (s_peak_l + s_peak_r + 1));
    s_polar_vol = s_polar_vol * 0.7f + vol * 0.3f;

    // Advance slow rotational phase
    s_polar_rot += 0.02f + s_polar_vol * 0.05f;
    if (s_polar_rot > 6.2831853f) s_polar_rot -= 6.2831853f;

    // 2. Draw outer orbital reticle ticks
    const int num_ticks = 12;
    for (int i = 0; i < num_ticks; i++) {
        float a = s_polar_rot + ((float)i / (float)num_ticks) * 6.2831853f;
        int x1 = cx + (int)(27.0f * cosf(a));
        int y1 = cy + (int)(27.0f * sinf(a));
        int x2 = cx + (int)(30.0f * cosf(a));
        int y2 = cy + (int)(30.0f * sinf(a));
        SafeDraw::drawLine(x1, y1, x2, y2);
    }

    // 3. Render Circular Waveform (Stereo: Left on top/left, Right on bottom/right)
    const int PTS = 64;
    int first_x = -1, first_y = -1;
    int prev_x = -1, prev_y = -1;

    float inv_pk_l = 1.0f / (float)s_peak_l;
    float inv_pk_r = 1.0f / (float)s_peak_r;

    for (int i = 0; i <= PTS; i++) {
        int sample_idx = (i * (int)n) / PTS;
        if (sample_idx >= (int)n) sample_idx = n - 1;

        float amp = 0.0f;
        if (i < PTS / 2) {
            amp = (float)left[sample_idx] * inv_pk_l;
        } else {
            amp = (float)right[sample_idx] * inv_pk_r;
        }

        float theta = ((float)i / (float)PTS) * 6.2831853f + s_polar_rot;
        float r = base_r + amp * 9.0f + s_polar_vol * 4.0f;
        if (r < 5.0f) r = 5.0f;
        if (r > 30.0f) r = 30.0f;

        int px = cx + (int)(r * cosf(theta));
        int py = cy + (int)(r * sinf(theta));

        if (i == 0) {
            first_x = px;
            first_y = py;
        } else {
            SafeDraw::drawLine(prev_x, prev_y, px, py);
        }
        prev_x = px;
        prev_y = py;

        // Peak burst spikes
        if (fabsf(amp) > 0.65f && (i % 4 == 0)) {
            int out_x = cx + (int)((r + 5.0f) * cosf(theta));
            int out_y = cy + (int)((r + 5.0f) * sinf(theta));
            SafeDraw::drawLine(px, py, out_x, out_y);
        }
    }
    if (first_x >= 0 && prev_x >= 0) {
        SafeDraw::drawLine(prev_x, prev_y, first_x, first_y);
    }

    // 4. Center Pulsing Core & Rings
    int core_r = 2 + (int)(s_polar_vol * 5.0f);
    SafeDraw::drawDisc(cx, cy, core_r);
    SafeDraw::drawCircle(cx, cy, 9);

    // 5. HUD
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 6, "POLAR RING");
    SafeDraw::drawStr(108, 6, "REACTOR");
}
