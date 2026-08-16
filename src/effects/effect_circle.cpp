#include "effects.h"

// -----------------------------------------------------------------------
// MODE 6 — CIRCLE MVT  (radial stereo visualizer with "MVT" center)
// -----------------------------------------------------------------------
#define CIRCLE_BANDS 20
static float s_circle_peak               = FFT_MAG_FLOOR;
static float s_circle_pk_l[CIRCLE_BANDS] = {0};
static float s_circle_pk_r[CIRCLE_BANDS] = {0};
static float s_circle_vol                = 0.0f;

void effect_circle_on_enter() {
    memset(s_circle_pk_l, 0, sizeof(s_circle_pk_l));
    memset(s_circle_pk_r, 0, sizeof(s_circle_pk_r));
    s_circle_peak = FFT_MAG_FLOOR;
    s_circle_vol = 0.0f;
}

void effect_circle_on_exit() {
    memset(s_circle_pk_l, 0, sizeof(s_circle_pk_l));
    memset(s_circle_pk_r, 0, sizeof(s_circle_pk_r));
    s_circle_peak = FFT_MAG_FLOOR;
    s_circle_vol = 0.0f;
}

void effect_circle_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 64;
    const int cy = 35;           // Lowered down towards bottom for ideal vertical balance
    const int R_IN = 16;         // 1.5x bigger core radius (diameter 32px)
    const int R_MAX_LEN = 16;    // Dynamic ray span extending outward up to radius 32px

    static float bands_l[CIRCLE_BANDS]; // static: avoid per-frame stack allocation
    static float bands_r[CIRCLE_BANDS]; // static: avoid per-frame stack allocation
    memset(bands_l, 0, sizeof(bands_l));
    memset(bands_r, 0, sizeof(bands_r));

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
        int start_bin = 1 + k * 2;
        for (int b = 0; b < 2 && (start_bin + b) < (int)(n / 2); b++) {
            sum += s_fft_real[start_bin + b];
        }
        bands_l[k] = sum * 0.5f;
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
        int start_bin = 1 + k * 2;
        for (int b = 0; b < 2 && (start_bin + b) < (int)(n / 2); b++) {
            sum += s_fft_real[start_bin + b];
        }
        bands_r[k] = sum * 0.5f;
        if (bands_r[k] > max_mag) max_mag = bands_r[k];
    }

    // Dynamic AGC & Audio Energy Level
    s_circle_peak = max_mag > s_circle_peak ? max_mag : (s_circle_peak * 0.94f);
    if (s_circle_peak < FFT_MAG_FLOOR) s_circle_peak = FFT_MAG_FLOOR;

    float raw_vol = (bands_l[0] + bands_l[1] + bands_r[0] + bands_r[1]) / (4.0f * s_circle_peak);
    s_circle_vol = s_circle_vol * 0.70f + raw_vol * 0.30f;
    if (s_circle_vol > 1.0f) s_circle_vol = 1.0f;

    // 3. Draw Left Radial Rays (Left semicircle: Bass at Top k=0 → Treble at Bottom k=19)
    for (int k = 0; k < CIRCLE_BANDS; k++) {
        float norm_l = bands_l[k] / s_circle_peak;
        if (norm_l > 1.0f) norm_l = 1.0f;
        float len = powf(norm_l, 0.65f) * (float)R_MAX_LEN;
        if (len < 0.0f) len = 0.0f;
        if (len > R_MAX_LEN) len = R_MAX_LEN;

        // Smooth peak follower
        if (len >= s_circle_pk_l[k]) s_circle_pk_l[k] = len;
        else s_circle_pk_l[k] *= 0.90f;

        // Angle: sweeps counter-clockwise on left side from top (-90°) to bottom (-270°)
        float angle = -1.570796f - (3.141592f / (CIRCLE_BANDS + 1)) * (k + 1);
        float ca = cosf(angle);
        float sa = sinf(angle);

        int r_start = R_IN + 1;
        int r_end   = r_start + (int)len;

        if (len >= 1.0f) {
            int x1 = cx + (int)(ca * r_start + 0.5f);
            int y1 = cy + (int)(sa * r_start + 0.5f);
            int x2 = cx + (int)(ca * r_end + 0.5f);
            int y2 = cy + (int)(sa * r_end + 0.5f);
            SafeDraw::drawLine(x1, y1, x2, y2);

            // Double stroke on prominent bands
            if (norm_l > 0.55f && (k % 2 == 0)) {
                SafeDraw::drawLine(x1, y1 + 1, x2, y2 + 1);
            }
        }

        // Floating peak dot
        if (s_circle_pk_l[k] >= 1.5f) {
            int pk_r = R_IN + 2 + (int)(s_circle_pk_l[k] + 0.5f);
            int px = cx + (int)(ca * pk_r + 0.5f);
            int py = cy + (int)(sa * pk_r + 0.5f);
            if (py >= 0 && py <= 63) {
                SafeDraw::drawPixel(px, py);
            }
        }
    }

    // 4. Draw Right Radial Rays (Right semicircle: Bass at Top k=0 → Treble at Bottom k=19)
    for (int k = 0; k < CIRCLE_BANDS; k++) {
        float norm_r = bands_r[k] / s_circle_peak;
        if (norm_r > 1.0f) norm_r = 1.0f;
        float len = powf(norm_r, 0.65f) * (float)R_MAX_LEN;
        if (len < 0.0f) len = 0.0f;
        if (len > R_MAX_LEN) len = R_MAX_LEN;

        // Smooth peak follower
        if (len >= s_circle_pk_r[k]) s_circle_pk_r[k] = len;
        else s_circle_pk_r[k] *= 0.90f;

        // Angle: sweeps clockwise on right side from top (-90°) to bottom (+90°)
        float angle = -1.570796f + (3.141592f / (CIRCLE_BANDS + 1)) * (k + 1);
        float ca = cosf(angle);
        float sa = sinf(angle);

        int r_start = R_IN + 1;
        int r_end   = r_start + (int)len;

        if (len >= 1.0f) {
            int x1 = cx + (int)(ca * r_start + 0.5f);
            int y1 = cy + (int)(sa * r_start + 0.5f);
            int x2 = cx + (int)(ca * r_end + 0.5f);
            int y2 = cy + (int)(sa * r_end + 0.5f);
            SafeDraw::drawLine(x1, y1, x2, y2);

            // Double stroke on prominent bands
            if (norm_r > 0.55f && (k % 2 == 0)) {
                SafeDraw::drawLine(x1, y1 + 1, x2, y2 + 1);
            }
        }

        // Floating peak dot
        if (s_circle_pk_r[k] >= 1.5f) {
            int pk_r = R_IN + 2 + (int)(s_circle_pk_r[k] + 0.5f);
            int px = cx + (int)(ca * pk_r + 0.5f);
            int py = cy + (int)(sa * pk_r + 0.5f);
            if (py >= 0 && py <= 63) {
                SafeDraw::drawPixel(px, py);
            }
        }
    }

    // 5. Center Cyber Rings & "MVT" Typography
    SafeDraw::drawCircle(cx, cy, R_IN);
    // Inner concentric ring pulsing on bass
    int pulse_r = R_IN - 3 - (int)(s_circle_vol * 3.0f);
    if (pulse_r > 8) {
        SafeDraw::drawCircle(cx, cy, pulse_r);
    }

    // Center Cardinal Reticle Ticks (top, bottom, left, right)
    SafeDraw::drawPixel(cx, cy - R_IN + 1);
    SafeDraw::drawPixel(cx, cy + R_IN - 1);
    SafeDraw::drawPixel(cx - R_IN + 1, cy);
    SafeDraw::drawPixel(cx + R_IN - 1, cy);

    // Bold "MVT" Typography
    SafeDraw::setFont(u8g2_font_7x14B_tr);
    int tw = SafeDraw::getStrWidth("MVT");
    SafeDraw::drawStr(cx - tw / 2, cy + 5, "MVT");

    // 6. Side channel indicators
    SafeDraw::setFont(u8g2_font_04b_03_tr);
    SafeDraw::drawStr(2, 8, "L");
    SafeDraw::drawStr(122, 8, "R");
}
