#include "effects.h"

// -----------------------------------------------------------------------
// MODE 53 — MVT SUPERNOVA (Cosmic Stellar Explosion & Plasma Remnants)
// -----------------------------------------------------------------------
struct DebrisParticle {
    float x, y;
    float vx, vy;
    float life;
};

static DebrisParticle s_debris[40];
static float s_shockwave_r = 0.0f;
static float s_core_phase = 0.0f;

void effect_supernova_on_enter() {
    s_shockwave_r = 0.0f;
    s_core_phase = 0.0f;
    for (int i = 0; i < 40; i++) {
        s_debris[i].x = 64.0f;
        s_debris[i].y = 31.0f;
        float ang = ((float)rand() / (float)RAND_MAX) * 6.28318f;
        // Slowed down to 1/4 speed
        float spd = 0.15f + ((float)rand() / (float)RAND_MAX) * 0.55f;
        s_debris[i].vx = cosf(ang) * spd;
        s_debris[i].vy = sinf(ang) * (spd * 0.55f);
        s_debris[i].life = (float)(rand() % 100) / 100.0f;
    }
}

void effect_supernova_on_exit() {}

void effect_supernova_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 64, cy = 31;

    // 1. Audio analysis — use pre-computed frame bands
    const float bass = g_frame_bands.bass;

    // 2. Shockwave Expansion (Slowed down to 1/4 speed)
    s_shockwave_r += 0.30f + bass * 0.88f;
    if (bass > 0.6f && s_shockwave_r > 35.0f) {
        s_shockwave_r = 4.0f; // Re-trigger explosion
    }
    if (s_shockwave_r > 60.0f) s_shockwave_r = 60.0f;

    // 3. Draw Expanding Shockwave Ellipses
    if (s_shockwave_r < 58.0f) {
        SafeDraw::drawEllipse(cx, cy, (int)s_shockwave_r, (int)(s_shockwave_r * 0.55f));
        if (s_shockwave_r > 10.0f) {
            SafeDraw::drawEllipse(cx, cy, (int)(s_shockwave_r * 0.7f), (int)(s_shockwave_r * 0.7f * 0.55f));
        }
    }

    // 4. Update and Draw Debris Particles (Slowed down to 1/4 speed)
    for (int i = 0; i < 40; i++) {
        s_debris[i].x += s_debris[i].vx * (1.0f + bass * 1.5f);
        s_debris[i].y += s_debris[i].vy * (1.0f + bass * 1.5f);
        s_debris[i].life -= 0.00625f; // Decays 4x slower so particles float gracefully

        if (s_debris[i].life <= 0.0f || s_debris[i].x < 0 || s_debris[i].x > 127 || s_debris[i].y < 0 || s_debris[i].y > 63) {
            s_debris[i].x = cx + (float)(rand() % 7 - 3);
            s_debris[i].y = cy + (float)(rand() % 5 - 2);
            float ang = ((float)rand() / (float)RAND_MAX) * 6.28318f;
            float spd = 0.15f + ((float)rand() / (float)RAND_MAX) * (0.38f + bass * 0.62f);
            s_debris[i].vx = cosf(ang) * spd;
            s_debris[i].vy = sinf(ang) * (spd * 0.55f);
            s_debris[i].life = 1.0f;
        }

        int px = (int)s_debris[i].x;
        int py = (int)s_debris[i].y;
        SafeDraw::drawPixel(px, py);
        if (bass > 0.4f && (i % 2 == 0)) {
            SafeDraw::drawPixel(px + 1, py);
        }
    }

    // 5. Draw Deforming Live Nebula Remnant (Rotation slowed down to 1/4 speed)
    s_core_phase += 0.02f;
    for (int step = 0; step < 24; step++) {
        float a1 = (float)step * (6.28318f / 24.0f) + s_core_phase;
        float a2 = (float)(step + 1) * (6.28318f / 24.0f) + s_core_phase;
        
        int s_idx1 = (step * 4) % (n > 0 ? n : 1);
        int s_idx2 = ((step + 1) * 4) % (n > 0 ? n : 1);
        float sample_val1 = (float)left[s_idx1] / (float)AUDIO_NOMINAL_PEAK;
        float sample_val2 = (float)left[s_idx2] / (float)AUDIO_NOMINAL_PEAK;

        float rad1 = 10.0f + bass * 12.0f + sample_val1 * 5.0f;
        float rad2 = 10.0f + bass * 12.0f + sample_val2 * 5.0f;
        if (rad1 < 3.0f) rad1 = 3.0f;
        if (rad2 < 3.0f) rad2 = 3.0f;

        int x1 = cx + (int)(cosf(a1) * rad1);
        int y1 = cy + (int)(sinf(a1) * rad1 * 0.55f);
        int x2 = cx + (int)(cosf(a2) * rad2);
        int y2 = cy + (int)(sinf(a2) * rad2 * 0.55f);

        SafeDraw::drawLine(x1, y1, x2, y2);
    }

    // 6. Central Superdense Core (Neutron Star / Pulsar)
    int core_r = 2 + (int)(bass * 3.0f);
    SafeDraw::drawDisc(cx, cy, core_r);

    // 7. HUD Telemetry
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 7, "SUPERNOVA BLAST");
    SafeDraw::drawStr(98, 62, "MVT-NOVA");
}
