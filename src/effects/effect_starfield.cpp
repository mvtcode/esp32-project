#include "effects.h"

// -----------------------------------------------------------------------
// MODE 42 — 3D WARP STARFIELD (Hyper-speed Space Travel with Audio Boost)
// -----------------------------------------------------------------------

#define STAR_COUNT 75

struct Star3D {
    float x, y, z;
    float prev_z;
};

static Star3D s_stars[STAR_COUNT];
static float s_star_vol = 0.0f;
static float s_warp_streak = 0.0f;

static void reset_star(Star3D &s, bool initial_spread) {
    s.x = (float)((rand() % 240) - 120);
    s.y = (float)((rand() % 140) - 70);
    if (fabsf(s.x) < 5.0f && fabsf(s.y) < 5.0f) {
        s.x = (rand() % 2 ? 1 : -1) * (10.0f + rand() % 50);
    }
    s.z = initial_spread ? (float)(10 + rand() % 190) : 200.0f;
    s.prev_z = s.z;
}

void effect_starfield_on_enter() {
    s_star_vol = 0.0f;
    s_warp_streak = 0.0f;
    for (int i = 0; i < STAR_COUNT; i++) {
        reset_star(s_stars[i], true);
    }
}

void effect_starfield_on_exit() {}

void effect_starfield_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = SCREEN_W / 2;
    const int cy = SCREEN_H / 2;

    // 1. Audio volume measurement
    int64_t sum = 0;
    int64_t sum_l = 0, sum_r = 0;
    for (size_t i = 0; i < n; i++) {
        int32_t al = abs(left[i]);
        int32_t ar = abs(right[i]);
        sum_l += al;
        sum_r += ar;
        sum += (al + ar);
    }
    float vol = (float)sum / (float)(n * (s_peak_l + s_peak_r + 1));
    if (vol > 1.0f) vol = 1.0f;
    s_star_vol = s_star_vol * 0.7f + vol * 0.3f;

    // Stereo balance tilt
    float balance = 0.0f;
    if (sum_l + sum_r > 0) {
        balance = (float)(sum_r - sum_l) / (float)(sum_l + sum_r);
    }

    // Warp speed boosted by audio volume
    float speed = 2.0f + s_star_vol * 14.0f;
    s_warp_streak = s_warp_streak * 0.8f + (s_star_vol > 0.35f ? 1.0f : 0.0f) * 0.2f;

    float fov = 64.0f;

    // Center focal point with subtle stereo vibration
    int center_x = cx + (int)(balance * 12.0f);
    int center_y = cy;

    for (int i = 0; i < STAR_COUNT; i++) {
        Star3D &s = s_stars[i];
        s.prev_z = s.z;
        s.z -= speed;

        if (s.z <= 2.0f) {
            reset_star(s, false);
            continue;
        }

        // 3D Perspective Projection
        int sx = center_x + (int)((s.x * fov) / s.z);
        int sy = center_y + (int)((s.y * fov) / s.z);

        // Previous position for warp streak line
        int px = center_x + (int)((s.x * fov) / s.prev_z);
        int py = center_y + (int)((s.y * fov) / s.prev_z);

        // Check bounds
        if (sx < 0 || sx >= SCREEN_W || sy < 0 || sy >= SCREEN_H) {
            reset_star(s, false);
            continue;
        }

        // Draw warp trail when high speed or close to camera
        if (s_star_vol > 0.25f || s.z < 60.0f) {
            SafeDraw::drawLine(px, py, sx, sy);
            if (s.z < 40.0f) {
                SafeDraw::drawPixel(sx + 1, sy);
                SafeDraw::drawPixel(sx, sy + 1);
            }
        } else {
            SafeDraw::drawPixel(sx, sy);
        }
    }

    // Subtle crosshair/speed HUD ring at center when warp is high
    if (s_star_vol > 0.4f) {
        int r = (int)(s_star_vol * 10.0f);
        SafeDraw::drawCircle(center_x, center_y, r);
    }
}
