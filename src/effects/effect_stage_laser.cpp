#include "effects.h"

// -----------------------------------------------------------------------
// MODE 29 — MVT STAGE LASER (Dual Overhead Stage Laser Projectors)
// -----------------------------------------------------------------------
static float s_laser_ang1 = 0.0f;
static float s_laser_ang2 = 0.0f;
static float s_sweep_phase = 0.0f;

void effect_stage_laser_on_enter() {
    s_laser_ang1 = 0.0f;
    s_laser_ang2 = 0.0f;
    s_sweep_phase = 0.0f;
}

void effect_stage_laser_on_exit() {}

void effect_stage_laser_render(const int32_t *left, const int32_t *right, size_t n) {
    const int lx1 = 43; // 1/3 width
    const int lx2 = 85; // 2/3 width
    const int ly  = 3;  // Top emitter nozzle

    // 1. Audio and FFT analysis
    for (size_t i = 0; i < n; i++) {
        s_fft_real[i] = (float)((left[i] + right[i]) / 2);
        s_fft_imag[i] = 0.0f;
    }
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();

    float bass_mag = 0.0f;
    for (int b = 1; b <= 4; b++) {
        bass_mag += s_fft_real[b];
    }
    float bass = bass_mag / (4.0f * (float)s_peak_l);
    if (bass > 1.0f) bass = 1.0f;

    // Peak levels
    int32_t pk = s_peak_l > s_peak_r ? s_peak_l : s_peak_r;
    if (pk < 1) pk = 1;

    int32_t cl = 0, cr = 0;
    for (size_t i = 0; i < n; i++) {
        int32_t vl = left[i] < 0 ? -left[i] : left[i];
        int32_t vr = right[i] < 0 ? -right[i] : right[i];
        if (vl > cl) cl = vl;
        if (vr > cr) cr = vr;
    }
    float vol_l = (float)cl / (float)pk;
    float vol_r = (float)cr / (float)pk;
    if (vol_l > 1.0f) vol_l = 1.0f;
    if (vol_r > 1.0f) vol_r = 1.0f;

    // Sweep phase advances
    s_sweep_phase += 0.05f + bass * 0.12f;
    if (s_sweep_phase > 6.2831853f) s_sweep_phase -= 6.2831853f;

    // Target sweep angles for Laser 1 (Left) and Laser 2 (Right)
    // Downwards angle range: ~40 deg to ~140 deg from horizontal (down is 90 deg = 1.57 rad)
    float base_sweep1 = sinf(s_sweep_phase) * 0.75f;
    float base_sweep2 = -sinf(s_sweep_phase + 0.5f) * 0.75f;

    s_laser_ang1 = 1.570796f + base_sweep1 + (vol_l - 0.5f) * 0.4f;
    s_laser_ang2 = 1.570796f + base_sweep2 - (vol_r - 0.5f) * 0.4f;

    // 2. Draw Overhead Laser Rig Brackets
    SafeDraw::drawBox(lx1 - 5, 0, 11, 4);
    SafeDraw::drawBox(lx2 - 5, 0, 11, 4);
    // Overhead truss bar
    SafeDraw::drawHLine(0, 1, SCREEN_W);

    // 3. Draw Stage Floor Horizon & Floor Grid
    SafeDraw::drawHLine(0, 58, SCREEN_W);
    for (int gx = 8; gx < SCREEN_W; gx += 16) {
        SafeDraw::drawVLine(gx, 59, 5);
    }

    // Helper lambda to shoot laser rays from emitter
    auto shoot_laser = [&](int ex, int ey, float center_ang, float vol, int num_rays) {
        float fan_spread = 0.08f + vol * 0.18f + bass * 0.15f;
        int half = num_rays / 2;

        for (int r = -half; r <= half; r++) {
            float ang = center_ang + (float)r * fan_spread;
            float ca = cosf(ang);
            float sa = sinf(ang);

            if (sa <= 0.05f) sa = 0.05f; // Must point downwards

            // Cast ray to bottom floor y = 58 or side walls
            float dist_floor = (58.0f - (float)ey) / sa;
            float target_x = (float)ex + ca * dist_floor;
            float target_y = 58.0f;

            // Check wall collision
            if (target_x < 0.0f) {
                float dist_wall = (0.0f - (float)ex) / ca;
                target_x = 0.0f;
                target_y = (float)ey + sa * dist_wall;
            } else if (target_x >= 127.0f) {
                float dist_wall = (127.0f - (float)ex) / ca;
                target_x = 127.0f;
                target_y = (float)ey + sa * dist_wall;
            }

            int tx = (int)(target_x + 0.5f);
            int ty = (int)(target_y + 0.5f);

            // Draw laser ray
            SafeDraw::drawLine(ex, ey, tx, ty);

            // Draw impact spot on floor
            if (ty >= 57) {
                SafeDraw::drawDisc(tx, 58, r == 0 ? 2 : 1);
            } else {
                SafeDraw::drawPixel(tx, ty);
            }
        }
    };

    // Determine number of fan rays based on music energy
    int rays1 = (vol_l > 0.4f || bass > 0.3f) ? 5 : 3;
    int rays2 = (vol_r > 0.4f || bass > 0.3f) ? 5 : 3;

    shoot_laser(lx1, ly, s_laser_ang1, vol_l, rays1);
    shoot_laser(lx2, ly, s_laser_ang2, vol_r, rays2);

    // 4. Heavy Bass Kick: Cross-Laser Spark Burst at intersection
    if (bass > 0.35f) {
        // Approximate ray intersection
        float d1 = sinf(s_laser_ang1);
        float d2 = sinf(s_laser_ang2);
        if (d1 > 0.1f && d2 > 0.1f) {
            int ix = (lx1 + lx2) / 2 + (int)((vol_l - vol_r) * 15.0f);
            int iy = 26 + (int)(bass * 12.0f);
            if (iy >= 10 && iy <= 54) {
                SafeDraw::drawCircle(ix, iy, 3);
                SafeDraw::drawPixel(ix, iy);
            }
        }
    }

    // 5. Tech HUD & Stage Banner
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 10, "STAGE");
    SafeDraw::drawStr(2, 54, "MVT-LASER");

    // L / R Level bars on bottom
    int bl1 = (int)(vol_l * 20.0f);
    int bl2 = (int)(vol_r * 20.0f);
    SafeDraw::drawFrame(98, 48, 28, 4);
    if (bl1 > 0) SafeDraw::drawBox(99, 49, bl1 > 26 ? 26 : bl1, 2);
    SafeDraw::drawFrame(98, 53, 28, 4);
    if (bl2 > 0) SafeDraw::drawBox(99, 54, bl2 > 26 ? 26 : bl2, 2);
}
