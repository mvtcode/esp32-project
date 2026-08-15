#include "effects.h"

// -----------------------------------------------------------------------
// MODE 41 — TEXT PARTICLES (Kinetic Particle Assembly: LOVE -> MAC -> TAN -> HEART)
// -----------------------------------------------------------------------

#define NUM_TEXT_PARTS 96

struct TextParticle {
    float x, y;
    float vx, vy;
    int tx, ty; // Target coordinates
};

enum ParticleState {
    P_ASSEMBLE,
    P_HOLD,
    P_EXPLODE
};

static TextParticle s_tparts[NUM_TEXT_PARTS];
static ParticleState s_pstate = P_ASSEMBLE;
static int s_target_word = 0; // 0: LOVE, 1: MAC, 2: TAN, 3: HEART
static uint32_t s_state_time = 0;
static float s_part_vol = 0.0f;

// Build target coordinates for Word 0: LOVE
static void build_target_love() {
    int idx = 0;
    // 'L' (X: 18..30, Y: 18..46)
    for (int y = 18; y <= 46; y += 3) { if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = 18; s_tparts[idx].ty = y; idx++; } }
    for (int x = 21; x <= 32; x += 3) { if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = x; s_tparts[idx].ty = 46; idx++; } }

    // 'O' (X: 40..56, Y: 18..46)
    for (int y = 18; y <= 46; y += 4) {
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = 40; s_tparts[idx].ty = y; idx++; }
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = 56; s_tparts[idx].ty = y; idx++; }
    }
    for (int x = 44; x <= 52; x += 4) {
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = x; s_tparts[idx].ty = 18; idx++; }
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = x; s_tparts[idx].ty = 46; idx++; }
    }

    // 'V' (X: 66..82, Y: 18..46)
    for (int i = 0; i <= 9; i++) {
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = 66 + i; s_tparts[idx].ty = 18 + i * 3; idx++; }
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = 82 - i; s_tparts[idx].ty = 18 + i * 3; idx++; }
    }

    // 'E' (X: 92..106, Y: 18..46)
    for (int y = 18; y <= 46; y += 3) { if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = 92; s_tparts[idx].ty = y; idx++; } }
    for (int x = 95; x <= 106; x += 4) {
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = x; s_tparts[idx].ty = 18; idx++; }
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = x; s_tparts[idx].ty = 32; idx++; }
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = x; s_tparts[idx].ty = 46; idx++; }
    }

    // Fill remaining
    while (idx < NUM_TEXT_PARTS) {
        s_tparts[idx].tx = 63 + random(-10, 10);
        s_tparts[idx].ty = 31 + random(-10, 10);
        idx++;
    }
}

// Build target coordinates for Word 1: MAC
static void build_target_mac() {
    int idx = 0;
    // 'M' (X: 20..44, Y: 18..46)
    for (int y = 18; y <= 46; y += 3) {
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = 20; s_tparts[idx].ty = y; idx++; }
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = 44; s_tparts[idx].ty = y; idx++; }
    }
    for (int i = 0; i <= 6; i++) {
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = 20 + i * 2; s_tparts[idx].ty = 18 + i * 3; idx++; }
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = 44 - i * 2; s_tparts[idx].ty = 18 + i * 3; idx++; }
    }

    // 'A' (X: 54..78, Y: 18..46)
    for (int i = 0; i <= 9; i++) {
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = 66 - i; s_tparts[idx].ty = 18 + i * 3; idx++; }
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = 66 + i; s_tparts[idx].ty = 18 + i * 3; idx++; }
    }
    for (int x = 60; x <= 72; x += 3) {
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = x; s_tparts[idx].ty = 35; idx++; }
    }

    // 'C' (X: 86..108, Y: 18..46)
    for (int y = 22; y <= 42; y += 3) { if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = 86; s_tparts[idx].ty = y; idx++; } }
    for (int x = 89; x <= 108; x += 3) {
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = x; s_tparts[idx].ty = 18; idx++; }
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = x; s_tparts[idx].ty = 46; idx++; }
    }

    while (idx < NUM_TEXT_PARTS) {
        s_tparts[idx].tx = 63 + random(-10, 10);
        s_tparts[idx].ty = 31 + random(-10, 10);
        idx++;
    }
}

// Build target coordinates for Word 2: TAN
static void build_target_tan() {
    int idx = 0;
    // 'T' (X: 20..44, Y: 18..46)
    for (int x = 20; x <= 44; x += 3) { if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = x; s_tparts[idx].ty = 18; idx++; } }
    for (int y = 21; y <= 46; y += 3) { if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = 32; s_tparts[idx].ty = y; idx++; } }

    // 'A' (X: 54..78, Y: 18..46)
    for (int i = 0; i <= 9; i++) {
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = 66 - i; s_tparts[idx].ty = 18 + i * 3; idx++; }
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = 66 + i; s_tparts[idx].ty = 18 + i * 3; idx++; }
    }
    for (int x = 60; x <= 72; x += 3) {
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = x; s_tparts[idx].ty = 35; idx++; }
    }

    // 'N' (X: 86..108, Y: 18..46)
    for (int y = 18; y <= 46; y += 3) {
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = 86; s_tparts[idx].ty = y; idx++; }
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = 108; s_tparts[idx].ty = y; idx++; }
    }
    for (int i = 0; i <= 7; i++) {
        if (idx < NUM_TEXT_PARTS) { s_tparts[idx].tx = 86 + i * 3; s_tparts[idx].ty = 18 + i * 4; idx++; }
    }

    while (idx < NUM_TEXT_PARTS) {
        s_tparts[idx].tx = 63 + random(-10, 10);
        s_tparts[idx].ty = 31 + random(-10, 10);
        idx++;
    }
}

