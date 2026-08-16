#include "effects.h"

// -----------------------------------------------------------------------
// MODE 24 — MVT BLACKHOLE (Gravitational Spiral, Einstein Ring & Plasma Jets)
// -----------------------------------------------------------------------
struct StarParticle {
    float dist;
    float angle;
    float speed;
};

static StarParticle s_stars[36];
static float s_grav_wave = 0.0f;

void effect_blackhole_on_enter() {
    s_grav_wave = 0.0f;
    for (int i = 0; i < 36; i++) {
        s_stars[i].dist = 12.0f + (float)(rand() % 52);
        s_stars[i].angle = ((float)rand() / (float)RAND_MAX) * 6.28318f;
        s_stars[i].speed = 0.035f + ((float)rand() / (float)RAND_MAX) * 0.045f;
    }
}

void effect_blackhole_on_exit() {}

void effect_blackhole_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 64, cy = 31;

    // 1. FFT for Bass and Treble detection
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

    float treble = 0.0f;
    for (int b = 20; b <= 40; b++) treble += s_fft_real[b];
    treble /= (20.0f * (float)s_peak_l);
    if (treble > 1.0f) treble = 1.0f;

    // 2. Gravitational shockwave expansion
    s_grav_wave += 0.8f + bass * 2.2f;
    if (s_grav_wave > 48.0f) s_grav_wave = 8.0f;

    // Draw pulsating gravitational ripple rings
    if (bass > 0.35f) {
        int r1 = (int)s_grav_wave;
        if (r1 > 8 && r1 < 50) {
            SafeDraw::drawEllipse(cx, cy, r1, (int)(r1 * 0.40f));
        }
        int r2 = (int)(s_grav_wave * 0.6f);
        if (r2 > 8 && r2 < 50) {
            SafeDraw::drawEllipse(cx, cy, r2, (int)(r2 * 0.40f));
        }
    }

    // 3. Gravitational Infall & Swirling Accretion Disk (Keplerian physics)
    for (int i = 0; i < 36; i++) {
        float current_speed = s_stars[i].speed * (32.0f / (s_stars[i].dist + 4.0f)) * (1.0f + bass * 1.8f);
        s_stars[i].angle += current_speed;
        s_stars[i].dist -= 0.30f + bass * 0.6f;

        // Respawn when crossing event horizon
        if (s_stars[i].dist < 8.0f) {
            s_stars[i].dist = 48.0f + (float)(rand() % 16);
            s_stars[i].angle = ((float)rand() / (float)RAND_MAX) * 6.28318f;
        }

        // Elliptical 3D accretion disk projection
        float ex = cosf(s_stars[i].angle) * s_stars[i].dist;
        float ey = sinf(s_stars[i].angle) * (s_stars[i].dist * 0.40f);
        
        // Einstein gravitational lensing (particles behind black hole warp upwards/downwards)
        if (sinf(s_stars[i].angle) < 0 && s_stars[i].dist < 22.0f) {
            ey -= (22.0f - s_stars[i].dist) * 0.35f;
        }

        int px = cx + (int)ex;
        int py = cy + (int)ey;
        SafeDraw::drawPixel(px, py);

        // Particle trail streak near inner edge
        if (s_stars[i].dist < 20.0f) {
            float prev_a = s_stars[i].angle - current_speed * 1.5f;
            int tx = cx + (int)(cosf(prev_a) * (s_stars[i].dist + 0.6f));
            int ty = cy + (int)(sinf(prev_a) * ((s_stars[i].dist + 0.6f) * 0.40f));
            SafeDraw::drawLine(px, py, tx, ty);
        }
    }

    // 4. Relativistic Plasma Jets (Dual vertical beams)
    int jet_w = (int)(bass * 7.0f);
    if (jet_w > 0) {
        // Top Jet
        SafeDraw::drawLine(cx - jet_w / 2, cy - 8, cx, 0);
        SafeDraw::drawLine(cx + jet_w / 2, cy - 8, cx, 0);
        SafeDraw::drawLine(cx, cy - 8, cx, 0);
        // Bottom Jet
        SafeDraw::drawLine(cx - jet_w / 2, cy + 8, cx, 63);
        SafeDraw::drawLine(cx + jet_w / 2, cy + 8, cx, 63);
        SafeDraw::drawLine(cx, cy + 8, cx, 63);

        // High-energy particle flares along jet
        if (treble > 0.2f) {
            SafeDraw::drawPixel(cx - 2 - (rand() % 4), 6 + (rand() % 8));
            SafeDraw::drawPixel(cx + 2 + (rand() % 4), 14 + (rand() % 8));
            SafeDraw::drawPixel(cx - 2 - (rand() % 4), 50 + (rand() % 8));
            SafeDraw::drawPixel(cx + 2 + (rand() % 4), 42 + (rand() % 8));
        }
    }

    // 5. Einstein Ring & Central Event Horizon (Shadow & Photon Sphere)
    int horizon_r = 8 + (int)(bass * 2.0f);
    SafeDraw::drawCircle(cx, cy, horizon_r + 2); // Photon Sphere
    SafeDraw::drawCircle(cx, cy, horizon_r + 1);
    SafeDraw::setDrawColor(0);
    SafeDraw::drawDisc(cx, cy, horizon_r);       // Pure Black Void Event Horizon
    SafeDraw::setDrawColor(1);

    // 6. HUD Telemetry
    // SafeDraw::setFont(u8g2_font_4x6_tr);
    // SafeDraw::drawStr(2, 7, "EVENT HORIZON");
    // SafeDraw::drawStr(90, 62, "MVT-SINGULAR");
}
