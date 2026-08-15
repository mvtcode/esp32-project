#include "effects.h"

// -----------------------------------------------------------------------
// MODE 47 — 3D SPIRAL GALAXY (Perspective Cosmic Swarm with Audio Waves)
// -----------------------------------------------------------------------

#define GALAXY_STARS 90

struct GalaxyStar {
    float r;       // Radial distance from core
    float base_ang;// Base polar angle on spiral arm
    float z_height;// Vertical offset relative to galactic plane
    uint8_t arm;   // Spiral arm index (0 or 1)
};

static GalaxyStar s_galaxy_stars[GALAXY_STARS];
static float s_galaxy_rot = 0.0f;
static float s_galaxy_vol = 0.0f;

void effect_galaxy_on_enter() {
    s_galaxy_rot = 0.0f;
    s_galaxy_vol = 0.0f;

    for (int i = 0; i < GALAXY_STARS; i++) {
        GalaxyStar &s = s_galaxy_stars[i];
        s.arm = i % 2;
        // Non-linear radial distribution (denser near core)
        float u = (float)i / (float)GALAXY_STARS;
        s.r = 4.0f + sqrtf(u) * 36.0f;
        // Logarithmic spiral angle: theta = k * log(r)
        s.base_ang = s.r * 0.16f + (s.arm * (float)M_PI) + ((float)(rand() % 40) - 20.0f) * 0.01f;
        // Vertical thickness decreases towards edge
        float max_h = 6.0f * (1.0f - s.r / 42.0f);
        s.z_height = ((float)(rand() % 100) - 50.0f) * 0.02f * fmaxf(1.0f, max_h);
    }
}

void effect_galaxy_on_exit() {}

void effect_galaxy_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = SCREEN_W / 2;
    const int cy = SCREEN_H / 2;

    // 1. Audio volume measurement
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += abs(left[i]) + abs(right[i]);
    }
    float vol = (float)sum / (float)(n * (s_peak_l + s_peak_r + 1));
    if (vol > 1.0f) vol = 1.0f;
    s_galaxy_vol = s_galaxy_vol * 0.7f + vol * 0.3f;

    // Galactic spin rate boosted by audio
    s_galaxy_rot += 0.025f + s_galaxy_vol * 0.06f;

    // 3D Perspective Tilt (Looking at galaxy disc tilted ~55 degrees)
    float pitch = 0.95f; // ~54.4 degrees tilt
    float cos_p = cosf(pitch), sin_p = sinf(pitch);

    float cam_dist = 60.0f;
    float fov = 50.0f;

    // 2. Render Stars
    for (int i = 0; i < GALAXY_STARS; i++) {
        const GalaxyStar &s = s_galaxy_stars[i];

        // Audio ripple wave expanding outward from core
        size_t sample_idx = ((int)s.r * 3) % (n > 0 ? n : 1);
        float wave_amp = 0.0f;
        if (s_peak_l > 0) {
            wave_amp = ((float)left[sample_idx] / (float)s_peak_l) * (1.5f + s_galaxy_vol * 3.0f);
        }

        float current_r = s.r + wave_amp;
        float current_ang = s.base_ang + s_galaxy_rot;

        // Polar to Cartesian in galactic plane (X-Y)
        float x0 = current_r * cosf(current_ang);
        float y0 = current_r * sinf(current_ang);
        float z0 = s.z_height;

        // Tilt 3D around X axis
        float y1 = y0 * cos_p - z0 * sin_p;
        float z1 = y0 * sin_p + z0 * cos_p;
        float x1 = x0;

        // Perspective Projection
        float z_cam = cam_dist - z1;
        if (z_cam < 1.0f) z_cam = 1.0f;

        int sx = cx + (int)((x1 * fov) / z_cam);
        int sy = cy - (int)((y1 * fov) / z_cam);

        // Render point or small disc if near center/front
        if (s.r < 10.0f && s_galaxy_vol > 0.3f) {
            SafeDraw::drawDisc(sx, sy, 1);
        } else {
            SafeDraw::drawPixel(sx, sy);
        }
    }

    // 3. Galactic Supermassive Core
    int core_r = 2 + (int)(s_galaxy_vol * 5.0f);
    SafeDraw::drawDisc(cx, cy, core_r);

    // Outer shockwave ring on high bass
    if (s_galaxy_vol > 0.45f) {
        int ring_r = (int)(s_galaxy_vol * 18.0f);
        SafeDraw::drawCircle(cx, cy, ring_r);
    }
}
