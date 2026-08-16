#include "effects.h"

// -----------------------------------------------------------------------
// MODE 26 — MVT SOUND ICON (Iconic Megaphone Speaker & Blast Shockwaves)
// Dynamic audio intensity reaction, mechanical recoil & expanding shockwaves
// -----------------------------------------------------------------------

struct SoundParticle {
    float x, y;
    float vx, vy;
    int life;
    int max_life;
};

static const int NUM_SOUND_PARTICLES = 24;
static SoundParticle s_particles[NUM_SOUND_PARTICLES];
static float s_wave_phase = 0.0f;
static float s_vol_smooth = 0.0f;
static float s_bass_smooth = 0.0f;

// Shockwave blast rings on sudden transients
struct ShockwaveRing {
    float r;
    float speed;
    int life;
};
static const int NUM_SHOCKWAVES = 3;
static ShockwaveRing s_shockwaves[NUM_SHOCKWAVES];

void effect_sound_icon_on_enter() {
    s_wave_phase = 0.0f;
    s_vol_smooth = 0.0f;
    s_bass_smooth = 0.0f;
    for (int i = 0; i < NUM_SOUND_PARTICLES; i++) {
        s_particles[i].life = 0;
    }
    for (int i = 0; i < NUM_SHOCKWAVES; i++) {
        s_shockwaves[i].life = 0;
        s_shockwaves[i].r = 0.0f;
        s_shockwaves[i].speed = 0.0f;
    }
}

void effect_sound_icon_on_exit() {}

