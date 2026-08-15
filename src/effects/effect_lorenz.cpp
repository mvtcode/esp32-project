#include "effects.h"

// -----------------------------------------------------------------------
// MODE 35 — LORENZ 3D (Chaos Theory Strange Attractor with 3D Projection)
// -----------------------------------------------------------------------

#define LORENZ_TRAIL_LEN 80

struct LorenzPoint {
    float x, y, z;
};

static LorenzPoint s_trail[LORENZ_TRAIL_LEN];
static int s_head_idx = 0;
static float s_lx = 0.1f, s_ly = 0.0f, s_lz = 0.0f;
static float s_rot_yaw = 0.0f;
static float s_rot_pitch = 0.3f;
static float s_lorenz_vol = 0.0f;

void effect_lorenz_on_enter() {
    s_lx = 0.1f;
    s_ly = 0.0f;
    s_lz = 0.0f;
    s_head_idx = 0;
    for (int i = 0; i < LORENZ_TRAIL_LEN; i++) {
        s_trail[i] = { 0.1f, 0.0f, 0.0f };
    }
    s_rot_yaw = 0.0f;
    s_rot_pitch = 0.35f;
    s_lorenz_vol = 0.0f;
}

void effect_lorenz_on_exit() {}

void effect_lorenz_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 63, cy = 31;

    // 1. Audio volume measurement
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += abs(left[i]) + abs(right[i]);
    }
    float vol = (float)sum / (float)(n * (s_peak_l + s_peak_r + 1));
    s_lorenz_vol = s_lorenz_vol * 0.7f + vol * 0.3f;

    // 2. Lorenz differential integration
    const float sigma = 10.0f;
    const float beta  = 8.0f / 3.0f;
    // Bass pulse kicks Rayleigh parameter rho
    float rho = 24.0f + s_lorenz_vol * 14.0f;
    float dt = 0.009f + s_lorenz_vol * 0.005f;

    // Step physics 8 times per frame for smoothness
    for (int step = 0; step < 8; step++) {
        float dx = sigma * (s_ly - s_lx);
        float dy = s_lx * (rho - s_lz) - s_ly;
        float dz = s_lx * s_ly - beta * s_lz;

        s_lx += dx * dt;
        s_ly += dy * dt;
        s_lz += dz * dt;

        // Safety clamp against numerical overflow
        if (s_lx > 60.0f) s_lx = 60.0f;
        if (s_lx < -60.0f) s_lx = -60.0f;
        if (s_ly > 60.0f) s_ly = 60.0f;
        if (s_ly < -60.0f) s_ly = -60.0f;
        if (s_lz > 80.0f) s_lz = 80.0f;
        if (s_lz < 0.0f) s_lz = 0.0f;
    }

    // Save to rolling trail buffer
    s_trail[s_head_idx] = { s_lx, s_ly, s_lz };
    s_head_idx = (s_head_idx + 1) % LORENZ_TRAIL_LEN;

    // Advance 3D orbit rotation
    s_rot_yaw += 0.02f + s_lorenz_vol * 0.03f;
    if (s_rot_yaw > 6.2831853f) s_rot_yaw -= 6.2831853f;

    float cos_y = cosf(s_rot_yaw);
    float sin_y = sinf(s_rot_yaw);
    float cos_p = cosf(s_rot_pitch);
    float sin_p = sinf(s_rot_pitch);

    // 3. Render 3D Lorenz Trail
    int prev_sx = -1, prev_sy = -1;

    for (int i = 0; i < LORENZ_TRAIL_LEN; i++) {
        int idx = (s_head_idx + i) % LORENZ_TRAIL_LEN;
        // Center Z around mean ~ 25
        float px = s_trail[idx].x;
        float py = s_trail[idx].y;
        float pz = s_trail[idx].z - 25.0f;

        // 3D rotation: Yaw around Y axis
        float x1 = px * cos_y - pz * sin_y;
        float z1 = px * sin_y + pz * cos_y;

        // Pitch around X axis
        float y2 = py * cos_p - z1 * sin_p;
        float z2 = py * sin_p + z1 * cos_p;

        // Perspective projection
        float dist = 65.0f;
        float fov = 46.0f;
        float z_proj = dist + z2;
        if (z_proj < 15.0f) z_proj = 15.0f;

        int sx = cx + (int)((x1 * fov) / z_proj);
        int sy = cy - (int)((y2 * fov) / z_proj);

        if (prev_sx >= 0 && (abs(sx - prev_sx) + abs(sy - prev_sy) < 30)) {
            SafeDraw::drawLine(prev_sx, prev_sy, sx, sy);
        } else {
            SafeDraw::drawPixel(sx, sy);
        }
        prev_sx = sx;
        prev_sy = sy;

        // Current attractor head dot
        if (i == LORENZ_TRAIL_LEN - 1) {
            SafeDraw::drawDisc(sx, sy, 1);
        }
    }

    // 4. HUD
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 6, "LORENZ 3D");
    SafeDraw::drawStr(102, 6, "CHAOS");
}
