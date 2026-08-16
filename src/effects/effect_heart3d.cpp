#include "effects.h"

// -----------------------------------------------------------------------
// MODE 50 — 3D AUDIO HEART (Parametric 3D Wireframe Heart with Wave Ripples)
// -----------------------------------------------------------------------

static float s_heart3d_rot_x = 0.0f;
static float s_heart3d_rot_y = 0.0f;
static float s_heart3d_rot_z = 0.0f;
static float s_heart3d_vol = 0.0f;

void effect_heart3d_on_enter() {
    s_heart3d_rot_x = 0.0f;
    s_heart3d_rot_y = 0.0f;
    s_heart3d_rot_z = 0.0f;
    s_heart3d_vol = 0.0f;
}

void effect_heart3d_on_exit() {}

void effect_heart3d_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = SCREEN_W / 2;
    const int cy = SCREEN_H / 2;

    // 1. Audio volume measurement
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += abs(left[i]) + abs(right[i]);
    }
    float vol = (float)sum / (float)(n * (s_peak_l + s_peak_r + 1));
    if (vol > 1.0f) vol = 1.0f;
    s_heart3d_vol = s_heart3d_vol * 0.7f + vol * 0.3f;

    // Continuous 3D rotation speed modulated by audio volume
    s_heart3d_rot_x += 0.024f + s_heart3d_vol * 0.045f;
    s_heart3d_rot_y += 0.034f + s_heart3d_vol * 0.055f;
    s_heart3d_rot_z += 0.016f + s_heart3d_vol * 0.025f;

    float cos_x = cosf(s_heart3d_rot_x), sin_x = sinf(s_heart3d_rot_x);
    float cos_y = cosf(s_heart3d_rot_y), sin_y = sinf(s_heart3d_rot_y);
    float cos_z = cosf(s_heart3d_rot_z), sin_z = sinf(s_heart3d_rot_z);

    // Heart scale with bass heartbeat pulse (enlarged for full-screen WOW effect)
    float base_scale = 1.75f + s_heart3d_vol * 0.55f;
    float cam_dist = 60.0f;
    float fov = 60.0f;

    const int NUM_U = 18; // Points around heart perimeter
    const int NUM_V = 7;  // Depth layers from front to back

    static int proj_x[18][7]; // static: NUM_U=18, NUM_V=7, avoid stack allocation
    static int proj_y[18][7]; // static: NUM_U=18, NUM_V=7, avoid stack allocation

    for (int i = 0; i < NUM_U; i++) {
        float u = (float)i * (2.0f * (float)M_PI / (float)NUM_U);
        float sin_u = sinf(u);
        float cos_u = cosf(u);

        // Parametric Heart 2D Equation
        // x = 16 * sin^3(u)
        // y = 13 * cos(u) - 5 * cos(2u) - 2 * cos(3u) - cos(4u)
        float sin3_u = sin_u * sin_u * sin_u;
        float hx = 16.0f * sin3_u;
        float hy = 13.0f * cosf(u) - 5.0f * cosf(2.0f * u) - 2.0f * cosf(3.0f * u) - cosf(4.0f * u);

        // Audio waveform ripple deformation along the heart profile
        size_t sample_idx = (i * 6) % (n > 0 ? n : 1);
        float wave_mod = 0.0f;
        if (s_peak_l > 0) {
            wave_mod = ((float)left[sample_idx] / (float)s_peak_l) * (2.2f + s_heart3d_vol * 3.2f);
        }

        for (int j = 0; j < NUM_V; j++) {
            // Latitude angle from -PI/2 (back cap) to +PI/2 (front cap)
            float v = -(float)M_PI / 2.0f + (float)j * ((float)M_PI / (float)(NUM_V - 1));
            float cos_v = cosf(v);
            float sin_v = sinf(v);

            // 3D Heart coordinates with audio wave modulation
            float r_factor = cos_v;
            float x0 = (hx * r_factor + wave_mod * cos_u) * base_scale;
            float y0 = (hy * r_factor + wave_mod * sin_u) * base_scale;
            float z0 = (12.0f * sin_v + wave_mod * 0.8f) * base_scale;

            // 3D Rotations: X -> Y -> Z
            // 1. Rotate around X
            float y1 = y0 * cos_x - z0 * sin_x;
            float z1 = y0 * sin_x + z0 * cos_x;
            float x1 = x0;

            // 2. Rotate around Y
            float x2 = x1 * cos_y + z1 * sin_y;
            float z2 = -x1 * sin_y + z1 * cos_y;
            float y2 = y1;

            // 3. Rotate around Z
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

    // Draw Heart Perimeter Ribs (along u for each depth ring v)
    for (int j = 0; j < NUM_V; j++) {
        for (int i = 0; i < NUM_U; i++) {
            int next_i = (i + 1) % NUM_U;
            SafeDraw::drawLine(proj_x[i][j], proj_y[i][j], proj_x[next_i][j], proj_y[next_i][j]);
        }
    }

    // Draw Longitudinal Depth Lines (along v across depths)
    for (int i = 0; i < NUM_U; i += 2) { // Every 2nd meridian for clean wireframe look
        for (int j = 0; j < NUM_V - 1; j++) {
            SafeDraw::drawLine(proj_x[i][j], proj_y[i][j], proj_x[i][j + 1], proj_y[i][j + 1]);
        }
    }

    // Pulsing central heartbeat core
    if (s_heart3d_vol > 0.35f) {
        int core_r = (int)(s_heart3d_vol * 3.5f);
        SafeDraw::drawDisc(cx, cy, core_r);
    }
}
