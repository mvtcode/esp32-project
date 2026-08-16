#include "effects.h"

// -----------------------------------------------------------------------
// MODE 1 — MIRROR  (symmetric vertical bars from center — mono mix)
// -----------------------------------------------------------------------
void effect_mirror_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cy     = 31;
    const int half   = 29;
    const int32_t pk = s_peak_l > s_peak_r ? s_peak_l : s_peak_r;

    // Draw center baseline
    SafeDraw::drawHLine(0, cy, SCREEN_W);

    for (int x = 0; x < SCREEN_W && x < (int)n; x++) {
        // Mono mix, avoid overflow
        int32_t sample = left[x] / 2 + right[x] / 2;
        int h = (int)(fabsf((float)sample / (float)pk) * half);
        if (h > half) h = half;
        if (h > 0) {
            SafeDraw::drawVLine(x, cy - h, h * 2 + 1);
        }
    }
}
