#include "effects.h"

// -----------------------------------------------------------------------
// MODE 14 — MVT TERRAIN (3D Historical Waterfall Spectrum Mesh)
// -----------------------------------------------------------------------
#define TERRAIN_DEPTH 6
#define TERRAIN_BINS  14
static float s_terrain_mesh[TERRAIN_DEPTH][TERRAIN_BINS] = {{0}};

void effect_terrain_on_enter() {
    memset(s_terrain_mesh, 0, sizeof(s_terrain_mesh));
}

void effect_terrain_on_exit() {
    memset(s_terrain_mesh, 0, sizeof(s_terrain_mesh));
}

void effect_terrain_render(const int32_t *left, const int32_t *right, size_t n) {
    // 1. FFT for new front row
    // Note: s_fft_real already contains mono-mix magnitudes computed by audio_compute_bands in display.cpp

    // Shift history backward
    for (int z = TERRAIN_DEPTH - 1; z > 0; z--) {
        for (int b = 0; b < TERRAIN_BINS; b++) {
            s_terrain_mesh[z][b] = s_terrain_mesh[z - 1][b];
        }
    }

    // Store new spectrum in front row (z=0)
    for (int b = 0; b < TERRAIN_BINS; b++) {
        float sum = 0.0f;
        for (int k = 0; k < 3; k++) sum += s_fft_real[1 + b * 3 + k];
        float norm = (sum / 3.0f) / (float)s_peak_l;
        if (norm > 1.0f) norm = 1.0f;
        s_terrain_mesh[0][b] = powf(norm, 0.60f) * 16.0f;
    }

    // 2. Draw 3D Perspective Wireframe Mesh
    for (int z = TERRAIN_DEPTH - 1; z >= 0; z--) {
        float z_scale = 1.0f - z * 0.12f;
        int y_base = 58 - z * 8;

        for (int b = 0; b < TERRAIN_BINS; b++) {
            int x1 = 64 + (int)(((b - (TERRAIN_BINS / 2)) * 8.5f) * z_scale);
            int y1 = y_base - (int)(s_terrain_mesh[z][b] * z_scale);

            // Connect to next bin horizontally
            if (b < TERRAIN_BINS - 1) {
                int x2 = 64 + (int)(((b + 1 - (TERRAIN_BINS / 2)) * 8.5f) * z_scale);
                int y2 = y_base - (int)(s_terrain_mesh[z][b + 1] * z_scale);
                SafeDraw::drawLine(x1, y1, x2, y2);
            }

            // Connect to slice behind longitudinally
            if (z < TERRAIN_DEPTH - 1) {
                float z_next = 1.0f - (z + 1) * 0.12f;
                int y_next_base = 58 - (z + 1) * 8;
                int x_next = 64 + (int)(((b - (TERRAIN_BINS / 2)) * 8.5f) * z_next);
                int y_next = y_next_base - (int)(s_terrain_mesh[z + 1][b] * z_next);
                SafeDraw::drawLine(x1, y1, x_next, y_next);
            }
        }
    }

    // Horizon line & Title
    SafeDraw::drawHLine(10, 14, 108);
    SafeDraw::setFont(u8g2_font_04b_03_tr);
    SafeDraw::drawStr(50, 10, "MVT 3D MESH");
}
