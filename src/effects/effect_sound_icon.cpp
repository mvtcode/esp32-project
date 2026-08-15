#include "effects.h"

// -----------------------------------------------------------------------
// MODE 26 — MVT SOUND ICON (Iconic Megaphone Speaker & Blast Shockwaves)
// -----------------------------------------------------------------------
struct SoundParticle {
    float x, y;
    float vx, vy;
    int life;
};

static const int NUM_SOUND_PARTICLES = 16;
static SoundParticle s_particles[NUM_SOUND_PARTICLES];
static float s_wave_phase = 0.0f;
static float s_vol_smooth = 0.0f;

void effect_sound_icon_on_enter() {
    s_wave_phase = 0.0f;
    s_vol_smooth = 0.0f;
    for (int i = 0; i < NUM_SOUND_PARTICLES; i++) {
        s_particles[i].x = 0.0f;
        s_particles[i].y = 0.0f;
        s_particles[i].vx = 0.0f;
        s_particles[i].vy = 0.0f;
        s_particles[i].life = 0;
    }
}

void effect_sound_icon_on_exit() {}

void effect_sound_icon_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cy = 32;

    // 1. Audio Frequency & Bass Analysis
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
    float cur_vol = (float)cl / (float)pk;
    if (cur_vol > 1.0f) cur_vol = 1.0f;
    s_vol_smooth = s_vol_smooth * 0.7f + cur_vol * 0.3f;

    // Advance wave phase
    s_wave_phase += 0.07f + bass * 0.14f;
    if (s_wave_phase >= 1.0f) s_wave_phase -= 1.0f;

    // 2. Iconic Megaphone Speaker Body (Left Side)
    int kick = (int)(bass * 4.0f);
    int bx = 6 - kick;
    int by = 22;
    int bw = 11;
    int bh = 20;

    // Speaker Base Cabinet Box (x = bx..bx+bw, y = 22..42)
    SafeDraw::drawRBox(bx, by, bw, bh, 2);
    // Engraved acoustic grill lines inside base box
    SafeDraw::setDrawColor(0);
    SafeDraw::drawVLine(bx + 3, by + 3, bh - 6);
    SafeDraw::drawVLine(bx + 6, by + 3, bh - 6);
    SafeDraw::setDrawColor(1);

    // Flared Megaphone Cone (Polygon from base to mouth)
    // Points: P1(bx+bw, by), P2(bx+bw+15, 10), P3(bx+bw+15, 54), P4(bx+bw, by+bh)
    int hx = bx + bw;
    int mouth_x = hx + 15;
    SafeDraw::drawTriangle(hx, by, mouth_x, 10, mouth_x, 54);
    SafeDraw::drawTriangle(hx, by, mouth_x, 54, hx, by + bh);

    // Cutout / Highlight inside the megaphone horn for 3D depth
    SafeDraw::setDrawColor(0);
    SafeDraw::drawTriangle(hx + 3, by + 2, mouth_x - 2, 14, mouth_x - 2, 50);
    SafeDraw::drawTriangle(hx + 3, by + 2, mouth_x - 2, 50, hx + 3, by + bh - 2);
    SafeDraw::setDrawColor(1);

    // Speaker Cone Diaphragm & Rim (Pulsing driver at mouth)
    int driver_r = (int)(bass * 5.0f);
    SafeDraw::drawVLine(mouth_x, 10, 45);
    SafeDraw::drawVLine(mouth_x + 1, 12, 41);
    // Pulsing inner sound driver
    SafeDraw::drawDisc(mouth_x - 2, cy, 3 + driver_r / 2);

    // 3. Dynamic Expanding Sound Wave Arcs ( ( ( ( )
    int origin_x = mouth_x - 4;
    int num_arcs = 4;
    for (int a = 0; a < num_arcs; a++) {
        float r = ((float)a + s_wave_phase) * 20.0f + 8.0f;
        if (r < 10.0f || r > 92.0f) continue;

        // Skip distant arcs if volume is too quiet
        if (r > 50.0f && s_vol_smooth < 0.2f && bass < 0.25f) continue;

        // Draw curved arc from -52 deg to +52 deg
        for (int deg = -52; deg <= 52; deg += 3) {
            float rad = (float)deg * 0.0174532925f;
            // Slight audio ripple modulation on the arc curve
            int idx = (deg + 52) * (int)(n - 1) / 104;
            int32_t samp = left[idx] / 4 + right[idx] / 4;
            float ripple = ((float)samp / (float)pk) * (bass * 3.5f);

            float cur_r = r + ripple;
            int px = origin_x + (int)(cur_r * cosf(rad));
            int py = cy + (int)(cur_r * sinf(rad));

            if (px >= mouth_x + 2) {
                SafeDraw::drawPixel(px, py);
                // Double arc line for strong sound wave presence
                if (r < 65.0f) {
                    SafeDraw::drawPixel(px + 1, py);
                }
            }
        }
    }

    // 4. Acoustic Sound Particles (Shooting outward to the right)
    for (int i = 0; i < NUM_SOUND_PARTICLES; i++) {
        if (s_particles[i].life <= 0) {
            if (bass > 0.25f || cur_vol > 0.2f) {
                // Spawn new particle at speaker mouth
                s_particles[i].x = (float)(mouth_x + 3);
                s_particles[i].y = (float)(cy + (rand() % 28 - 14));
                float angle = ((float)(rand() % 80) - 40.0f) * 0.0174532925f;
                float speed = 1.2f + ((float)(rand() % 100) / 100.0f) * (2.0f + bass * 3.0f);
                s_particles[i].vx = cosf(angle) * speed;
                s_particles[i].vy = sinf(angle) * speed * 0.6f;
                s_particles[i].life = 15 + rand() % 20;
            }
        } else {
            s_particles[i].x += s_particles[i].vx;
            s_particles[i].y += s_particles[i].vy;
            s_particles[i].life--;

            int px = (int)s_particles[i].x;
            int py = (int)s_particles[i].y;
            if (px >= mouth_x + 2) {
                SafeDraw::drawPixel(px, py);
            }
        }
    }

    // 5. Sound HUD (Volume Level Bar & Brand)
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 8, "MVT");
    SafeDraw::drawStr(2, 62, "SOUND");

    // Mini dynamic volume bar meter (top right)
    SafeDraw::drawFrame(74, 3, 52, 5);
    int bar_len = (int)(s_vol_smooth * 48.0f);
    if (bar_len > 48) bar_len = 48;
    if (bar_len > 0) {
        SafeDraw::drawBox(76, 4, bar_len, 3);
    }
}
