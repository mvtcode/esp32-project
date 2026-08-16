#include "effects.h"

// -----------------------------------------------------------------------
// MODE 59 — MVT DINO RUNNER (Pixel T-Rex, Bass Jump & Desert Obstacles)
// -----------------------------------------------------------------------
struct Obstacle {
    float x;
    int type; // 0: Small Cactus, 1: Double Cactus, 2: Flying Bird
};

static Obstacle s_cacti[2];
static float s_dino_y = 48.0f;
static float s_dino_vy = 0.0f;
static bool s_is_jumping = false;
static float s_desert_scroll = 0.0f;
static int s_leg_frame = 0;

void effect_dino_on_enter() {
    s_dino_y = 48.0f;
    s_dino_vy = 0.0f;
    s_is_jumping = false;
    s_desert_scroll = 0.0f;
    s_leg_frame = 0;

    s_cacti[0] = { 80.0f, 0 };
    s_cacti[1] = { 140.0f, 1 };
}

void effect_dino_on_exit() {}

// Draw T-Rex sprite (10x12)
static void draw_dino(int x, int y, int leg) {
    // Head & Snout
    SafeDraw::drawBox(x + 4, y - 11, 7, 4);
    SafeDraw::drawPixel(x + 6, y - 10); // Eye (cutout will be visible)
    SafeDraw::drawBox(x + 2, y - 7, 6, 3); // Neck
    SafeDraw::drawBox(x - 2, y - 4, 9, 6); // Body
    SafeDraw::drawPixel(x + 7, y - 2);     // Tiny arm

    // Tail
    SafeDraw::drawPixel(x - 3, y - 3);
    SafeDraw::drawPixel(x - 4, y - 2);

    // Legs animation
    if (s_is_jumping) {
        SafeDraw::drawVLine(x, y + 2, 2);
        SafeDraw::drawVLine(x + 3, y + 2, 2);
    } else if (leg == 0) {
        SafeDraw::drawVLine(x, y + 2, 3);
        SafeDraw::drawPixel(x + 3, y + 2);
    } else {
        SafeDraw::drawPixel(x, y + 2);
        SafeDraw::drawVLine(x + 3, y + 2, 3);
    }
}

// Draw Cactus
static void draw_cactus(int x, int y, int type) {
    if (type == 0) {
        // Single Cactus
        SafeDraw::drawBox(x + 2, y - 8, 2, 9);
        SafeDraw::drawHLine(x, y - 5, 2);
        SafeDraw::drawVLine(x, y - 7, 3);
        SafeDraw::drawHLine(x + 4, y - 4, 2);
        SafeDraw::drawVLine(x + 5, y - 6, 3);
    } else if (type == 1) {
        // Double Cactus
        SafeDraw::drawBox(x + 1, y - 9, 2, 10);
        SafeDraw::drawBox(x + 6, y - 7, 2, 8);
    } else {
        // Pterodactyl Bird
        SafeDraw::drawBox(x, y - 14, 5, 2);
        SafeDraw::drawPixel(x - 2, y - 15);
        SafeDraw::drawPixel(x + 6, y - 13);
    }
}

void effect_dino_render(const int32_t *left, const int32_t *right, size_t n) {
    const int ground_y = 52;

    // 1. RMS Volume
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

    // FFT for Bass Kick (Triggers Dino Jump)
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

    // Trigger jump on bass
    if (bass > 0.42f && !s_is_jumping) {
        s_dino_vy = -4.2f - bass * 1.5f;
        s_is_jumping = true;
    }

    // 2. Physics Update for Dino
    if (s_is_jumping) {
        s_dino_y += s_dino_vy;
        s_dino_vy += 0.45f; // Gravity
        if (s_dino_y >= (float)ground_y - 4.0f) {
            s_dino_y = (float)ground_y - 4.0f;
            s_dino_vy = 0.0f;
            s_is_jumping = false;
        }
    }

    // 3. Move Obstacles & Ground
    float spd = 1.8f + avg * 2.2f;
    s_desert_scroll += spd;
    if (s_desert_scroll > 16.0f) s_desert_scroll -= 16.0f;

    for (int i = 0; i < 2; i++) {
        s_cacti[i].x -= spd;
        if (s_cacti[i].x < -10.0f) {
            s_cacti[i].x = 128.0f + (rand() % 40);
            s_cacti[i].type = rand() % 3;
        }
        draw_cactus((int)s_cacti[i].x, ground_y, s_cacti[i].type);
    }

    // 4. Draw Dino
    s_leg_frame = (millis() / 100) % 2;
    draw_dino(24, (int)s_dino_y, s_leg_frame);

    // 5. Draw Desert Ground & Skyline
    SafeDraw::drawHLine(0, ground_y, 128);
    for (int gx = 0; gx < 128; gx += 16) {
        int px = gx - (int)s_desert_scroll;
        if (px >= 0 && px < 126) {
            SafeDraw::drawPixel(px, ground_y + 2);
            SafeDraw::drawPixel(px + 4, ground_y + 3);
            SafeDraw::drawPixel(px + 9, ground_y + 2);
        }
    }

    // Distant Sun & Clouds
    int sun_r = 6 + (int)(bass * 2.5f);
    SafeDraw::drawCircle(104, 14, sun_r);

    SafeDraw::drawHLine(40, 10, 12);
    SafeDraw::drawHLine(36, 12, 18);

    // 6. HUD Telemetry
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 6, "HI 09999");
    SafeDraw::drawStr(96, 62, "MVT-DINO");
}
