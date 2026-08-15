#include "effects.h"

// -----------------------------------------------------------------------
// MODE 49 — 3D AUDIO CYLINDER (Perspective Wireframe Audio Tube)
// -----------------------------------------------------------------------

#define CYL_RINGS 6
#define CYL_SEGS  16

struct CylRing {
    float z;
};

static CylRing s_rings[CYL_RINGS];
static float s_cyl_rot = 0.0f;
static float s_cyl_vol = 0.0f;

void effect_cylinder3d_on_enter() {
    s_cyl_rot = 0.0f;
    s_cyl_vol = 0.0f;
    for (int i = 0; i < CYL_RINGS; i++) {
        s_rings[i].z = 15.0f + (float)i * 12.0f;
    }
}

void effect_cylinder3d_on_exit() {}

void effect_cylinder3d_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = SCREEN_W / 2;
    const int cy = SCREEN_H / 2;

    // 1. Audio volume measurement
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += abs(left[i]) + abs(right[i]);
    }
    float vol = (float)sum / (float)(n * (s_peak_l + s_peak_r + 1));
    if (vol > 1.0f) vol = 1.0f;
    s_cyl_vol = s_cyl_vol * 0.7f + vol * 0.3f;

    // Cylinder scroll speed & rotation
    float scroll_speed = 0.6f + s_cyl_vol * 2.2f;
    s_cyl_rot += 0.02f + s_cyl_vol * 0.04f;

    // Update ring Z depth (fly toward camera)
    for (int r = 0; r < CYL_RINGS; r++) {
        s_rings[r].z -= scroll_speed;
        if (s_rings[r].z < 10.0f) {
            s_rings[r].z += (float)CYL_RINGS * 12.0f;
        }
    }

    float fov = 48.0f;
    float base_radius = 18.0f + s_cyl_vol * 6.0f;

    int proj_x[CYL_RINGS][CYL_SEGS];
    int proj_y[CYL_RINGS][CYL_SEGS];

    // Compute 3D points
    for (int r = 0; r < CYL_RINGS; r++) {
        float z = s_rings[r].z;
        if (z < 1.0f) z = 1.0f;

        for (int s = 0; s < CYL_SEGS; s++) {
            float ang = (float)s * (2.0f * (float)M_PI / (float)CYL_SEGS) + s_cyl_rot;

            // Audio wave distortion along cylinder perimeter
            size_t sample_idx = ((r * CYL_SEGS + s) * 3) % (n > 0 ? n : 1);
            float wave_deform = 0.0f;
            if (s_peak_l > 0) {
                wave_deform = ((float)left[sample_idx] / (float)s_peak_l) * (3.0f + s_cyl_vol * 4.0f);
            }

            float current_r = base_radius + wave_deform;
            float x = current_r * cosf(ang);
            float y = current_r * sinf(ang);

            proj_x[r][s] = cx + (int)((x * fov) / z);
            proj_y[r][s] = cy - (int)((y * fov) / z);
        }
    }

    // 2. Draw Ring Polygons
    for (int r = 0; r < CYL_RINGS; r++) {
        for (int s = 0; s < CYL_SEGS; s++) {
            int next_s = (s + 1) % CYL_SEGS;
            SafeDraw::drawLine(proj_x[r][s], proj_y[r][s], proj_x[r][next_s], proj_y[r][next_s]);
        }
    }

    // 3. Draw Longitudinal Ribs (connecting consecutive rings)
    for (int s = 0; s < CYL_SEGS; s += 2) { // Every 2nd rib for clarity
        for (int r = 0; r < CYL_RINGS - 1; r++) {
            // Find which ring is behind which
            SafeDraw::drawLine(proj_x[r][s], proj_y[r][s], proj_x[r + 1][s], proj_y[r + 1][s]);
        }
    }

    // Center portal beacon
    if (s_cyl_vol > 0.35f) {
        int r = (int)(s_cyl_vol * 4.0f);
        SafeDraw::drawDisc(cx, cy, r);
    }
}
