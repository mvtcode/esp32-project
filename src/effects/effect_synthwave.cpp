#include "effects.h"

// -----------------------------------------------------------------------
// MODE 21 — MVT SYNTHWAVE (80s Neon Sun + 3D Grid + City FFT Skyline)
// -----------------------------------------------------------------------
static float s_grid_offset = 0.0f;
static float s_synth_peak = FFT_MAG_FLOOR;
static float s_building_heights[16] = {0};

void effect_synthwave_on_enter() {
    s_grid_offset = 0.0f;
    s_synth_peak = FFT_MAG_FLOOR;
    for (int i = 0; i < 16; i++) {
        s_building_heights[i] = 0.0f;
    }
}

void effect_synthwave_on_exit() {
    for (int i = 0; i < 16; i++) {
        s_building_heights[i] = 0.0f;
    }
}

void effect_synthwave_render(const int32_t *left, const int32_t *right, size_t n) {
    const int horizon_y = 30;

    // 1. FFT Processing & Adaptive Peak Tracking
    for (size_t i = 0; i < n; i++) {
        s_fft_real[i] = (float)((left[i] + right[i]) / 2);
        s_fft_imag[i] = 0.0f;
    }
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();

    // Adaptive peak for dynamic bouncing
    float max_mag = 0.0f;
    for (int b = 1; b <= 24; b++) {
        if (s_fft_real[b] > max_mag) max_mag = s_fft_real[b];
    }
    s_synth_peak = max_mag > s_synth_peak ? max_mag : (s_synth_peak * 0.96f);
    if (s_synth_peak < FFT_MAG_FLOOR) s_synth_peak = FFT_MAG_FLOOR;

    // Bass detection (bins 1..3)
    float bass = (s_fft_real[1] + s_fft_real[2] + s_fft_real[3]) / (3.0f * s_synth_peak);
    if (bass > 1.0f) bass = 1.0f;

    // Grid scrolling speed (speeds up on bass punch)
    s_grid_offset += 0.035f + bass * 0.07f;
    if (s_grid_offset >= 1.0f) s_grid_offset -= 1.0f;

    // 2. Twinkling Retro Sky Stars
    static const uint8_t STAR_X[6] = { 6, 20, 36, 92, 108, 122 };
    static const uint8_t STAR_Y[6] = { 5, 8, 3, 4, 9, 6 };
    for (int i = 0; i < 6; i++) {
        SafeDraw::drawPixel(STAR_X[i], STAR_Y[i]);
    }

    // 3. Iconic Synthwave Segmented Neon Sun at Horizon (cx=64, cy=30)
    int sun_r = 13 + (int)(bass * 3.5f);
    // Draw solid sun upper hemisphere
    for (int y = horizon_y - sun_r; y <= horizon_y; y++) {
        int dy = horizon_y - y;
        int dx = (int)sqrtf((float)(sun_r * sun_r - dy * dy));
        SafeDraw::drawHLine(64 - dx, y, dx * 2 + 1);
    }

    // Carve out authentic Synthwave dark horizontal slits near the horizon (widening towards bottom)
    SafeDraw::setDrawColor(0);
    SafeDraw::drawHLine(64 - sun_r - 1, horizon_y - 1, (sun_r + 1) * 2 + 1);
    SafeDraw::drawHLine(64 - sun_r - 1, horizon_y - 3, (sun_r + 1) * 2 + 1);
    SafeDraw::drawHLine(64 - sun_r - 1, horizon_y - 6, (sun_r + 1) * 2 + 1);
    if (sun_r >= 13) {
        SafeDraw::drawHLine(64 - sun_r - 1, horizon_y - 10, (sun_r + 1) * 2 + 1);
    }
    SafeDraw::setDrawColor(1);

    // 4. City Skyline FFT Equalizer Buildings
    for (int col = 0; col < 16; col++) {
        int bx = col * 8;
        float mag = s_fft_real[col + 1] / s_synth_peak;
        if (mag > 1.0f) mag = 1.0f;
        int target_h = (int)(powf(mag, 0.75f) * 18.0f);
        if (target_h > 18) target_h = 18;

        // Keep center sun area prominent: lower buildings near center
        if (col >= 6 && col <= 9 && target_h > 6) {
            target_h = 6;
        }

        // Peak decay smoothing
        if ((float)target_h >= s_building_heights[col]) {
            s_building_heights[col] = (float)target_h;
        } else {
            s_building_heights[col] -= 0.8f;
            if (s_building_heights[col] < 0.0f) s_building_heights[col] = 0.0f;
        }

        int bh = (int)s_building_heights[col];
        if (bh >= 2) {
            // 1) Clear solid black interior so building silhouettes cleanly over the sun
            SafeDraw::setDrawColor(0);
            SafeDraw::drawBox(bx + 1, horizon_y - bh, 6, bh);
            SafeDraw::setDrawColor(1);

            // 2) Wireframe building outline
            SafeDraw::drawFrame(bx + 1, horizon_y - bh, 6, bh + 1);

            // 3) Rooftop Antenna on tall buildings
            if (bh > 12) {
                SafeDraw::drawVLine(bx + 3, horizon_y - bh - 3, 3);
                SafeDraw::drawPixel(bx + 3, horizon_y - bh - 4);
            }

            // 4) Lit window matrix
            if (bh > 5)  SafeDraw::drawPixel(bx + 3, horizon_y - bh + 3);
            if (bh > 9)  SafeDraw::drawPixel(bx + 3, horizon_y - bh + 7);
            if (bh > 13) SafeDraw::drawPixel(bx + 3, horizon_y - bh + 11);
        }
    }

    // Horizon line
    SafeDraw::drawHLine(0, horizon_y, 128);

    // 5. 3D Perspective Ground Grid (y = 31..63)
    // Perspective vanishing rays (cleanly spaced, avoiding vanishing point clumping)
    static const int X_GROUND[11] = { -40, -12, 12, 32, 48, 64, 80, 96, 116, 140, 168 };
    for (int i = 0; i < 11; i++) {
        SafeDraw::drawLine(64, horizon_y + 1, X_GROUND[i], 63);
    }

    // Moving horizontal grid lines (logarithmic/quadratic perspective spacing)
    for (int line = 0; line < 6; line++) {
        float t = ((float)line + s_grid_offset) / 6.0f;
        if (t > 1.0f) t -= 1.0f;
        int py = horizon_y + 1 + (int)(t * t * (63 - horizon_y));
        if (py <= 63 && py > horizon_y) {
            SafeDraw::drawHLine(0, py, 128);
        }
    }

    // 6. Retro Synthwave HUD
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 7, "MVT 84");
    SafeDraw::drawStr(98, 7, "OUTRUN");
}
