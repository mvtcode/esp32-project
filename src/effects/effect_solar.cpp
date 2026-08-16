#include "effects.h"

// -----------------------------------------------------------------------
// MODE 52 — MVT SOLAR SYSTEM (3D Keplerian Orbits, Solar Flares & Wind)
// -----------------------------------------------------------------------
struct Planet {
    float a;      // Semi-major axis (radius X)
    float b;      // Semi-minor axis (radius Y)
    float angle;  // Current orbital angle
    float speed;  // Keplerian base orbital speed
    int size;     // Planet pixel radius
    bool has_ring;
    bool has_moon;
};

static Planet s_planets[5];
static float s_solar_wind_r[16];
static float s_solar_wind_ang[16];

void effect_solar_on_enter() {
    // 0: Mercury
    s_planets[0] = { 13.0f, 5.0f, 0.5f, 0.070f, 1, false, false };
    // 1: Venus
    s_planets[1] = { 21.0f, 8.5f, 2.1f, 0.045f, 2, false, false };
    // 2: Earth (+ Moon)
    s_planets[2] = { 30.0f, 12.0f, 4.2f, 0.032f, 2, false, true };
    // 3: Mars
    s_planets[3] = { 40.0f, 16.0f, 1.0f, 0.024f, 1, false, false };
    // 4: Jupiter (+ Ring)
    s_planets[4] = { 52.0f, 21.0f, 3.4f, 0.015f, 3, true, false };

    for (int i = 0; i < 16; i++) {
        s_solar_wind_r[i] = 7.0f + (float)(rand() % 45);
        s_solar_wind_ang[i] = ((float)rand() / (float)RAND_MAX) * 6.28318f;
    }
}

void effect_solar_on_exit() {}

void effect_solar_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 64, cy = 31;

    // 1. Audio Processing: RMS & Stereo
    int64_t sum_l = 0, sum_r = 0;
    for (size_t i = 0; i < n; i++) {
        sum_l += (left[i] < 0 ? -left[i] : left[i]);
        sum_r += (right[i] < 0 ? -right[i] : right[i]);
    }
    float norm_l = (float)sum_l / ((float)n * (float)s_peak_l);
    float norm_r = (float)sum_r / ((float)n * (float)s_peak_r);
    if (norm_l > 1.0f) norm_l = 1.0f;
    if (norm_r > 1.0f) norm_r = 1.0f;
    float avg_vol = (norm_l + norm_r) * 0.5f;

    // FFT for Bass Beat
    for (size_t i = 0; i < n; i++) {
        s_fft_real[i] = (float)((left[i] + right[i]) / 2);
        s_fft_imag[i] = 0.0f;
    }
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();

    float bass = 0.0f;
    for (int b = 1; b <= 4; b++) bass += s_fft_real[b];
    bass /= (4.0f * (float)s_peak_l);
    if (bass > 1.0f) bass = 1.0f;

    // 2. Dynamic 3D tilt based on Stereo Balance
    float tilt = 0.40f + (norm_r - norm_l) * 0.12f;

    // 3. Draw Dotted Orbital Paths (Distant Background)
    for (int p = 0; p < 5; p++) {
        float a = s_planets[p].a;
        float b = a * tilt;
        for (int step = 0; step < 24; step += 2) {
            float ang = (float)step * (6.28318f / 24.0f);
            int ox = cx + (int)(cosf(ang) * a);
            int oy = cy + (int)(sinf(ang) * b);
            SafeDraw::drawPixel(ox, oy);
        }
    }

    // 4. Update & Draw Solar Wind Particles
    for (int i = 0; i < 16; i++) {
        s_solar_wind_r[i] += 0.6f + avg_vol * 1.8f;
        if (s_solar_wind_r[i] > 58.0f) {
            s_solar_wind_r[i] = 6.0f + (float)(rand() % 4);
            s_solar_wind_ang[i] = ((float)rand() / (float)RAND_MAX) * 6.28318f;
        }
        int sx = cx + (int)(cosf(s_solar_wind_ang[i]) * s_solar_wind_r[i]);
        int sy = cy + (int)(sinf(s_solar_wind_ang[i]) * (s_solar_wind_r[i] * tilt));
        SafeDraw::drawPixel(sx, sy);
    }

    // 5. Draw Central Sun (Pulsing Corona & Solar Flares)
    int sun_r = 5 + (int)(bass * 3.5f);
    SafeDraw::drawDisc(cx, cy, sun_r);

    // Solar Flares (Ray bursts)
    for (int flare = 0; flare < 8; flare++) {
        float fang = (float)flare * (6.28318f / 8.0f) + (bass * 0.5f);
        int flen = sun_r + 2 + (int)(bass * (4.0f + (flare % 3) * 2.0f));
        int fx = cx + (int)(cosf(fang) * flen);
        int fy = cy + (int)(sinf(fang) * flen);
        SafeDraw::drawLine(cx, cy, fx, fy);
    }

    // 6. Update and Draw Planets (Keplerian motion)
    for (int p = 0; p < 5; p++) {
        // Orbit speed increases with music volume
        s_planets[p].angle += s_planets[p].speed * (1.0f + avg_vol * 1.2f);
        if (s_planets[p].angle > 6.28318f) s_planets[p].angle -= 6.28318f;

        float a = s_planets[p].a;
        float b = a * tilt;
        int px = cx + (int)(cosf(s_planets[p].angle) * a);
        int py = cy + (int)(sinf(s_planets[p].angle) * b);

        // Planet body
        if (s_planets[p].size == 1) {
            SafeDraw::drawPixel(px, py);
        } else {
            SafeDraw::drawDisc(px, py, s_planets[p].size);
        }

        // Jupiter Ring
        if (s_planets[p].has_ring) {
            SafeDraw::drawHLine(px - 5, py, 11);
        }

        // Earth's Moon
        if (s_planets[p].has_moon) {
            float moon_ang = s_planets[p].angle * 4.0f;
            int mx = px + (int)(cosf(moon_ang) * 4.5f);
            int my = py + (int)(sinf(moon_ang) * 2.5f);
            SafeDraw::drawPixel(mx, my);
        }
    }

    // 7. HUD Telemetry
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 7, "SOLAR ORBIT");
    SafeDraw::drawStr(96, 62, "MVT-HELIOS");
}
