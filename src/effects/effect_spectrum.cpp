#include "effects.h"

// -----------------------------------------------------------------------
// MODE 2 — SPECTRUM (Seamless Stereo Butterfly Spectrum: Bass Center, Treble Flanks)
// -----------------------------------------------------------------------
#define NUM_SPEC_BARS 32

static float s_spec_peak_l = FFT_MAG_FLOOR;
static float s_spec_peak_r = FFT_MAG_FLOOR;
static float s_peak_hold_l[NUM_SPEC_BARS];
static float s_peak_hold_r[NUM_SPEC_BARS];

void effect_spectrum_on_enter() {
    s_spec_peak_l = FFT_MAG_FLOOR;
    s_spec_peak_r = FFT_MAG_FLOOR;
    for (int i = 0; i < NUM_SPEC_BARS; i++) {
        s_peak_hold_l[i] = 0.0f;
        s_peak_hold_r[i] = 0.0f;
    }
}

void effect_spectrum_on_exit() {
    for (int i = 0; i < NUM_SPEC_BARS; i++) {
        s_peak_hold_l[i] = 0.0f;
        s_peak_hold_r[i] = 0.0f;
    }
}

void effect_spectrum_render(const int32_t *left, const int32_t *right, size_t n) {
    const int BAR_B = 63;      // Bottom baseline Y
    const int BAR_MAX_H = 58;  // Max bar height

    // 1. FFT for Left Channel
    for (size_t i = 0; i < n; i++) {
        s_fft_real[i] = (float)left[i];
        s_fft_imag[i] = 0.0f;
    }
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();

    static float mag_l[NUM_SPEC_BARS]; // static: avoid per-frame stack allocation
    float max_l = 0.0f;
    for (int b = 0; b < NUM_SPEC_BARS; b++) {
        int bin_idx = b + 1;
        mag_l[b] = s_fft_real[bin_idx];
        if (mag_l[b] > max_l) max_l = mag_l[b];
    }
    s_spec_peak_l = max_l > s_spec_peak_l ? max_l : (s_spec_peak_l * 0.95f);
    if (s_spec_peak_l < FFT_MAG_FLOOR) s_spec_peak_l = FFT_MAG_FLOOR;

    // 2. FFT for Right Channel
    for (size_t i = 0; i < n; i++) {
        s_fft_real[i] = (float)right[i];
        s_fft_imag[i] = 0.0f;
    }
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();

    static float mag_r[NUM_SPEC_BARS]; // static: avoid per-frame stack allocation
    float max_r = 0.0f;
    for (int b = 0; b < NUM_SPEC_BARS; b++) {
        int bin_idx = b + 1;
        mag_r[b] = s_fft_real[bin_idx];
        if (mag_r[b] > max_r) max_r = mag_r[b];
    }
    s_spec_peak_r = max_r > s_spec_peak_r ? max_r : (s_spec_peak_r * 0.95f);
    if (s_spec_peak_r < FFT_MAG_FLOOR) s_spec_peak_r = FFT_MAG_FLOOR;

    // 3. Draw Left Channel Bars (Bass at center X=62..63 -> Treble at flank X=0..1)
    for (int b = 0; b < NUM_SPEC_BARS; b++) {
        int h = (int)(mag_l[b] / s_spec_peak_l * (float)BAR_MAX_H);
        if (h < 0) h = 0;
        if (h > BAR_MAX_H) h = BAR_MAX_H;

        // Peak hold decay
        if ((float)h >= s_peak_hold_l[b]) {
            s_peak_hold_l[b] = (float)h;
        } else {
            s_peak_hold_l[b] -= 0.8f;
            if (s_peak_hold_l[b] < 0.0f) s_peak_hold_l[b] = 0.0f;
        }

        int x = 62 - b * 2;
        // Bar
        if (h > 0) {
            SafeDraw::drawVLine(x, BAR_B - h, h);
            SafeDraw::drawVLine(x + 1, BAR_B - h, h);
        }

        // Peak Hold Dot
        int ph_y = BAR_B - (int)s_peak_hold_l[b] - 1;
        if (s_peak_hold_l[b] > 0.5f && ph_y >= 0 && ph_y < BAR_B - h) {
            SafeDraw::drawPixel(x, ph_y);
            SafeDraw::drawPixel(x + 1, ph_y);
        }
    }

    // 4. Draw Right Channel Bars (Bass at center X=64..65 -> Treble at flank X=126..127)
    for (int b = 0; b < NUM_SPEC_BARS; b++) {
        int h = (int)(mag_r[b] / s_spec_peak_r * (float)BAR_MAX_H);
        if (h < 0) h = 0;
        if (h > BAR_MAX_H) h = BAR_MAX_H;

        // Peak hold decay
        if ((float)h >= s_peak_hold_r[b]) {
            s_peak_hold_r[b] = (float)h;
        } else {
            s_peak_hold_r[b] -= 0.8f;
            if (s_peak_hold_r[b] < 0.0f) s_peak_hold_r[b] = 0.0f;
        }

        int x = 64 + b * 2;
        // Bar
        if (h > 0) {
            SafeDraw::drawVLine(x, BAR_B - h, h);
            SafeDraw::drawVLine(x + 1, BAR_B - h, h);
        }

        // Peak Hold Dot
        int ph_y = BAR_B - (int)s_peak_hold_r[b] - 1;
        if (s_peak_hold_r[b] > 0.5f && ph_y >= 0 && ph_y < BAR_B - h) {
            SafeDraw::drawPixel(x, ph_y);
            SafeDraw::drawPixel(x + 1, ph_y);
        }
    }
}
