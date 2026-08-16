#include "effects.h"

// -----------------------------------------------------------------------
// MODE 62 — MAGIC PLASMA BALL (Core Electrode & Twisting Electric Arcs)
// -----------------------------------------------------------------------
struct PlasmaArc {
    float angle;
    float speed;
    float jitter;
    int branches;
};

static PlasmaArc s_arcs[7];

void effect_plasma_ball_on_enter() {
    for (int i = 0; i < 7; i++) {
        s_arcs[i].angle = (float)i * (6.28318f / 7.0f);
        s_arcs[i].speed = 0.02f + (float)(rand() % 20) / 1000.0f;
        s_arcs[i].jitter = 0.0f;
        s_arcs[i].branches = (i % 2 == 0) ? 1 : 0;
    }
}

void effect_plasma_ball_on_exit() {}

void effect_plasma_ball_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 64, cy = 30;
    const int sphere_r = 27;

    // 1. Audio Processing: RMS & Stereo
    int64_t sum_l = 0, sum_r = 0;
    for (size_t i = 0; i < n; i++) {
        sum_l += (left[i] < 0 ? -left[i] : left[i]);
        sum_r += (right[i] < 0 ? -right[i] : right[i]);
    }
    float norm_l = (float)sum_l / ((float)n * (float)s_peak_l);
    float norm_r = (float)sum_r / ((float)n * (float)s_peak_r);
    if (norm_l > 1.0f) norm_l = 1.0f;
    if (norm_r > 1.0f) norm_r = 1.0f;
    float avg = (norm_l + norm_r) * 0.5f;

    // Audio analysis — use pre-computed frame bands
    const float bass = g_frame_bands.bass;

    // 2. Draw Outer Glass Sphere & Stand Base
    SafeDraw::drawCircle(cx, cy, sphere_r);
    SafeDraw::drawCircle(cx, cy, sphere_r - 1);

    // Stand base below sphere
    SafeDraw::drawBox(cx - 10, cy + sphere_r - 2, 20, 5);
    SafeDraw::drawBox(cx - 14, cy + sphere_r + 3, 28, 4);

    // 3. Central Glowing Plasma Electrode Core
    int core_r = 4 + (int)(bass * 3.5f);
    SafeDraw::drawDisc(cx, cy, core_r);
    SafeDraw::drawCircle(cx, cy, core_r + 2);

    // 4. Update and Draw Twisting Electric Arcs
    for (int i = 0; i < 7; i++) {
        s_arcs[i].angle += s_arcs[i].speed * (1.0f + avg * 2.0f);
        if (s_arcs[i].angle > 6.28318f) s_arcs[i].angle -= 6.28318f;

        // Tendency to skew towards Stereo channels
        float arc_ang = s_arcs[i].angle + (norm_r - norm_l) * 0.4f;

        // Destination point on glass surface
        int end_x = cx + (int)(cosf(arc_ang) * (sphere_r - 2));
        int end_y = cy + (int)(sinf(arc_ang) * (sphere_r - 2));

        // Generate zig-zag fractal lightning arc segments from core to glass
        int cur_x = cx + (int)(cosf(arc_ang) * core_r);
        int cur_y = cy + (int)(sinf(arc_ang) * core_r);
        const int num_segments = 5;

        for (int s = 1; s <= num_segments; s++) {
            float t = (float)s / (float)num_segments;
            int ideal_x = cx + (int)((end_x - cx) * t);
            int ideal_y = cy + (int)((end_y - cy) * t);

            int jitter_mag = (s == num_segments) ? 0 : (int)(2.0f + avg * 4.0f + bass * 3.0f);
            int next_x = ideal_x + (rand() % (jitter_mag * 2 + 1) - jitter_mag);
            int next_y = ideal_y + (rand() % (jitter_mag * 2 + 1) - jitter_mag);

            SafeDraw::drawLine(cur_x, cur_y, next_x, next_y);
            if (bass > 0.45f && (s % 2 == 0)) {
                // Thicker bright beam on bass hit
                SafeDraw::drawLine(cur_x + 1, cur_y, next_x + 1, next_y);
            }

            // Side branch discharge
            if (s_arcs[i].branches > 0 && s == 3 && (bass > 0.3f || avg > 0.3f)) {
                int bx = next_x + (rand() % 9 - 4);
                int by = next_y + (rand() % 9 - 4);
                SafeDraw::drawLine(next_x, next_y, bx, by);
            }

            cur_x = next_x;
            cur_y = next_y;
        }

        // 5. Impact Spark on Glass Surface
        SafeDraw::drawDisc(end_x, end_y, 1);
        if (bass > 0.4f) {
            SafeDraw::drawCircle(end_x, end_y, 3);
        }
    }

    // 6. HUD Telemetry
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 6, "PLASMA CORE");
    SafeDraw::drawStr(98, 6, "MVT-TESLA");
}
