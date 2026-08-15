#include "effects.h"

// -----------------------------------------------------------------------
// MODE 24 — MVT BLACKHOLE (Gravitational Spiral & Relativistic Plasma Jets)
// -----------------------------------------------------------------------
struct StarParticle {
    float dist;
    float angle;
    float speed;
};

static StarParticle s_stars[24];

void effect_blackhole_on_enter() {
    for (int i = 0; i < 24; i++) {
        s_stars[i].dist = 14.0f + (float)(rand() % 50);
        s_stars[i].angle = ((float)rand() / (float)RAND_MAX) * 6.28318f;
        s_stars[i].speed = 0.04f + ((float)rand() / (float)RAND_MAX) * 0.04f;
    }
}

void effect_blackhole_on_exit() {}

void effect_blackhole_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 64, cy = 31;

    // 1. FFT for Bass detection (Triggers Relativistic Jet)
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

    // 2. Gravitational Infall & Swirling Accretion Disk
    for (int i = 0; i < 24; i++) {
        // Closer particles orbit faster (Keplerian physics)
        float current_speed = s_stars[i].speed * (30.0f / (s_stars[i].dist + 5.0f)) * (1.0f + bass * 1.5f);
        s_stars[i].angle += current_speed;
        s_stars[i].dist -= 0.35f + bass * 0.5f;

        // Respawn when swallowed by event horizon
        if (s_stars[i].dist < 8.0f) {
            s_stars[i].dist = 52.0f + (float)(rand() % 12);
            s_stars[i].angle = ((float)rand() / (float)RAND_MAX) * 6.28318f;
        }

        // Draw elliptical accretion disk projection
        int px = cx + (int)(cosf(s_stars[i].angle) * s_stars[i].dist);
        int py = cy + (int)(sinf(s_stars[i].angle) * (s_stars[i].dist * 0.42f));
        SafeDraw::drawPixel(px, py);
        if (s_stars[i].dist < 18.0f) {
            // Bright streak near horizon
            SafeDraw::drawPixel(px + 1, py);
        }
    }

    // 3. Relativistic Plasma Jets (Vertical Beams on Bass)
    if (bass > 0.25f) {
        int jet_w = (int)(bass * 6.0f);
        if (jet_w < 1) jet_w = 1;
        // Top Jet
        SafeDraw::drawBox(cx - jet_w / 2, 0, jet_w, cy - 8);
        // Bottom Jet
        SafeDraw::drawBox(cx - jet_w / 2, cy + 9, jet_w, 64 - (cy + 9));

        // Flare sparks along jet
        SafeDraw::drawPixel(cx - jet_w - 2, 8);
        SafeDraw::drawPixel(cx + jet_w + 2, 12);
        SafeDraw::drawPixel(cx - jet_w - 2, 52);
        SafeDraw::drawPixel(cx + jet_w + 2, 48);
    }

    // 4. Central Event Horizon (Photon Ring & Shadow)
    SafeDraw::drawCircle(cx, cy, 9);
    SafeDraw::drawCircle(cx, cy, 8);
    SafeDraw::setDrawColor(0);
    SafeDraw::drawDisc(cx, cy, 7); // Void black center
    SafeDraw::setDrawColor(1);

    // 5. HUD telemetry
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 8, "EVENT HORIZON");
    SafeDraw::drawStr(96, 62, "MVT-GRAV");
}
