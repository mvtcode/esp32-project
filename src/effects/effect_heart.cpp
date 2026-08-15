#include "effects.h"

// -----------------------------------------------------------------------
// MODE 7 — MVT HEART  (Heart stereo visualizer with "MVT" text)
// -----------------------------------------------------------------------
#define HEART_BARS 15
static float s_heart_scale = 1.0f;

void effect_heart_on_enter() {
    s_heart_scale = 1.0f;
}

void effect_heart_on_exit() {
    s_heart_scale = 1.0f;
}

static void draw_heart(int hx, int hy, int pulse) {
    int r = 5 + pulse;
    // Top two circular lobes
    SafeDraw::drawDisc(hx - 5, hy - 4, r);
    SafeDraw::drawDisc(hx + 5, hy - 4, r);
    // Center bridge
    SafeDraw::drawBox(hx - 5 - r, hy - 4, 11 + 2 * r, 5 + pulse);
    // Bottom triangle tapering down
    SafeDraw::drawTriangle(hx - 5 - r, hy, hx + 5 + r, hy, hx, hy + 12 + pulse * 2);
}

void effect_heart_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cy = 27;           // Lowered baseline for larger wave clearance
    const int hx = 64;
    const int hy = 26;
    const int MAX_H = 22;        // Increased max wave height for larger effect

    float bands_l[HEART_BARS] = {0};
    float bands_r[HEART_BARS] = {0};

    // 1. FFT on Left Channel
    for (size_t i = 0; i < n; i++) {
        s_fft_real[i] = (float)left[i];
        s_fft_imag[i] = 0.0f;
    }
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();

    float max_mag = 1.0f;
    for (int k = 0; k < HEART_BARS; k++) {
        float sum = 0.0f;
        int start_bin = 1 + k * 3;
        for (int b = 0; b < 3 && (start_bin + b) < (int)(n / 2); b++) {
            sum += s_fft_real[start_bin + b];
        }
        bands_l[k] = sum / 3.0f;
        if (bands_l[k] > max_mag) max_mag = bands_l[k];
    }

    // 2. FFT on Right Channel
    for (size_t i = 0; i < n; i++) {
        s_fft_real[i] = (float)right[i];
        s_fft_imag[i] = 0.0f;
    }
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();

    for (int k = 0; k < HEART_BARS; k++) {
        float sum = 0.0f;
        int start_bin = 1 + k * 3;
        for (int b = 0; b < 3 && (start_bin + b) < (int)(n / 2); b++) {
            sum += s_fft_real[start_bin + b];
        }
        bands_r[k] = sum / 3.0f;
        if (bands_r[k] > max_mag) max_mag = bands_r[k];
    }

    // Dynamic AGC
    s_heart_scale = max_mag > s_heart_scale ? max_mag : s_heart_scale * 0.95f;
    if (s_heart_scale < 1.0f) s_heart_scale = 1.0f;

    // Center baseline lines leading to heart
    SafeDraw::drawHLine(0, cy, 53);
    SafeDraw::drawHLine(75, cy, 53);

    // 3. Draw Left symmetrical bars (Bass near heart, Treble far from heart)
    for (int k = 0; k < HEART_BARS; k++) {
        int band_idx = (HEART_BARS - 1) - k;  // k=0 (far left) is Treble, k=14 (near heart) is Bass
        float norm_l = bands_l[band_idx] / s_heart_scale;
        if (norm_l > 1.0f) norm_l = 1.0f;
        int h = (int)(powf(norm_l, 0.60f) * MAX_H);
        if (h < 1) h = 1;
        if (h > MAX_H) h = MAX_H;

        int x = 6 + k * 3;
        SafeDraw::drawVLine(x, cy - h, 2 * h + 1);
        SafeDraw::drawVLine(x + 1, cy - h, 2 * h + 1);
    }

    // 4. Draw Right symmetrical bars (Bass near heart, Treble far from heart)
    for (int k = 0; k < HEART_BARS; k++) {
        float norm_r = bands_r[k] / s_heart_scale;
        if (norm_r > 1.0f) norm_r = 1.0f;
        int h = (int)(powf(norm_r, 0.60f) * MAX_H);
        if (h < 1) h = 1;
        if (h > MAX_H) h = MAX_H;

        int x = 78 + k * 3;
        SafeDraw::drawVLine(x, cy - h, 2 * h + 1);
        SafeDraw::drawVLine(x + 1, cy - h, 2 * h + 1);
    }

    // 5. Dynamic beating Heart in center (pulses with bass)
    int pulse = ((bands_l[0] + bands_r[0]) / (2.0f * s_heart_scale)) > 0.60f ? 1 : 0;
    draw_heart(hx, hy, pulse);

    // 6. Centered "MVT" text (scaled down to 2/3)
    SafeDraw::setFont(u8g2_font_6x10_tf);
    int tw = SafeDraw::getStrWidth("MVT");
    SafeDraw::drawStr(hx - tw / 2, 60, "MVT");

    // 7. Channel labels
    SafeDraw::setFont(u8g2_font_04b_03_tr);
    SafeDraw::drawStr(1, 10, "L");
    SafeDraw::drawStr(123, 10, "R");
}
