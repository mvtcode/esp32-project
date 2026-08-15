#include "effects.h"

// -----------------------------------------------------------------------
// MODE 25 — MVT HIGHWAY (Cyber Night Highway & Outrun Retro Car)
// -----------------------------------------------------------------------
static float s_dash_pos = 0.0f;
static float s_car_x = 64.0f;

void effect_highway_on_enter() {
    s_dash_pos = 0.0f;
    s_car_x = 64.0f;
}

void effect_highway_on_exit() {}

void effect_highway_render(const int32_t *left, const int32_t *right, size_t n) {
    const int horiz_y = 20;

    // 1. RMS & Stereo balance
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

    // Speed & Car sway
    s_dash_pos += 0.08f + avg * 0.16f;
    if (s_dash_pos > 1.0f) s_dash_pos -= 1.0f;

    float target_car_x = 64.0f + (norm_r - norm_l) * 22.0f;
    s_car_x += (target_car_x - s_car_x) * 0.2f;

    // 2. Horizon & Distant Mountains / Equalizer Hills
    SafeDraw::drawHLine(0, horiz_y, 128);
    for (int col = 0; col < 16; col++) {
        int x = col * 8;
        int h = (int)(norm_l * 6.0f + sinf(col * 0.5f + s_dash_pos * 3.14f) * 4.0f);
        if (h > 0 && horiz_y - h >= 0) {
            SafeDraw::drawVLine(x, horiz_y - h, h);
            SafeDraw::drawVLine(x + 1, horiz_y - h, h);
        }
    }

    // 3. 3D Perspective Road Edges
    SafeDraw::drawLine(60, horiz_y, 4, 63);   // Left road edge
    SafeDraw::drawLine(68, horiz_y, 124, 63); // Right road edge

    // Roadside Audio Light Poles (Left & Right)
    for (int p = 1; p <= 3; p++) {
        float t = (float)p / 3.0f + (s_dash_pos * 0.33f);
        if (t > 1.0f) t -= 1.0f;
        int py = horiz_y + (int)(t * t * 43.0f);
        int lx = 60 - (int)(t * 56.0f);
        int rx = 68 + (int)(t * 56.0f);
        int pole_h = (int)(t * 12.0f * (1.0f + avg));
        if (py <= 63) {
            // Left pole
            SafeDraw::drawVLine(lx, py - pole_h, pole_h);
            SafeDraw::drawDisc(lx, py - pole_h, 1);
            // Right pole
            SafeDraw::drawVLine(rx, py - pole_h, pole_h);
            SafeDraw::drawDisc(rx, py - pole_h, 1);
        }
    }

    // Moving Dashed Center Line
    for (int d = 1; d <= 4; d++) {
        float t1 = (float)d / 4.0f + (s_dash_pos * 0.25f);
        if (t1 > 1.0f) t1 -= 1.0f;
        float t2 = t1 + 0.08f;
        if (t2 > 1.0f) t2 = 1.0f;
        int y1 = horiz_y + (int)(t1 * t1 * 43.0f);
        int y2 = horiz_y + (int)(t2 * t2 * 43.0f);
        if (y2 <= 63) {
            SafeDraw::drawLine(64, y1, 64, y2);
        }
    }

    // 4. Retro Sports Car (Rear View at bottom)
    int cx = (int)s_car_x;
    int cy = 52;
    // Car Body
    SafeDraw::drawBox(cx - 13, cy, 26, 8);
    // Roof Cabin
    SafeDraw::drawBox(cx - 8, cy - 6, 16, 6);
    // Rear Window (Dark cut)
    SafeDraw::setDrawColor(0);
    SafeDraw::drawBox(cx - 6, cy - 5, 12, 4);
    SafeDraw::setDrawColor(1);
    // Rear Spoiler
    SafeDraw::drawHLine(cx - 15, cy - 2, 30);
    SafeDraw::drawVLine(cx - 12, cy - 2, 2);
    SafeDraw::drawVLine(cx + 12, cy - 2, 2);
    // Glowing Cyber Tail Lights
    SafeDraw::drawBox(cx - 11, cy + 2, 6, 2);
    SafeDraw::drawBox(cx + 5, cy + 2, 6, 2);
    if (avg > 0.4f) {
        // Brake / Afterburner light flare
        SafeDraw::drawPixel(cx - 13, cy + 3);
        SafeDraw::drawPixel(cx + 13, cy + 3);
    }
    // Wheels
    SafeDraw::drawBox(cx - 14, cy + 6, 3, 4);
    SafeDraw::drawBox(cx + 11, cy + 6, 3, 4);

    // 5. HUD Dashboard
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 8, "TURBO");
    SafeDraw::drawStr(100, 8, "MVT 80s");
}
