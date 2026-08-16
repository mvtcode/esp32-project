#include "effects.h"

// -----------------------------------------------------------------------
// MODE 56 — MVT SPACE INVADERS (Retro Arcade Cannon, Stereo Dodge & Laser Bass)
// -----------------------------------------------------------------------
struct Alien {
    float x, y;
    bool alive;
    int respawn_timer;
};

struct Laser {
    float x, y;
    bool active;
};

struct Spark {
    float x, y;
    float vx, vy;
    int life;
};

static Alien s_aliens[12];
static Laser s_lasers[3];
static Spark s_sparks[16];
static float s_alien_dir = 1.0f;
static float s_alien_offset_x = 0.0f;
static float s_cannon_x = 64.0f;
static int s_anim_frame = 0;

void effect_invaders_on_enter() {
    s_cannon_x = 64.0f;
    s_alien_offset_x = 0.0f;
    s_alien_dir = 1.0f;
    s_anim_frame = 0;

    for (int i = 0; i < 12; i++) {
        int row = i / 6;
        int col = i % 6;
        s_aliens[i].x = 16.0f + col * 18.0f;
        s_aliens[i].y = 8.0f + row * 12.0f;
        s_aliens[i].alive = true;
        s_aliens[i].respawn_timer = 0;
    }
    for (int i = 0; i < 3; i++) s_lasers[i].active = false;
    for (int i = 0; i < 16; i++) s_sparks[i].life = 0;
}

void effect_invaders_on_exit() {}

// Draw 7x5 pixel classic alien sprite
static void draw_alien_sprite(int x, int y, int frame) {
    if (frame == 0) {
        // Frame A
        SafeDraw::drawHLine(x + 2, y, 3);
        SafeDraw::drawHLine(x + 1, y + 1, 5);
        SafeDraw::drawHLine(x, y + 2, 7);
        SafeDraw::drawPixel(x + 1, y + 3);
        SafeDraw::drawPixel(x + 5, y + 3);
        SafeDraw::drawPixel(x, y + 4);
        SafeDraw::drawPixel(x + 6, y + 4);
    } else {
        // Frame B
        SafeDraw::drawHLine(x + 2, y, 3);
        SafeDraw::drawHLine(x + 1, y + 1, 5);
        SafeDraw::drawHLine(x, y + 2, 7);
        SafeDraw::drawPixel(x + 2, y + 3);
        SafeDraw::drawPixel(x + 4, y + 3);
        SafeDraw::drawPixel(x + 1, y + 4);
        SafeDraw::drawPixel(x + 5, y + 4);
    }
}

// Draw player cannon sprite
static void draw_cannon_sprite(int x, int y) {
    SafeDraw::drawBox(x - 6, y + 3, 13, 3);
    SafeDraw::drawBox(x - 4, y + 1, 9, 2);
    SafeDraw::drawBox(x - 1, y - 1, 3, 2);
}

void effect_invaders_render(const int32_t *left, const int32_t *right, size_t n) {
    // 1. RMS & Stereo Balance
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

    // Cannon smoothly moves left/right based on Stereo Balance
    float target_x = 64.0f + (norm_r - norm_l) * 48.0f;
    s_cannon_x += (target_x - s_cannon_x) * 0.25f;

    // 2. Bass Beat detection for Laser Fire
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

    // Fire laser on bass hit
    if (bass > 0.45f) {
        for (int i = 0; i < 3; i++) {
            if (!s_lasers[i].active) {
                s_lasers[i].x = s_cannon_x;
                s_lasers[i].y = 54.0f;
                s_lasers[i].active = true;
                break;
            }
        }
    }

    // 3. Move Alien Formation
    float alien_spd = 0.5f + avg * 1.5f;
    s_alien_offset_x += s_alien_dir * alien_spd;
    if (s_alien_offset_x > 12.0f) {
        s_alien_dir = -1.0f;
        s_anim_frame = 1 - s_anim_frame;
    } else if (s_alien_offset_x < -12.0f) {
        s_alien_dir = 1.0f;
        s_anim_frame = 1 - s_anim_frame;
    }

    // 4. Update & Draw Lasers & Collisions
    for (int i = 0; i < 3; i++) {
        if (s_lasers[i].active) {
            s_lasers[i].y -= 4.5f;
            SafeDraw::drawVLine((int)s_lasers[i].x, (int)s_lasers[i].y, 4);

            // Check hit against aliens
            for (int a = 0; a < 12; a++) {
                if (s_aliens[a].alive) {
                    float ax = s_aliens[a].x + s_alien_offset_x;
                    float ay = s_aliens[a].y;
                    if (s_lasers[i].x >= ax - 2 && s_lasers[i].x <= ax + 9 &&
                        s_lasers[i].y >= ay && s_lasers[i].y <= ay + 6) {
                        // Alien killed!
                        s_aliens[a].alive = false;
                        s_aliens[a].respawn_timer = 40;
                        s_lasers[i].active = false;

                        // Spawn explosion sparks
                        for (int s = 0; s < 8; s++) {
                            int s_idx = (a * 2 + s) % 16;
                            s_sparks[s_idx].x = ax + 3.0f;
                            s_sparks[s_idx].y = ay + 2.0f;
                            float sang = ((float)rand() / (float)RAND_MAX) * 6.28318f;
                            float sspd = 1.0f + (float)(rand() % 20) / 10.0f;
                            s_sparks[s_idx].vx = cosf(sang) * sspd;
                            s_sparks[s_idx].vy = sinf(sang) * sspd;
                            s_sparks[s_idx].life = 12;
                        }
                        break;
                    }
                }
            }

            if (s_lasers[i].y < 0) s_lasers[i].active = false;
        }
    }

    // 5. Update and Draw Aliens
    for (int a = 0; a < 12; a++) {
        if (s_aliens[a].alive) {
            int ax = (int)(s_aliens[a].x + s_alien_offset_x);
            int ay = (int)s_aliens[a].y;
            draw_alien_sprite(ax, ay, s_anim_frame);
        } else {
            s_aliens[a].respawn_timer--;
            if (s_aliens[a].respawn_timer <= 0) {
                s_aliens[a].alive = true;
            }
        }
    }

    // 6. Update and Draw Explosion Sparks
    for (int s = 0; s < 16; s++) {
        if (s_sparks[s].life > 0) {
            s_sparks[s].x += s_sparks[s].vx;
            s_sparks[s].y += s_sparks[s].vy;
            s_sparks[s].life--;
            SafeDraw::drawPixel((int)s_sparks[s].x, (int)s_sparks[s].y);
        }
    }

    // 7. Draw Player Cannon & Base Line
    SafeDraw::drawHLine(0, 60, 128);
    draw_cannon_sprite((int)s_cannon_x, 56);

    // 8. HUD Telemetry
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 6, "SCORE<1> 1980");
    SafeDraw::drawStr(92, 6, "MVT-ARCADE");
}