void effect_sound_icon_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cy = 32;

    // 1. Audio Frequency, RMS Volume & Bass Analysis
    double sum_sq = 0.0;
    for (size_t i = 0; i < n; i++) {
        int32_t mixed = (left[i] + right[i]) / 2;
        double norm_sample = (double)mixed / (double)AUDIO_NOMINAL_PEAK;
        sum_sq += norm_sample * norm_sample;
        s_fft_real[i] = (float)mixed;
        s_fft_imag[i] = 0.0f;
    }

    // RMS Volume calculation
    float cur_rms = (float)sqrt(sum_sq / (double)n) * 1.5f;
    if (cur_rms > 1.0f) cur_rms = 1.0f;

    // Fast attack, smooth decay for volume
    if (cur_rms > s_vol_smooth) {
        s_vol_smooth = s_vol_smooth * 0.4f + cur_rms * 0.6f;
    } else {
        s_vol_smooth = s_vol_smooth * 0.82f + cur_rms * 0.18f;
    }

    // FFT analysis for Bass and Treble
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();

    float bass_mag = 0.0f;
    for (int b = 1; b <= 4; b++) {
        bass_mag += s_fft_real[b];
    }
    float cur_bass = bass_mag / (4.0f * FFT_MAG_FLOOR);
    if (cur_bass > 1.0f) cur_bass = 1.0f;

    if (cur_bass > s_bass_smooth) {
        s_bass_smooth = s_bass_smooth * 0.35f + cur_bass * 0.65f;
    } else {
        s_bass_smooth = s_bass_smooth * 0.80f + cur_bass * 0.20f;
    }

    // Advance wave phase (faster when louder / bass-heavy)
    s_wave_phase += 0.06f + s_vol_smooth * 0.10f + s_bass_smooth * 0.12f;
    if (s_wave_phase >= 1.0f) s_wave_phase -= 1.0f;

    // Trigger explosive shockwave rings on strong beats
    if (s_bass_smooth > 0.55f || (cur_rms > 0.60f && cur_rms > s_vol_smooth * 1.2f)) {
        for (int i = 0; i < NUM_SHOCKWAVES; i++) {
            if (s_shockwaves[i].life <= 0) {
                s_shockwaves[i].r = 6.0f;
                s_shockwaves[i].speed = 2.2f + s_bass_smooth * 2.8f;
                s_shockwaves[i].life = 22;
                break;
            }
        }
    }

    // 2. Iconic Megaphone Speaker Body with Elastic Recoil
    // Horizontal recoil kick and vertical vibration
    int kick = (int)(s_bass_smooth * 6.0f);
    int bx = 7 - kick; // Speaker cabinet shifts left on punch
    if (bx < 1) bx = 1;
    
    // Dynamic cabinet dimensions
    int bw = 10;
    int bh = 20 + (int)(s_bass_smooth * 2.0f);
    int by = cy - bh / 2;

    // Draw vibration recoil lines behind speaker when kicking hard
    if (s_bass_smooth > 0.35f) {
        int trail_x = bx - 3;
        if (trail_x >= 0) {
            SafeDraw::drawVLine(trail_x, by + 4, bh - 8);
        }
        if (s_bass_smooth > 0.6f && trail_x - 2 >= 0) {
            SafeDraw::drawVLine(trail_x - 2, by + 7, bh - 14);
        }
    }

    // Speaker Base Cabinet Box (solid rounded box with inner acoustic grill)
    SafeDraw::drawRBox(bx, by, bw, bh, 2);
    SafeDraw::setDrawColor(0);
    SafeDraw::drawVLine(bx + 3, by + 3, bh - 6);
    SafeDraw::drawVLine(bx + 6, by + 3, bh - 6);
    SafeDraw::setDrawColor(1);

    // Flared Megaphone Cone with dynamic mouth expansion
    int hx = bx + bw;
    int mouth_x = hx + 14;
    int mouth_half_h = 20 + (int)(s_vol_smooth * 6.0f + s_bass_smooth * 4.0f); // Horn opens wider when loud
    int mouth_top = cy - mouth_half_h;
    int mouth_bot = cy + mouth_half_h;

    // Draw outer megaphone horn triangles
    SafeDraw::drawTriangle(hx, by, mouth_x, mouth_top, mouth_x, mouth_bot);
    SafeDraw::drawTriangle(hx, by, mouth_x, mouth_bot, hx, by + bh);

    // Inner horn acoustic depth cutout (3D chamber effect)
    SafeDraw::setDrawColor(0);
    SafeDraw::drawTriangle(hx + 3, by + 2, mouth_x - 2, mouth_top + 4, mouth_x - 2, mouth_bot - 4);
    SafeDraw::drawTriangle(hx + 3, by + 2, mouth_x - 2, mouth_bot - 4, hx + 3, by + bh - 2);
    SafeDraw::setDrawColor(1);

    // Dynamic speaker driver diaphragm at cone mouth
    int driver_r = 3 + (int)(s_bass_smooth * 6.0f);
    SafeDraw::drawVLine(mouth_x, mouth_top, mouth_half_h * 2 + 1);
    SafeDraw::drawVLine(mouth_x + 1, mouth_top + 2, (mouth_half_h - 2) * 2 + 1);
    
    // Pulsing inner sound driver core
    SafeDraw::drawDisc(mouth_x - 2, cy, driver_r);
    SafeDraw::drawCircle(mouth_x - 2, cy, driver_r + 2);

    // 3. Acoustic Sound Blast Rays (Radial beams when intensity is high)
    if (s_vol_smooth > 0.40f) {
        int num_rays = 3 + (int)(s_vol_smooth * 4.0f);
        float ray_len = 16.0f + s_vol_smooth * 35.0f;
        for (int r = 0; r < num_rays; r++) {
            float ray_angle = -0.75f + ((float)r / (float)(num_rays - 1)) * 1.5f;
            int rx1 = mouth_x + 3;
            int ry1 = cy + (int)(sinf(ray_angle) * 8.0f);
            int rx2 = mouth_x + (int)(cosf(ray_angle) * ray_len);
            int ry2 = cy + (int)(sinf(ray_angle) * ray_len * 1.2f);
            // Dotted or faint ray lines
            SafeDraw::drawLine(rx1, ry1, rx2, ry2);
        }
    }

    // 4. Expanding Shockwave Blast Rings
    int origin_x = mouth_x - 2;
    for (int i = 0; i < NUM_SHOCKWAVES; i++) {
        if (s_shockwaves[i].life > 0) {
            s_shockwaves[i].r += s_shockwaves[i].speed;
            s_shockwaves[i].life--;
            int cur_r = (int)s_shockwaves[i].r;
            if (cur_r < 95) {
                // Draw arc for shockwave
                for (int deg = -56; deg <= 56; deg += 4) {
                    float rad = (float)deg * 0.0174532925f;
                    int px = origin_x + (int)(cur_r * cosf(rad));
                    int py = cy + (int)(cur_r * sinf(rad));
                    if (px >= mouth_x + 2 && px < 128 && py >= 0 && py < 64) {
                        SafeDraw::drawPixel(px, py);
                        if (s_shockwaves[i].life > 10) {
                            SafeDraw::drawPixel(px + 1, py);
                        }
                    }
                }
            }
        }
    }

    // 5. Dynamic Sound Wave Arcs with Real Audio Waveform Ripples
    // Number of active arcs scales directly with audio intensity (1 to 5 arcs)
    int max_arcs = 1 + (int)(s_vol_smooth * 4.2f + s_bass_smooth * 1.2f);
    if (max_arcs > 5) max_arcs = 5;

    for (int a = 0; a < 5; a++) {
        float r = ((float)a + s_wave_phase) * 18.0f + 7.0f;
        if (r < 8.0f || r > 96.0f) continue;

        // Progressive distance fade based on volume
        if (a >= max_arcs && r > 32.0f) continue;

        // Angle span of arcs widens with volume
        int max_deg = 42 + (int)(s_vol_smooth * 18.0f);
        if (max_deg > 60) max_deg = 60;

        for (int deg = -max_deg; deg <= max_deg; deg += 3) {
            float rad = (float)deg * 0.0174532925f;

            // Raw waveform ripple modulation across the sound arc
            int sample_idx = (deg + max_deg) * (int)(n - 1) / (max_deg * 2);
            int32_t samp = (left[sample_idx] + right[sample_idx]) / 2;
            float ripple = ((float)samp / (float)AUDIO_NOMINAL_PEAK) * (3.0f + s_bass_smooth * 4.0f);

            float cur_r = r + ripple;
            int px = origin_x + (int)(cur_r * cosf(rad));
            int py = cy + (int)(cur_r * sinf(rad));

            if (px >= mouth_x + 2 && px < 128 && py >= 0 && py < 64) {
                SafeDraw::drawPixel(px, py);
                
                // Double/triple thick arc lines on intense sound waves
                if (s_vol_smooth > 0.35f && (a == 0 || a == 1 || r < 40.0f)) {
                    SafeDraw::drawPixel(px + 1, py);
                }
                if (s_vol_smooth > 0.70f && a == 0) {
                    SafeDraw::drawPixel(px, py + 1);
                }
            }
        }
    }

    // 6. Acoustic Sound Particles (Bursting outward from speaker mouth)
    for (int i = 0; i < NUM_SOUND_PARTICLES; i++) {
        if (s_particles[i].life <= 0) {
            // Spawn new particle if audio energy is present
            if (s_vol_smooth > 0.15f || s_bass_smooth > 0.20f) {
                s_particles[i].x = (float)(mouth_x + 3);
                s_particles[i].y = (float)(cy + (rand() % (mouth_half_h + 4) - mouth_half_h / 2));
                float angle = ((float)(rand() % 84) - 42.0f) * 0.0174532925f;
                float speed = 1.2f + ((float)(rand() % 100) / 100.0f) * (2.2f + s_vol_smooth * 4.5f + s_bass_smooth * 3.0f);
                s_particles[i].vx = cosf(angle) * speed;
                s_particles[i].vy = sinf(angle) * speed * 0.65f;
                s_particles[i].max_life = 12 + rand() % 18;
                s_particles[i].life = s_particles[i].max_life;
            }
        } else {
            s_particles[i].x += s_particles[i].vx;
            s_particles[i].y += s_particles[i].vy;
            s_particles[i].life--;

            int px = (int)s_particles[i].x;
            int py = (int)s_particles[i].y;
            if (px >= mouth_x + 2 && px < 128 && py >= 0 && py < 64) {
                SafeDraw::drawPixel(px, py);
                // Particle motion trail on fast intense particles
                if (s_particles[i].life > s_particles[i].max_life / 2 && s_vol_smooth > 0.45f) {
                    SafeDraw::drawPixel(px - 1, py);
                }
            }
        }
    }

    // 7. Sound HUD & Dynamic Status Display
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 7, "MVT");

    // Dynamic state icon text based on volume level
    if (s_vol_smooth < 0.12f) {
        SafeDraw::drawStr(2, 63, "MUTE");
    } else if (s_vol_smooth < 0.45f) {
        SafeDraw::drawStr(2, 63, "SOUND");
    } else if (s_vol_smooth < 0.75f) {
        SafeDraw::drawStr(2, 63, "LOUD");
    } else {
        SafeDraw::drawStr(2, 63, "BLAST!");
    }

    // Bottom Sound Intensity dB / Power gauge (x = 74..126, y = 57..63)
    if (s_vol_smooth > 0.05f) {
        int pwr_w = (int)(s_vol_smooth * 48.0f);
        if (pwr_w > 48) pwr_w = 48;
        SafeDraw::drawFrame(76, 58, 50, 5);
        if (pwr_w > 0) {
            SafeDraw::drawBox(77, 59, pwr_w, 3);
        }
        // Small center mark at 50%
        SafeDraw::drawVLine(76 + 25, 57, 2);
    }
}
