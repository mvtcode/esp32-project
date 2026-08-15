#include "effects.h"

// -----------------------------------------------------------------------
// MODE 11 — MVT TUNNEL (3D Cyber Warp Drive with Bass Shockwaves)
// -----------------------------------------------------------------------
static float s_tunnel_z[5] = {0.2f, 0.4f, 0.6f, 0.8f, 1.0f};

void effect_tunnel_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 64;
    const int cy = 31;

    // Calculate Bass Energy from first few FFT bins
    float bass_sum = 0.0f;
    for (size_t i = 0; i < n; i++) {
        s_fft_real[i] = (float)((left[i] + right[i]) / 2);
        s_fft_imag[i] = 0.0f;
    }
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();
    for (int b = 1; b <= 4; b++) bass_sum += s_fft_real[b];
    float bass_norm = bass_sum / (4.0f * (float)s_peak_l);
    if (bass_norm > 1.0f) bass_norm = 1.0f;

    // Speed up tunnel on bass hits
    float speed = 0.00625f + bass_norm * 0.015f;

    // 1. Perspective Wireframe Rings
    for (int i = 0; i < 5; i++) {
        s_tunnel_z[i] += speed;
        if (s_tunnel_z[i] > 1.0f) s_tunnel_z[i] -= 0.9f;

        float z = s_tunnel_z[i];
        int w = (int)(z * 122.0f + bass_norm * 8.0f * z);
        int h = (int)(z * 58.0f + bass_norm * 4.0f * z);
        if (w < 4) w = 4;
        if (h < 4) h = 4;

        int x = cx - w / 2;
        int y = cy - h / 2;
        SafeDraw::drawFrame(x, y, w, h);
    }

    // 2. Diagonal Perspective Edge Lines
    SafeDraw::drawLine(0, 0, cx - 10, cy - 6);
    SafeDraw::drawLine(127, 0, cx + 10, cy - 6);
    SafeDraw::drawLine(0, 63, cx - 10, cy + 6);
    SafeDraw::drawLine(127, 63, cx + 10, cy + 6);

    // 3. Central Vanishing Point with "MVT" Logo
    SafeDraw::setDrawColor(0);
    SafeDraw::drawBox(cx - 15, cy - 8, 30, 16);
    SafeDraw::setDrawColor(1);
    SafeDraw::drawFrame(cx - 15, cy - 8, 30, 16);

    SafeDraw::setFont(u8g2_font_6x10_tf);
    int tw = SafeDraw::getStrWidth("MVT");
    SafeDraw::drawStr(cx - tw / 2, cy + 4, "MVT");
}
