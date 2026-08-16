#include "effects.h"

// -----------------------------------------------------------------------
// MODE 15 — HEART MATRIX (Digital Rain + Dynamic Expanding Hollow Heart + "MVT")
// -----------------------------------------------------------------------
#define MATRIX_COLS 16
static float s_rheart_y[MATRIX_COLS]   = {0};
static float s_rheart_spd[MATRIX_COLS] = {0};
static float s_rheart_peak             = FFT_MAG_FLOOR;

void effect_rain_heart_on_enter() {
    for (int col = 0; col < MATRIX_COLS; col++) {
        s_rheart_y[col] = (float)(-(rand() % 32));
    }
    s_rheart_peak = FFT_MAG_FLOOR;
}

void effect_rain_heart_on_exit() {
    memset(s_rheart_y, 0, sizeof(s_rheart_y));
    s_rheart_peak = FFT_MAG_FLOOR;
}

void effect_rain_heart_render(const int32_t *left, const int32_t *right, size_t n) {
    const int hx = 64;
    const int hy = 29;

    // 1. Time-Domain Amplitude Peak
    int32_t max_sample = 0;
    for (size_t i = 0; i < n; i++) {
        int32_t smp = abs(left[i]) > abs(right[i]) ? abs(left[i]) : abs(right[i]);
        if (smp > max_sample) max_sample = smp;
    }
    float amp_norm = (float)max_sample / (float)(s_peak_l > 100 ? s_peak_l : 100);
    if (amp_norm > 1.0f) amp_norm = 1.0f;

    // 2. FFT on Audio Mix
    for (size_t i = 0; i < n; i++) {
        s_fft_real[i] = (float)((left[i] + right[i]) / 2);
        s_fft_imag[i] = 0.0f;
    }
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();

    // Bass energy tracking with fast dynamic follower
    float bass_sum = 0.0f;
    for (int b = 1; b <= 5; b++) bass_sum += s_fft_real[b];
    s_rheart_peak = bass_sum > s_rheart_peak ? bass_sum : (s_rheart_peak * 0.94f);
    if (s_rheart_peak < FFT_MAG_FLOOR) s_rheart_peak = FFT_MAG_FLOOR;
    float bass_norm = bass_sum / s_rheart_peak;
    if (bass_norm > 1.0f) bass_norm = 1.0f;

    // Combined instantaneous intensity with aggressive curve for high sensitivity
    float intensity = 0.60f * amp_norm + 0.40f * bass_norm;
    if (intensity > 1.0f) intensity = 1.0f;

    // 3. Falling digital rain in background
    for (int col = 0; col < MATRIX_COLS; col++) {
        float mag = s_fft_real[1 + col * 3];
        float norm = mag / s_rheart_peak;
        if (norm > 1.0f) norm = 1.0f;

        s_rheart_spd[col] = 1.2f + powf(norm, 0.50f) * 5.5f;
        s_rheart_y[col] += s_rheart_spd[col];
        if (s_rheart_y[col] > 74.0f) {
            s_rheart_y[col] = (float)(-(rand() % 16));
        }

        int x = col * 8 + 3;
        int y = (int)s_rheart_y[col];

        SafeDraw::drawDisc(x, y, 1);
        for (int t = 1; t <= 5; t++) {
            int ty = y - t * 3;
            if (t % 2 == 0) {
                SafeDraw::drawPixel(x, ty);
            }
        }
        if (y >= 60 && norm > 0.25f) {
            SafeDraw::drawPixel(x - 1, 62);
            SafeDraw::drawPixel(x + 1, 62);
        }
    }

    // 4. Highly Responsive Dynamic Expanding Hollow Heart
    // Scales dynamically from 0.70 (compact) up to 1.45 (huge pulse)
    float h_scale = 0.70f + powf(intensity, 0.40f) * 0.75f;
    const int NUM_PTS = 36;
    int pts_x[NUM_PTS];
    int pts_y[NUM_PTS];

    // A. Fill Heart Interior with Black (Shield) to block matrix rain
    SafeDraw::setDrawColor(0);
    for (int i = 0; i < NUM_PTS; i++) {
        float t = i * (6.283185f / (float)NUM_PTS);
        float st = sinf(t);
        float ct = cosf(t);
        float x_raw = 16.0f * st * st * st;
        float y_raw = -(13.0f * ct - 5.0f * cosf(2.0f * t) - 2.0f * cosf(3.0f * t) - cosf(4.0f * t));
        pts_x[i] = hx + (int)(x_raw * h_scale + (x_raw >= 0 ? 0.5f : -0.5f));
        pts_y[i] = hy + (int)(y_raw * h_scale + (y_raw >= 0 ? 0.5f : -0.5f));
        SafeDraw::drawHLine(pts_x[i] < hx ? pts_x[i] : hx, pts_y[i], abs(pts_x[i] - hx) + 1);
    }

    // B. Draw Crisp White Hollow Heart Outline
    SafeDraw::setDrawColor(1);
    for (int i = 0; i < NUM_PTS; i++) {
        int next = (i + 1) % NUM_PTS;
        SafeDraw::drawLine(pts_x[i], pts_y[i], pts_x[next], pts_y[next]);
    }

    // 5. Centered "MVT" Text inside Heart
    SafeDraw::setFont(u8g2_font_6x10_tf);
    int tw = SafeDraw::getStrWidth("MVT");
    SafeDraw::drawStr(hx - tw / 2, hy + 4, "MVT");
}
