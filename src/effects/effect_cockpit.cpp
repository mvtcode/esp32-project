#include "effects.h"

// -----------------------------------------------------------------------
// MODE 55 — MVT CYBER COCKPIT (Sci-Fi Spaceship HUD, Stereo Radar & Warp Speed)
// -----------------------------------------------------------------------
struct WarpStar {
    float x, y, z;
};

static WarpStar s_warp_stars[32];
static float s_radar_ang = 0.0f;

void effect_cockpit_on_enter() {
    s_radar_ang = 0.0f;
    for (int i = 0; i < 32; i++) {
        s_warp_stars[i].x = (float)(rand() % 160 - 80);
        s_warp_stars[i].y = (float)(rand() % 100 - 50);
        s_warp_stars[i].z = 10.0f + (float)(rand() % 90);
    }
}

void effect_cockpit_on_exit() {}

void effect_cockpit_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 64, cy = 28;

    // 1. RMS & Stereo
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

    // Center aim sway based on stereo
    int aim_x = cx + (int)((norm_r - norm_l) * 16.0f);
    int aim_y = cy;

    // 2. Warp Stars in 3D Perspective
    float warp_spd = 1.5f + avg * 6.0f;
    for (int i = 0; i < 32; i++) {
        s_warp_stars[i].z -= warp_spd;
        if (s_warp_stars[i].z <= 2.0f) {
            s_warp_stars[i].x = (float)(rand() % 160 - 80);
            s_warp_stars[i].y = (float)(rand() % 100 - 50);
            s_warp_stars[i].z = 90.0f;
        }

        float k = 40.0f / s_warp_stars[i].z;
        int px = aim_x + (int)(s_warp_stars[i].x * k);
        int py = aim_y + (int)(s_warp_stars[i].y * k);

        // Calculate tail streak
        float prev_k = 40.0f / (s_warp_stars[i].z + warp_spd * 1.5f);
        int tx = aim_x + (int)(s_warp_stars[i].x * prev_k);
        int ty = aim_y + (int)(s_warp_stars[i].y * prev_k);

        SafeDraw::drawLine(px, py, tx, ty);
    }

    // 3. Center HUD Target Reticle
    SafeDraw::drawCircle(aim_x, aim_y, 8);
    SafeDraw::drawLine(aim_x - 12, aim_y, aim_x - 9, aim_y);
    SafeDraw::drawLine(aim_x + 9, aim_y, aim_x + 12, aim_y);
    SafeDraw::drawLine(aim_x, aim_y - 12, aim_x, aim_y - 9);
    SafeDraw::drawLine(aim_x, aim_y + 9, aim_x, aim_y + 12);
    SafeDraw::drawPixel(aim_x, aim_y);

    // 4. Cockpit Canopy Canopy Frame
    SafeDraw::drawLine(0, 0, 32, 14);
    SafeDraw::drawLine(127, 0, 95, 14);
    SafeDraw::drawLine(0, 63, 28, 48);
    SafeDraw::drawLine(127, 63, 99, 48);
    SafeDraw::drawHLine(28, 48, 72);

    // 5. Left Radar (Stereo L)
    s_radar_ang += 0.10f;
    if (s_radar_ang > 6.28318f) s_radar_ang -= 6.28318f;
    int rx = 14, ry = 55;
    SafeDraw::drawCircle(rx, ry, 7);
    int blip_x = rx + (int)(cosf(s_radar_ang) * (norm_l * 6.0f));
    int blip_y = ry + (int)(sinf(s_radar_ang) * (norm_l * 6.0f));
    SafeDraw::drawLine(rx, ry, blip_x, blip_y);

    // 6. Right Reactor Gauge (Stereo R)
    int gx = 108, gy = 50;
    SafeDraw::drawFrame(gx, gy, 18, 10);
    int bar_w = (int)(norm_r * 14.0f);
    if (bar_w > 14) bar_w = 14;
    SafeDraw::drawBox(gx + 2, gy + 2, bar_w, 6);

    // 7. HUD Telemetry
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(46, 57, "WARP 9.8");
    SafeDraw::drawStr(44, 63, "MVT-PILOT");
}
