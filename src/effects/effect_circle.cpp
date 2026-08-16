#include "effects.h"

// -----------------------------------------------------------------------
// MODE 6 — CIRCLE MVT  (radial stereo visualizer with "MVT" center)
// -----------------------------------------------------------------------
#define CIRCLE_BANDS 16
static float s_circle_peak               = FFT_MAG_FLOOR;
static float s_circle_pk_l[CIRCLE_BANDS] = {0};
static float s_circle_pk_r[CIRCLE_BANDS] = {0};

void effect_circle_on_enter() {
    memset(s_circle_pk_l, 0, sizeof(s_circle_pk_l));
    memset(s_circle_pk_r, 0, sizeof(s_circle_pk_r));
    s_circle_peak = FFT_MAG_FLOOR;
}

void effect_circle_on_exit() {
    memset(s_circle_pk_l, 0, sizeof(s_circle_pk_l));
    memset(s_circle_pk_r, 0, sizeof(s_circle_pk_r));
    s_circle_peak = FFT_MAG_FLOOR;
}

void effect_circle_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 64;
    const int cy = 31;
    const int R_IN = 12;         // 1.5x size (radius 12 px, diameter 24 px)
    const int R_MAX_LEN = 17;    // Maximum ray length to fit screen boundaries

    float bands_l[CIRCLE_BANDS] = {0};
    float bands_r[CIRCLE_BANDS] = {0};

    // 1. FFT on Left Channel
    for (size_t i = 0; i < n; i++) {
        s_fft_real[i] = (float)left[i];
        s_fft_imag[i] = 0.0f;
    }
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();

    float max_mag = 0.0f;
    for (int k = 0; k < CIRCLE_BANDS; k++) {
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

    for (int k = 0; k < CIRCLE_BANDS; k++) {
        float sum = 0.0f;
        int start_bin = 1 + k * 3;
        for (int b = 0; b < 3 && (start_bin + b) < (int)(n / 2); b++) {
            sum += s_fft_real[start_bin + b];
        }
        bands_r[k] = sum / 3.0f;
        if (bands_r[k] > max_mag) max_mag = bands_r[k];
    }

    // Fast-adapting AGC for lively responsiveness
    s_circle_peak = max_mag > s_circle_peak ? max_mag : (s_circle_peak * 0.95f);
    if (s_circle_peak < FFT_MAG_FLOOR) s_circle_peak = FFT_MAG_FLOOR;

    // 3. Draw Left Radial Rays (Left semicircle: Bass at Top k=0 → Treble at Bottom k=15)
    for (int k = 0; k < CIRCLE_BANDS; k++) {
        float norm_l = bands_l[k] / s_circle_peak;
        if (norm_l > 1.0f) norm_l = 1.0f;
        float len = powf(norm_l, 0.60f) * (float)R_MAX_LEN;
        if (len < 0.0f) len = 0.0f;
        if (len > R_MAX_LEN) len = R_MAX_LEN;

        // Peak follower
        if (len >= s_circle_pk_l[k]) s_circle_pk_l[k] = len;
        else s_circle_pk_l[k] *= 0.92f;

        // Angle: sweeps counter-clockwise on left side from top (-90°) to bottom (-270°)
        float angle = -1.570796f - (3.141592f / (CIRCLE_BANDS + 1)) * (k + 1);
        float ca = cosf(angle);
        float sa = sinf(angle);

        if (len >= 1.0f) {
            int x1 = cx + (int)(ca * (R_IN + 1) + 0.5f);
            int y1 = cy + (int)(sa * (R_IN + 1) + 0.5f);
            int x2 = cx + (int)(ca * (R_IN + 1 + len) + 0.5f);
            int y2 = cy + (int)(sa * (R_IN + 1 + len) + 0.5f);
            SafeDraw::drawLine(x1, y1, x2, y2);
        }

        // Peak dot
        if (s_circle_pk_l[k] >= 1.0f) {
            int pk_r = R_IN + 2 + (int)(s_circle_pk_l[k] + 0.5f);
            if (pk_r <= 31) {
                SafeDraw::drawPixel(cx + (int)(ca * pk_r + 0.5f), cy + (int)(sa * pk_r + 0.5f));
            }
        }
    }

    // 4. Draw Right Radial Rays (Right semicircle: Bass at Top k=0 → Treble at Bottom k=15)
    for (int k = 0; k < CIRCLE_BANDS; k++) {
        float norm_r = bands_r[k] / s_circle_peak;
        if (norm_r > 1.0f) norm_r = 1.0f;
        float len = powf(norm_r, 0.60f) * (float)R_MAX_LEN;
        if (len < 0.0f) len = 0.0f;
        if (len > R_MAX_LEN) len = R_MAX_LEN;

        // Peak follower
        if (len >= s_circle_pk_r[k]) s_circle_pk_r[k] = len;
        else s_circle_pk_r[k] *= 0.92f;

        // Angle: sweeps clockwise on right side from top (-90°) to bottom (+90°)
        float angle = -1.570796f + (3.141592f / (CIRCLE_BANDS + 1)) * (k + 1);
        float ca = cosf(angle);
        float sa = sinf(angle);

        if (len >= 1.0f) {
            int x1 = cx + (int)(ca * (R_IN + 1) + 0.5f);
            int y1 = cy + (int)(sa * (R_IN + 1) + 0.5f);
            int x2 = cx + (int)(ca * (R_IN + 1 + len) + 0.5f);
            int y2 = cy + (int)(sa * (R_IN + 1 + len) + 0.5f);
            SafeDraw::drawLine(x1, y1, x2, y2);
        }

        // Peak dot
        if (s_circle_pk_r[k] >= 1.0f) {
            int pk_r = R_IN + 2 + (int)(s_circle_pk_r[k] + 0.5f);
            if (pk_r <= 31) {
                SafeDraw::drawPixel(cx + (int)(ca * pk_r + 0.5f), cy + (int)(sa * pk_r + 0.5f));
            }
        }
    }

    // 5. Center Circle & "MVT" text
    SafeDraw::drawCircle(cx, cy, R_IN);
    SafeDraw::setFont(u8g2_font_6x10_tf);
    int tw = SafeDraw::getStrWidth("MVT");
    SafeDraw::drawStr(cx - tw / 2, cy + 4, "MVT");

    // 6. Side channel labels
    SafeDraw::setFont(u8g2_font_04b_03_tr);
    SafeDraw::drawStr(2, 33, "L");
    SafeDraw::drawStr(122, 33, "R");
}
