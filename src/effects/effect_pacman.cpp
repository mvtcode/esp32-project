#include "effects.h"

// -----------------------------------------------------------------------
// MODE 58 — MVT PAC-BEAT (Live Waveform Mouth, Ghost Chase & Power Pellet)
// -----------------------------------------------------------------------
static float s_pac_x = 40.0f;
static float s_ghost1_x = 90.0f;
static float s_ghost2_x = 110.0f;
static float s_dot_offset = 0.0f;
static int s_power_mode = 0; // > 0 when power pellet active

void effect_pacman_on_enter() {
    s_pac_x = 40.0f;
    s_ghost1_x = 85.0f;
    s_ghost2_x = 105.0f;
    s_dot_offset = 0.0f;
    s_power_mode = 0;
}

void effect_pacman_on_exit() {}

// Draw Pacman with dynamic mouth angle
static void draw_pacman(int cx, int cy, int r, float mouth_angle, int dir) {
    SafeDraw::drawDisc(cx, cy, r);
    // Cut out mouth wedge
    if (mouth_angle > 0.05f) {
        SafeDraw::setDrawColor(0);
        int mx = cx + (int)(cosf(dir == 1 ? 0 : 3.14159f) * (r + 2));
        int my1 = cy - (int)(sinf(mouth_angle) * (r + 2));
        int my2 = cy + (int)(sinf(mouth_angle) * (r + 2));
        SafeDraw::drawTriangle(cx, cy, mx, my1, mx, my2);
        SafeDraw::setDrawColor(1);
    }
}

// Draw Ghost sprite (Blinky / Scared Ghost)
static void draw_ghost(int x, int y, bool scared, int anim) {
    SafeDraw::drawBox(x - 4, y - 2, 9, 6);
    SafeDraw::drawCircle(x, y - 3, 4);
    // Skirt tentacles
    if (anim == 0) {
        SafeDraw::drawPixel(x - 4, y + 4);
        SafeDraw::drawPixel(x - 2, y + 4);
        SafeDraw::drawPixel(x, y + 4);
        SafeDraw::drawPixel(x + 2, y + 4);
        SafeDraw::drawPixel(x + 4, y + 4);
    } else {
        SafeDraw::drawPixel(x - 3, y + 4);
        SafeDraw::drawPixel(x - 1, y + 4);
        SafeDraw::drawPixel(x + 1, y + 4);
        SafeDraw::drawPixel(x + 3, y + 4);
    }

    // Eyes
    if (!scared) {
        SafeDraw::setDrawColor(0);
        SafeDraw::drawPixel(x - 2, y - 2);
        SafeDraw::drawPixel(x + 2, y - 2);
        SafeDraw::setDrawColor(1);
    } else {
        // Scared wavy mouth
        SafeDraw::setDrawColor(0);
        SafeDraw::drawHLine(x - 2, y, 5);
        SafeDraw::setDrawColor(1);
    }
}

void effect_pacman_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cy = 34;

    // 1. RMS & Beat Detection
    int64_t sum_l = 0, sum_r = 0;
    for (size_t i = 0; i < n; i++) {
        sum_l += (left[i] < 0 ? -left[i] : left[i]);
        sum_r += (right[i] < 0 ? -right[i] : right[i]);
    }
    float norm_l = (float)sum_l / ((float)n * (float)s_peak_l);
    float norm_r = (float)sum_r / ((float)n * (float)s_peak_r);
    if (norm_l > 1.0f) norm_l = 1.0f;
    if (norm_r > 1.0f) norm_r = 1.0f;
    float avg = (norm_l + norm_r) * 0.5f;

    // Audio analysis — use pre-computed frame bands
    const float bass = g_frame_bands.bass;

    if (bass > 0.50f) {
        s_power_mode = 30; // 30 frames of power mode!
    }
    if (s_power_mode > 0) s_power_mode--;

    // 2. Maze Borders (Top & Bottom Retro Maze Lines)
    SafeDraw::drawHLine(0, 16, 128);
    SafeDraw::drawHLine(0, 18, 128);
    SafeDraw::drawHLine(0, 50, 128);
    SafeDraw::drawHLine(0, 52, 128);

    // Maze Blocks
    SafeDraw::drawBox(10, 8, 20, 4);
    SafeDraw::drawBox(54, 8, 20, 4);
    SafeDraw::drawBox(98, 8, 20, 4);

    // 3. Scroll Food Dots (and Power Pellets)
    s_dot_offset += 1.2f + avg * 2.0f;
    if (s_dot_offset > 12.0f) s_dot_offset -= 12.0f;

    for (int col = 0; col < 128; col += 12) {
        int dx = col - (int)s_dot_offset;
        if (dx >= 0 && dx < 128) {
            // Check if power pellet
            if (col % 48 == 0) {
                SafeDraw::drawCircle(dx, cy, 3);
            } else {
                SafeDraw::drawPixel(dx, cy);
                SafeDraw::drawPixel(dx + 1, cy);
            }
        }
    }

    // 4. Live Waveform Mouth opening
    float mouth_angle = 0.2f + avg * 0.75f;
    if (mouth_angle > 0.9f) mouth_angle = 0.9f;

    // 5. Draw Pacman & Ghost Chase Logic
    int anim = (millis() / 150) % 2;
    if (s_power_mode > 0) {
        // Pacman chasing scared ghosts to the right
        draw_pacman((int)s_pac_x, cy, 7, mouth_angle, 1);
        draw_ghost((int)s_ghost1_x, cy, true, anim);
        draw_ghost((int)s_ghost2_x, cy, true, 1 - anim);
    } else {
        // Pacman running from ghosts to the right
        draw_pacman((int)s_pac_x, cy, 7, mouth_angle, 1);
        draw_ghost((int)s_ghost1_x, cy, false, anim);
        draw_ghost((int)s_ghost2_x, cy, false, 1 - anim);
    }

    // 6. HUD Telemetry
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 6, "HIGH SCORE 3333360");
    if (s_power_mode > 0) {
        SafeDraw::drawStr(50, 60, "** POWER BEAT **");
    } else {
        SafeDraw::drawStr(96, 62, "MVT-PAC");
    }
}
