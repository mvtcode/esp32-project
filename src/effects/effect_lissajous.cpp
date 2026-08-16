#include "effects.h"

// -----------------------------------------------------------------------
// MODE 3 — LISSAJOUS  (X=L, Y=R scatter plot)
// -----------------------------------------------------------------------
void effect_lissajous_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 63, cy = 31;
    const int rx = 58, ry = 27; // draw radius

    // SafeDraw::drawFrame(2, 2, 124, 60); // border

    for (size_t i = 0; i < n; i++) {
        int x = cx + (int)((float)left[i]  / s_peak_l * rx);
        int y = cy - (int)((float)right[i] / s_peak_r * ry);
        SafeDraw::drawPixel(x, y);
    }

    // Crosshairs (faint reference lines)
    // SafeDraw::drawPixel(cx, 3);
    // SafeDraw::drawPixel(cx, 60);
    // SafeDraw::drawPixel(3,  cy);
    // SafeDraw::drawPixel(124, cy);
}