// Build target coordinates for Word 3: Heart Shape
static void build_target_heart() {
    int idx = 0;
    for (int i = 0; i < NUM_TEXT_PARTS; i++) {
        float t = ((float)i / (float)NUM_TEXT_PARTS) * 6.2831853f;
        float hx = 16.0f * powf(sinf(t), 3.0f);
        float hy = -(13.0f * cosf(t) - 5.0f * cosf(2.0f * t) - 2.0f * cosf(3.0f * t) - cosf(4.0f * t));

        s_tparts[idx].tx = 63 + (int)(hx * 1.5f);
        s_tparts[idx].ty = 31 + (int)(hy * 1.3f);
        idx++;
    }
}

static void apply_target_word(int word) {
    if (word == 0) build_target_love();
    else if (word == 1) build_target_mac();
    else if (word == 2) build_target_tan();
    else build_target_heart();
}

void effect_text_particles_on_enter() {
    s_pstate = P_ASSEMBLE;
    s_target_word = 0;
    s_state_time = millis();
    s_part_vol = 0.0f;

    for (int i = 0; i < NUM_TEXT_PARTS; i++) {
        s_tparts[i].x = (float)random(0, 128);
        s_tparts[i].y = (float)random(0, 64);
        s_tparts[i].vx = ((float)random(-100, 100)) / 40.0f;
        s_tparts[i].vy = ((float)random(-100, 100)) / 40.0f;
    }
    apply_target_word(s_target_word);
}

void effect_text_particles_on_exit() {}

void effect_text_particles_render(const int32_t *left, const int32_t *right, size_t n) {
    // 1. Audio volume measurement
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += abs(left[i]) + abs(right[i]);
    }
    float vol = (float)sum / (float)(n * (s_peak_l + s_peak_r + 1));
    s_part_vol = s_part_vol * 0.7f + vol * 0.3f;

    uint32_t now = millis();
    uint32_t elapsed = now - s_state_time;

    // 2. State Machine Logic
    if (s_pstate == P_ASSEMBLE) {
        if (elapsed > 1600) {
            s_pstate = P_HOLD;
            s_state_time = now;
        }
    } else if (s_pstate == P_HOLD) {
        if (elapsed > 1200 || (vol > 0.8f && elapsed > 600)) {
            s_pstate = P_EXPLODE;
            s_state_time = now;
            // Blast particles outward
            for (int i = 0; i < NUM_TEXT_PARTS; i++) {
                float angle = ((float)random(0, 360) * 3.14159f) / 180.0f;
                float mag = 2.5f + s_part_vol * 4.0f + (float)random(0, 50) * 0.05f;
                s_tparts[i].vx = cosf(angle) * mag;
                s_tparts[i].vy = sinf(angle) * mag;
            }
        }
    } else if (s_pstate == P_EXPLODE) {
        if (elapsed > 550) {
            s_pstate = P_ASSEMBLE;
            s_state_time = now;
            s_target_word = (s_target_word + 1) % 4;
            apply_target_word(s_target_word);
        }
    }

    // 3. Physics Simulation & Rendering
    for (int i = 0; i < NUM_TEXT_PARTS; i++) {
        if (s_pstate == P_ASSEMBLE || s_pstate == P_HOLD) {
            // Spring force towards target
            float dx = (float)s_tparts[i].tx - s_tparts[i].x;
            float dy = (float)s_tparts[i].ty - s_tparts[i].y;

            // Audio vibration jitter
            float jitter = (s_pstate == P_HOLD) ? s_part_vol * 2.0f : 0.0f;
            float jx = ((float)random(-10, 10) / 10.0f) * jitter;
            float jy = ((float)random(-10, 10) / 10.0f) * jitter;

            s_tparts[i].vx = s_tparts[i].vx * 0.82f + (dx * 0.12f) + jx;
            s_tparts[i].vy = s_tparts[i].vy * 0.82f + (dy * 0.12f) + jy;
        } else if (s_pstate == P_EXPLODE) {
            // Drag deceleration
            s_tparts[i].vx *= 0.94f;
            s_tparts[i].vy *= 0.94f;
        }

        s_tparts[i].x += s_tparts[i].vx;
        s_tparts[i].y += s_tparts[i].vy;

        int sx = (int)s_tparts[i].x;
        int sy = (int)s_tparts[i].y;

        SafeDraw::drawPixel(sx, sy);
        // Draw dual pixel for larger particle when in HOLD state
        if (s_pstate == P_HOLD && (i % 3 == 0)) {
            SafeDraw::drawPixel(sx + 1, sy);
        }
    }

    // 4. Cyber Hologram Banner on Top/Bottom
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 6, "KINETIC");
    const char *state_labels[] = { "ASSEMBLING", "HOLD BEAT", "EXPLODE!" };
    SafeDraw::drawStr(78, 6, state_labels[s_pstate]);
}
