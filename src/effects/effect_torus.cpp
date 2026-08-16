#include "effects.h"

// -----------------------------------------------------------------------
// MODE 44 — 3D AUDIO TORUS (Spinning Donut Wireframe with Ripple Waves)
// -----------------------------------------------------------------------

static float s_torus_rot_x = 0.0f;
static float s_torus_rot_y = 0.0f;
static float s_torus_rot_z = 0.0f;
static float s_torus_vol = 0.0f;

void effect_torus_on_enter() {
    s_torus_rot_x = 0.0f;
    s_torus_rot_y = 0.0f;
    s_torus_rot_z = 0.0f;
    s_torus_vol = 0.0f;
}

void effect_torus_on_exit() {}

void effect_torus_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = SCREEN_W / 2;
    const int cy = SCREEN_H / 2;

    // 1. Audio volume measurement
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += abs(left[i]) + abs(right[i]);
    }
    float vol = (float)sum / (float)(n * (s_peak_l + s_peak_r + 1));
    if (vol > 1.0f) vol = 1.0f;
    s_torus_vol = s_torus_vol * 0.7f + vol * 0.3f;

    // Continuous 3D rotation
    s_torus_rot_x += 0.025f + s_torus_vol * 0.04f;
    s_torus_rot_y += 0.035f + s_torus_vol * 0.05f;
    s_torus_rot_z += 0.015f + s_torus_vol * 0.02f;

    float cos_x = cosf(s_torus_rot_x), sin_x = sinf(s_torus_rot_x);
    float cos_y = cosf(s_torus_rot_y), sin_y = sinf(s_torus_rot_y);
    float cos_z = cosf(s_torus_rot_z), sin_z = sinf(s_torus_rot_z);

    // Torus dimensions
    float R_major = 22.0f + s_torus_vol * 6.0f;  // Major radius (center to tube center)
    float r_minor = 8.5f + s_torus_vol * 4.0f;   // Minor radius (tube thickness)

    float cam_dist = 60.0f;
    float fov = 60.0f;

    const int NUM_U = 12; // Slices around major ring
    const int NUM_V = 8;  // Points around tube section

    static int proj_x[12][8]; // static: NUM_U=12, NUM_V=8, avoid stack allocation
    static int proj_y[12][8]; // static: NUM_U=12, NUM_V=8, avoid stack allocation

    for (int i = 0; i < NUM_U; i++) {
        float u = (float)i * (2.0f * (float)M_PI / (float)NUM_U);
        float cos_u = cosf(u), sin_u = sinf(u);

        // Audio waveform modulation along the ring
        size_t sample_idx = (i * 8) % (n > 0 ? n : 1);
        float wave_mod = 0.0f;
        if (s_peak_l > 0) {
            wave_mod = ((float)left[sample_idx] / (float)s_peak_l) * (2.0f + s_torus_vol * 3.0f);
        }

        float current_r = r_minor + wave_mod;

        for (int j = 0; j < NUM_V; j++) {
            float v = (float)j * (2.0f * (float)M_PI / (float)NUM_V);
            float cos_v = cosf(v), sin_v = sinf(v);

            // Torus 3D coordinates
            float x0 = (R_major + current_r * cos_v) * cos_u;
            float y0 = (R_major + current_r * cos_v) * sin_u;
            float z0 = current_r * sin_v;

            // 3D Rotations: X -> Y -> Z
            // 1. Rotate X
            float y1 = y0 * cos_x - z0 * sin_x;
            float z1 = y0 * sin_x + z0 * cos_x;
            float x1 = x0;

            // 2. Rotate Y
            float x2 = x1 * cos_y + z1 * sin_y;
            float z2 = -x1 * sin_y + z1 * cos_y;
            float y2 = y1;

            // 3. Rotate Z
            float x3 = x2 * cos_z - y2 * sin_z;
            float y3 = x2 * sin_z + y2 * cos_z;
            float z3 = z2;

            // Perspective projection
            float z_cam = cam_dist - z3;
            if (z_cam < 1.0f) z_cam = 1.0f;

            proj_x[i][j] = cx + (int)((x3 * fov) / z_cam);
            proj_y[i][j] = cy - (int)((y3 * fov) / z_cam);
        }
    }

    // Draw tube section rings (along v)
    for (int i = 0; i < NUM_U; i++) {
        for (int j = 0; j < NUM_V; j++) {
            int next_j = (j + 1) % NUM_V;
            SafeDraw::drawLine(proj_x[i][j], proj_y[i][j], proj_x[i][next_j], proj_y[i][next_j]);
        }
    }

    // Draw major longitudinal lines (along u)
    for (int j = 0; j < NUM_V; j += 2) { // Every 2nd line for clarity
        for (int i = 0; i < NUM_U; i++) {
            int next_i = (i + 1) % NUM_U;
            SafeDraw::drawLine(proj_x[i][j], proj_y[i][j], proj_x[next_i][j], proj_y[next_i][j]);
        }
    }
}
