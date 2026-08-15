#include "effects.h"

// -----------------------------------------------------------------------
// MODE 12 — MVT ORBIT (3D Atomic Orbit Visualizer)
// -----------------------------------------------------------------------
static float s_orbit_ang = 0.0f;

void effect_orbit_on_enter() {
    s_orbit_ang = 0.0f;
}

void effect_orbit_on_exit() {}

void effect_orbit_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 64;
    const int cy = 31;

    // 1. Audio and Bass Energy Analysis
    for (size_t i = 0; i < n; i++) {
        s_fft_real[i] = (float)((left[i] + right[i]) / 2);
        s_fft_imag[i] = 0.0f;
    }
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();

    float bass_mag = 0.0f;
    for (int b = 1; b <= 4; b++) {
        bass_mag += s_fft_real[b];
    }
    float bass = bass_mag / (4.0f * (float)s_peak_l);
    if (bass > 1.0f) bass = 1.0f;

    int32_t pk = s_peak_l > s_peak_r ? s_peak_l : s_peak_r;
    if (pk < 1) pk = 1;

    int32_t cl = 0;
    for (size_t i = 0; i < n; i++) {
        int32_t v = left[i] < 0 ? -left[i] : left[i];
        if (v > cl) cl = v;
    }
    float vol = (float)cl / (float)pk;
    if (vol > 1.0f) vol = 1.0f;

    // Combined sound intensity: base speed when quiet, accelerates sharply on high volume/bass
    float intensity = bass * 0.6f + vol * 0.4f;
    if (intensity > 1.0f) intensity = 1.0f;

    // Rotation speed: 0.012 rad (idle) up to 0.068 rad (high sound intensity)
    s_orbit_ang += 0.012f + (intensity * intensity * 0.045f) + (intensity * 0.020f);
    if (s_orbit_ang > 6.283185f) s_orbit_ang -= 6.283185f;

    // 2. Three 3D Tilted Orbital Ellipses (Clean dotted geometry)
    // Orbit 1: Horizontal tilt
    for (int deg = 0; deg < 360; deg += 6) {
        float rad = (float)deg * 0.0174533f;
        int x = cx + (int)(cosf(rad) * 44.0f + 0.5f);
        int y = cy + (int)(sinf(rad) * 14.0f + 0.5f);
        SafeDraw::drawPixel(x, y);
    }
    // Orbit 2: Inclined +35°
    for (int deg = 0; deg < 360; deg += 6) {
        float rad = (float)deg * 0.0174533f;
        float rx = cosf(rad) * 42.0f;
        float ry = sinf(rad) * 13.0f;
        int x = cx + (int)(rx * 0.819f - ry * 0.573f + 0.5f);
        int y = cy + (int)(rx * 0.573f + ry * 0.819f + 0.5f);
        SafeDraw::drawPixel(x, y);
    }
    // Orbit 3: Inclined -35°
    for (int deg = 0; deg < 360; deg += 6) {
        float rad = (float)deg * 0.0174533f;
        float rx = cosf(rad) * 42.0f;
        float ry = sinf(rad) * 13.0f;
        int x = cx + (int)(rx * 0.819f + ry * 0.573f + 0.5f);
        int y = cy + (int)(-rx * 0.573f + ry * 0.819f + 0.5f);
        SafeDraw::drawPixel(x, y);
    }

    // 3. Central Nucleus Pulsing with Bass
    int r_nuc = 10 + (int)(bass * 4.0f);
    SafeDraw::setDrawColor(0);
    SafeDraw::drawDisc(cx, cy, r_nuc + 2);
    SafeDraw::setDrawColor(1);
    SafeDraw::drawCircle(cx, cy, r_nuc);
    SafeDraw::drawCircle(cx, cy, r_nuc - 2);

    SafeDraw::setFont(u8g2_font_04b_03_tr);
    SafeDraw::drawStr(cx - 7, cy + 3, "MVT");

    // 4. Orbiting Electrons with 3D Z-Depth & Comet Trails
    for (int o = 0; o < 3; o++) {
        float base_a = s_orbit_ang + (float)o * (6.283185f / 3.0f);

        // Draw comet tail dots behind each electron (stretches with intensity)
        int num_tails = 3 + (int)(intensity * 3.0f);
        for (int t = num_tails; t >= 1; t--) {
            float ta = base_a - (float)t * (0.09f + intensity * 0.05f);
            float rx = cosf(ta) * 43.0f;
            float ry = sinf(ta) * 14.0f;
            int tx, ty;
            if (o == 0) {
                tx = cx + (int)(rx + 0.5f);
                ty = cy + (int)(ry + 0.5f);
            } else if (o == 1) {
                tx = cx + (int)(rx * 0.819f - ry * 0.573f + 0.5f);
                ty = cy + (int)(rx * 0.573f + ry * 0.819f + 0.5f);
            } else {
                tx = cx + (int)(rx * 0.819f + ry * 0.573f + 0.5f);
                ty = cy + (int)(-rx * 0.573f + ry * 0.819f + 0.5f);
            }
            SafeDraw::drawPixel(tx, ty);
        }

        // Main Electron Head with 3D Depth Scaling
        float rx = cosf(base_a) * 43.0f;
        float ry = sinf(base_a) * 14.0f;
        int px, py;
        float z_val;
        if (o == 0) {
            px = cx + (int)(rx + 0.5f);
            py = cy + (int)(ry + 0.5f);
            z_val = ry;
        } else if (o == 1) {
            px = cx + (int)(rx * 0.819f - ry * 0.573f + 0.5f);
            py = cy + (int)(rx * 0.573f + ry * 0.819f + 0.5f);
            z_val = ry * 0.819f;
        } else {
            px = cx + (int)(rx * 0.819f + ry * 0.573f + 0.5f);
            py = cy + (int)(-rx * 0.573f + ry * 0.819f + 0.5f);
            z_val = ry * 0.819f;
        }

        if (z_val >= 0.0f) {
            // Front side: larger electron disc with halo
            SafeDraw::drawDisc(px, py, 2);
            if (z_val > 6.0f) {
                SafeDraw::drawCircle(px, py, 3);
            }
        } else {
            // Back side: smaller electron dot
            SafeDraw::drawDisc(px, py, 1);
        }
    }
}
