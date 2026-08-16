#include "effects.h"

// -----------------------------------------------------------------------
// MODE 13 — MVT MATRIX (Digital Rain Reactive Stream)
// -----------------------------------------------------------------------
#define MATRIX_COLS 16
static float s_matrix_y[MATRIX_COLS]   = {0};
static float s_matrix_spd[MATRIX_COLS] = {0};

void effect_matrix_on_enter() {
    for (int col = 0; col < MATRIX_COLS; col++) {
        s_matrix_y[col]   = (float)(-(rand() % 32));
        s_matrix_spd[col] = 0.0f; // reset speed to avoid dirty state on re-enter
    }
}

void effect_matrix_on_exit() {
    memset(s_matrix_y, 0, sizeof(s_matrix_y));
}

void effect_matrix_render(const int32_t *left, const int32_t *right, size_t n) {
    // 1. FFT to drive 16 columns
    // Note: s_fft_real already contains mono-mix magnitudes computed by audio_compute_bands in display.cpp

    // 2. Update and draw each column
    for (int col = 0; col < MATRIX_COLS; col++) {
        float mag = s_fft_real[1 + col * 3];
        float norm = mag / (float)s_peak_l;
        if (norm > 1.0f) norm = 1.0f;

        s_matrix_spd[col] = 1.2f + powf(norm, 0.60f) * 5.0f;
        s_matrix_y[col] += s_matrix_spd[col];
        if (s_matrix_y[col] > 74.0f) {
            s_matrix_y[col] = (float)(-(rand() % 16));
        }

        int x = col * 8 + 3;
        int y = (int)s_matrix_y[col];

        // Draw falling trail
        if (y >= 0 && y < 64) {
            SafeDraw::drawDisc(x, y, 1);  // Head pixel
        }
        for (int t = 1; t <= 5; t++) {
            int ty = y - t * 3;
            if (ty >= 0 && ty < 64 && (t % 2 == 0)) {
                SafeDraw::drawPixel(x, ty);
            }
        }

        // Bottom splash
        if (y >= 60 && norm > 0.30f) {
            SafeDraw::drawPixel(x - 1, 62);
            SafeDraw::drawPixel(x + 1, 62);
        }
    }
}
