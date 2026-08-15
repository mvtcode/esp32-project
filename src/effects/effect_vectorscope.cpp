#include "effects.h"

// -----------------------------------------------------------------------
// MODE 32 — VECTORSCOPE (Goniometer Polar Audio Vectorscope - M/S Rotated)
// -----------------------------------------------------------------------

void effect_vectorscope_on_enter() {}
void effect_vectorscope_on_exit() {}

void effect_vectorscope_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 63, cy = 31;
    const int radius = 28;

    // 1. Draw Professional Graticule & HUD
    SafeDraw::drawCircle(cx, cy, radius);
    SafeDraw::drawCircle(cx, cy, radius / 2);

    // Crosshairs
    SafeDraw::drawHLine(cx - radius - 2, cy, (radius + 2) * 2 + 1);
    SafeDraw::drawVLine(cx, cy - radius - 2, (radius + 2) * 2 + 1);

    // Diagonal 45-degree guideline ticks (L and R axes)
    const int diag = 20; // 28 * 0.707
    SafeDraw::drawLine(cx - diag, cy - diag, cx + diag, cy + diag);
    SafeDraw::drawLine(cx - diag, cy + diag, cx + diag, cy - diag);

    // Labels
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(cx - 3, cy - radius - 4, "+M");
    SafeDraw::drawStr(cx + radius + 4, cy + 2, "+S");
    SafeDraw::drawStr(cx - radius - 12, cy + 2, "-S");
    SafeDraw::drawStr(2, 8, "V-SCOPE");
    SafeDraw::drawStr(106, 8, "STEREO");

    // 2. Audio vector plotting (Mid/Side 45-degree rotated transformation)
    // X = (L - R) / sqrt(2), Y = (L + R) / sqrt(2)
    int prev_x = -1, prev_y = -1;
    float inv_peak_l = 1.0f / (float)s_peak_l;
    float inv_peak_r = 1.0f / (float)s_peak_r;

    for (size_t i = 0; i < n; i++) {
        float norm_l = (float)left[i] * inv_peak_l;
        float norm_r = (float)right[i] * inv_peak_r;

        // Rotate 45 deg: Side (horizontal) = (L - R) * 0.707, Mid (vertical) = (L + R) * 0.707
        float side = (norm_l - norm_r) * 0.7071f;
        float mid  = (norm_l + norm_r) * 0.7071f;

        int px = cx + (int)(side * (float)radius);
        int py = cy - (int)(mid  * (float)radius);

        if (prev_x >= 0) {
            // Draw line between consecutive samples if distance is reasonable
            int dx = abs(px - prev_x);
            int dy = abs(py - prev_y);
            if (dx + dy <= 16) {
                SafeDraw::drawLine(prev_x, prev_y, px, py);
            } else {
                SafeDraw::drawPixel(px, py);
            }
        } else {
            SafeDraw::drawPixel(px, py);
        }
        prev_x = px;
        prev_y = py;
    }
}
