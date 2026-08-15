#include "effects.h"

// -----------------------------------------------------------------------
// MODE 43 — 3D PULSING WIREFRAME SPHERE (Audio-Reactive Globe)
// -----------------------------------------------------------------------

static float s_sphere_yaw = 0.0f;
static float s_sphere_pitch = 0.0f;
static float s_sphere_vol = 0.0f;

void effect_sphere3d_on_enter() {
    s_sphere_yaw = 0.0f;
    s_sphere_pitch = 0.0f;
    s_sphere_vol = 0.0f;
}

void effect_sphere3d_on_exit() {}

void effect_sphere3d_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = SCREEN_W / 2;
    const int cy = SCREEN_H / 2;

    // 1. Audio volume measurement
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += abs(left[i]) + abs(right[i]);
    }
    float vol = (float)sum / (float)(n * (s_peak_l + s_peak_r + 1));
    if (vol > 1.0f) vol = 1.0f;
    s_sphere_vol = s_sphere_vol * 0.7f + vol * 0.3f;

    // Rotation progression
    s_sphere_yaw += 0.035f + s_sphere_vol * 0.06f;
    s_sphere_pitch += 0.018f + s_sphere_vol * 0.03f;

    float cos_yaw = cosf(s_sphere_yaw), sin_yaw = sinf(s_sphere_yaw);
    float cos_pitch = cosf(s_sphere_pitch), sin_pitch = sinf(s_sphere_pitch);

    float base_radius = 21.0f + s_sphere_vol * 10.0f;
    float cam_dist = 60.0f;
    float fov = 55.0f;

    // Transform 3D point (x, y, z) -> projected (sx, sy)
    auto project_3d = [&](float x, float y, float z, int &sx, int &sy, float &out_z) {
        // Rotate around Y (Yaw)
        float x1 = x * cos_yaw + z * sin_yaw;
        float z1 = -x * sin_yaw + z * cos_yaw;
        // Rotate around X (Pitch)
        float y2 = y * cos_pitch - z1 * sin_pitch;
        float z2 = y * sin_pitch + z1 * cos_pitch;

        float z_cam = cam_dist - z2;
        if (z_cam < 1.0f) z_cam = 1.0f;
        out_z = z2;

        sx = cx + (int)((x1 * fov) / z_cam);
        sy = cy - (int)((y2 * fov) / z_cam);
    };

    // 2. Draw 5 Latitude Rings (vĩ tuyến)
    const float lats[] = { -0.8f, -0.4f, 0.0f, 0.4f, 0.8f };
    const int PTS_PER_RING = 18;

    for (int l = 0; l < 5; l++) {
        float sin_lat = lats[l];
        float cos_lat = sqrtf(fmaxf(0.0f, 1.0f - sin_lat * sin_lat));
        float r_lat = base_radius * cos_lat;
        float y_lat = base_radius * sin_lat;

        int prev_x = 0, prev_y = 0;
        int first_x = 0, first_y = 0;

        for (int p = 0; p <= PTS_PER_RING; p++) {
            float phi = (float)p * (2.0f * (float)M_PI / (float)PTS_PER_RING);
            
            // Audio ripple wave along latitude
            size_t sample_idx = (p * 4) % (n > 0 ? n : 1);
            float wave_deform = 0.0f;
            if (s_peak_l > 0) {
                wave_deform = ((float)left[sample_idx] / (float)s_peak_l) * (2.5f + s_sphere_vol * 3.0f);
            }

            float current_r = r_lat + wave_deform;
            float x = current_r * cosf(phi);
            float z = current_r * sinf(phi);

            int sx, sy;
            float pz;
            project_3d(x, y_lat, z, sx, sy, pz);

            if (p == 0) {
                first_x = sx;
                first_y = sy;
            } else {
                SafeDraw::drawLine(prev_x, prev_y, sx, sy);
            }
            prev_x = sx;
            prev_y = sy;
        }
    }

    // 3. Draw 4 Longitude Meridians (kinh tuyến)
    const int PTS_PER_MERIDIAN = 16;
    for (int m = 0; m < 4; m++) {
        float phi_m = (float)m * ((float)M_PI / 4.0f);
        float cos_phi = cosf(phi_m), sin_phi = sinf(phi_m);

        int prev_x = 0, prev_y = 0;
        int first_x = 0, first_y = 0;

        for (int p = 0; p <= PTS_PER_MERIDIAN; p++) {
            float theta = (float)p * (2.0f * (float)M_PI / (float)PTS_PER_MERIDIAN);
            float x = base_radius * sinf(theta) * cos_phi;
            float y = base_radius * cosf(theta);
            float z = base_radius * sinf(theta) * sin_phi;

            int sx, sy;
            float pz;
            project_3d(x, y, z, sx, sy, pz);

            if (p == 0) {
                first_x = sx;
                first_y = sy;
            } else {
                SafeDraw::drawLine(prev_x, prev_y, sx, sy);
            }
            prev_x = sx;
            prev_y = sy;
        }
    }

    // Center pulse core
    if (s_sphere_vol > 0.35f) {
        int core_r = (int)(s_sphere_vol * 4.0f);
        SafeDraw::drawDisc(cx, cy, core_r);
    }
}
