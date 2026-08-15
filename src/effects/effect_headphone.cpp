#include "effects.h"

// -----------------------------------------------------------------------
// MODE 19 — MVT HEADPHONE (Studio Headset with Integrated Ear-Cup VU Bars)
// -----------------------------------------------------------------------
static float s_hp_peak_l = 0.0f;
static float s_hp_peak_r = 0.0f;

void effect_headphone_on_enter() {
    s_hp_peak_l = 0.0f;
    s_hp_peak_r = 0.0f;
}

void effect_headphone_on_exit() {}

void effect_headphone_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 64, cy = 40;

    // 1. FFT for left and right channel analysis
    for (size_t i = 0; i < n; i++) {
        s_fft_real[i] = (float)left[i];
        s_fft_imag[i] = 0.0f;
    }
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();

    float fft_l[8];
    for (int b = 0; b < 8; b++) {
        fft_l[b] = s_fft_real[b + 1] / (float)s_peak_l;
        if (fft_l[b] > 1.0f) fft_l[b] = 1.0f;
    }

    for (size_t i = 0; i < n; i++) {
        s_fft_real[i] = (float)right[i];
        s_fft_imag[i] = 0.0f;
    }
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();

    float fft_r[8];
    for (int b = 0; b < 8; b++) {
        fft_r[b] = s_fft_real[b + 1] / (float)s_peak_r;
        if (fft_r[b] > 1.0f) fft_r[b] = 1.0f;
    }

    float bass = (fft_l[0] + fft_l[1] + fft_r[0] + fft_r[1]) * 0.5f;
    if (bass > 1.0f) bass = 1.0f;

    // Volume level calculation for Left and Right channels
    int32_t cl = 0;
    for (size_t i = 0; i < n; i++) {
        int32_t v = left[i] < 0 ? -left[i] : left[i];
        if (v > cl) cl = v;
    }
    float vol_l = (float)cl / (float)s_peak_l;
    if (vol_l > 1.0f) vol_l = 1.0f;

    int32_t cr = 0;
    for (size_t i = 0; i < n; i++) {
        int32_t v = right[i] < 0 ? -right[i] : right[i];
        if (v > cr) cr = v;
    }
    float vol_r = (float)cr / (float)s_peak_r;
    if (vol_r > 1.0f) vol_r = 1.0f;

    // 2. Headphone Headband Arch (Fully visible within screen)
    SafeDraw::drawCircle(cx, cy, 33);
    SafeDraw::drawCircle(cx, cy, 35);
    // Erase bottom half of headband below earcups
    SafeDraw::setDrawColor(0);
    SafeDraw::drawBox(0, cy + 2, 128, 64 - (cy + 2));
    SafeDraw::setDrawColor(1);

    // Top Headband Cushion (y = 3..8)
    SafeDraw::drawRBox(cx - 15, 3, 30, 5, 2);

    // 3. Ear-Cups with Integrated Real-Time Audio Level Bars
    int bounce = (int)(bass * 2.0f);
    const int BAR_MAX_H = 20;
    const int BAR_BASE_Y = 51;

    // Left Ear-Cup Frame (x=18..28, y=28..53)
    SafeDraw::drawRFrame(18 - bounce, 28, 11, 26, 3);
    int bar_h_l = (int)(vol_l * (float)BAR_MAX_H);
    if (bar_h_l < 1) bar_h_l = 1;

    // Peak hold decay for Left Channel
    if ((float)bar_h_l >= s_hp_peak_l) {
        s_hp_peak_l = (float)bar_h_l;
    } else {
        s_hp_peak_l -= 0.6f;
        if (s_hp_peak_l < 0.0f) s_hp_peak_l = 0.0f;
    }

    // Draw Left VU Bar & Peak Dot
    SafeDraw::drawBox(21 - bounce, BAR_BASE_Y - bar_h_l, 5, bar_h_l);
    int ph_y_l = BAR_BASE_Y - (int)s_hp_peak_l - 1;
    if (ph_y_l >= 29 && ph_y_l < BAR_BASE_Y - bar_h_l) {
        SafeDraw::drawHLine(21 - bounce, ph_y_l, 5);
    }

    // Right Ear-Cup Frame (x=100..110, y=28..53)
    SafeDraw::drawRFrame(100 + bounce, 28, 11, 26, 3);
    int bar_h_r = (int)(vol_r * (float)BAR_MAX_H);
    if (bar_h_r < 1) bar_h_r = 1;

    // Peak hold decay for Right Channel
    if ((float)bar_h_r >= s_hp_peak_r) {
        s_hp_peak_r = (float)bar_h_r;
    } else {
        s_hp_peak_r -= 0.6f;
        if (s_hp_peak_r < 0.0f) s_hp_peak_r = 0.0f;
    }

    // Draw Right VU Bar & Peak Dot
    SafeDraw::drawBox(103 + bounce, BAR_BASE_Y - bar_h_r, 5, bar_h_r);
    int ph_y_r = BAR_BASE_Y - (int)s_hp_peak_r - 1;
    if (ph_y_r >= 29 && ph_y_r < BAR_BASE_Y - bar_h_r) {
        SafeDraw::drawHLine(103 + bounce, ph_y_r, 5);
    }

    // 4. Central Dynamic Dot Waveform (Sóng âm dạng dot giữa 2 tai nghe)
    int32_t pk = s_peak_l > s_peak_r ? s_peak_l : s_peak_r;
    if (pk < 1) pk = 1;

    for (int x = 30; x <= 98; x += 2) {
        int idx = (x - 30) * (int)(n - 1) / 68;
        int32_t sample = (left[idx] + right[idx]) / 2;
        int dy = (int)(((float)sample / (float)pk) * 12.0f);

        int wy = cy + dy;
        if (wy >= 20 && wy <= 60) {
            SafeDraw::drawPixel(x, wy);
        }
        // Additional mirror dot when dynamic amplitude is high
        if (abs(dy) >= 2) {
            int sym_y = cy - dy;
            if (sym_y >= 20 && sym_y <= 60) {
                SafeDraw::drawPixel(x, sym_y);
            }
        }
    }

    // 5. Text & Status
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(cx - 7, 63, "MVT");
    SafeDraw::drawStr(4, 63, "L");
    SafeDraw::drawStr(120, 63, "R");
}
