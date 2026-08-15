#include "effects.h"

// -----------------------------------------------------------------------
// MODE 45 — 3D DNA DOUBLE HELIX (Audio-Reactive Genomic Spiral)
// -----------------------------------------------------------------------

static float s_dna_rot = 0.0f;
static float s_dna_tilt = 0.25f;
static float s_dna_vol = 0.0f;

void effect_dna_helix_on_enter() {
    s_dna_rot = 0.0f;
    s_dna_vol = 0.0f;
}

void effect_dna_helix_on_exit() {}

void effect_dna_helix_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = SCREEN_W / 2;
    const int cy = SCREEN_H / 2;

    // 1. Audio volume measurement
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += abs(left[i]) + abs(right[i]);
    }
    float vol = (float)sum / (float)(n * (s_peak_l + s_peak_r + 1));
    if (vol > 1.0f) vol = 1.0f;
    s_dna_vol = s_dna_vol * 0.7f + vol * 0.3f;

    // Rotation speed driven by audio
    s_dna_rot += 0.045f + s_dna_vol * 0.08f;

    const int NUM_RUNGS = 22;
    float helix_len = 110.0f;
    float base_radius = 14.0f + s_dna_vol * 7.0f;
    float spatial_freq = 0.065f;

    float cos_tilt = cosf(s_dna_tilt), sin_tilt = sinf(s_dna_tilt);
    float cam_dist = 85.0f;
    float fov = 75.0f;

    auto project_point = [&](float x, float y, float z, int &sx, int &sy, float &depth) {
        // Tilt slightly in 3D around X axis
        float y1 = y * cos_tilt - z * sin_tilt;
        float z1 = y * sin_tilt + z * cos_tilt;

        float z_cam = cam_dist - z1;
        if (z_cam < 1.0f) z_cam = 1.0f;
        depth = z1;

        sx = cx + (int)((x * fov) / z_cam);
        sy = cy - (int)((y1 * fov) / z_cam);
    };

    int prev_s1_x = 0, prev_s1_y = 0;
    int prev_s2_x = 0, prev_s2_y = 0;

    for (int i = 0; i < NUM_RUNGS; i++) {
        float x = -helix_len * 0.5f + ((float)i / (float)(NUM_RUNGS - 1)) * helix_len;
        float angle = x * spatial_freq + s_dna_rot;

        // Audio frequency response per rung (Bass on left -> Treble on right)
        size_t sample_idx = (i * 5) % (n > 0 ? n : 1);
        float rung_amp = 0.0f;
        if (s_peak_l > 0) {
            rung_amp = (fabsf((float)left[sample_idx]) / (float)s_peak_l) * (4.0f + s_dna_vol * 6.0f);
        }

        float r = base_radius + rung_amp;

        // Strand 1 position
        float y1 = r * sinf(angle);
        float z1 = r * cosf(angle);

        // Strand 2 position (180 deg offset)
        float y2 = -y1;
        float z2 = -z1;

        int s1_x, s1_y, s2_x, s2_y;
        float d1, d2;
        project_point(x, y1, z1, s1_x, s1_y, d1);
        project_point(x, y2, z2, s2_x, s2_y, d2);

        // Draw connecting base-pair rung
        if (i % 2 == 0) {
            // Solid ladder rung
            SafeDraw::drawLine(s1_x, s1_y, s2_x, s2_y);

            // Base pair node dots
            if (d1 > 0) SafeDraw::drawDisc(s1_x, s1_y, 1);
            if (d2 > 0) SafeDraw::drawDisc(s2_x, s2_y, 1);
        } else {
            // Dotted/middle marker rung on high volume
            if (s_dna_vol > 0.25f) {
                int mid_x = (s1_x + s2_x) / 2;
                int mid_y = (s1_y + s2_y) / 2;
                SafeDraw::drawPixel(mid_x, mid_y);
            }
        }

        // Draw backbone strand lines
        if (i > 0) {
            SafeDraw::drawLine(prev_s1_x, prev_s1_y, s1_x, s1_y);
            SafeDraw::drawLine(prev_s2_x, prev_s2_y, s2_x, s2_y);
        }

        prev_s1_x = s1_x;
        prev_s1_y = s1_y;
        prev_s2_x = s2_x;
        prev_s2_y = s2_y;
    }
}
