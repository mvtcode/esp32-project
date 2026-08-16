#include "effects.h"

// -----------------------------------------------------------------------
// MODE 57 — MVT FLAPPY BEAT (Retro Bird Altitude & FFT Pipe Obstacles)
// -----------------------------------------------------------------------
struct Pipe {
    float x;
    int gap_y;    // Center of the opening gap
    int gap_h;    // Height of the opening gap
};

static Pipe s_pipes[3];
static float s_bird_y = 30.0f;
static float s_bird_vy = 0.0f;
static int s_wing_state = 0;
static float s_ground_scroll = 0.0f;

void effect_flappy_on_enter() {
    s_bird_y = 30.0f;
    s_bird_vy = 0.0f;
    s_wing_state = 0;
    s_ground_scroll = 0.0f;

    for (int i = 0; i < 3; i++) {
        s_pipes[i].x = 60.0f + i * 46.0f;
        s_pipes[i].gap_y = 20 + rand() % 16;
        s_pipes[i].gap_h = 16;
    }
}

void effect_flappy_on_exit() {}

// Draw 9x7 Flappy Bird sprite
static void draw_bird_sprite(int x, int y, int wing) {
    SafeDraw::drawBox(x - 3, y - 2, 7, 5); // Body
    SafeDraw::drawPixel(x + 2, y - 1);     // Eye
    SafeDraw::drawBox(x + 4, y, 3, 2);     // Beak
    // Wing
    if (wing == 0) {
        SafeDraw::drawHLine(x - 3, y, 4); // Neutral wing
    } else if (wing == 1) {
        SafeDraw::drawHLine(x - 3, y + 2, 4); // Flapping down
    } else {
        SafeDraw::drawHLine(x - 3, y - 3, 4); // Flapping up
    }
}

void effect_flappy_render(const int32_t *left, const int32_t *right, size_t n) {
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

    // FFT for Bass Beat & Pipe Modulation
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

    // 2. Bird Physics & Audio Reactive Altitude
    float target_bird_y = 48.0f - (avg * 38.0f); // Higher volume = higher altitude
    s_bird_y += (target_bird_y - s_bird_y) * 0.18f;
    if (bass > 0.45f) {
        s_bird_y -= 4.0f; // Flap boost on bass kick
        s_wing_state = 2;
    } else {
        s_wing_state = (s_wing_state + 1) % 2;
    }
    if (s_bird_y < 6.0f) s_bird_y = 6.0f;
    if (s_bird_y > 50.0f) s_bird_y = 50.0f;

    // 3. Move & Draw FFT Pipes
    float scroll_spd = 1.0f + avg * 1.5f;
    s_ground_scroll += scroll_spd;
    if (s_ground_scroll > 8.0f) s_ground_scroll -= 8.0f;

    for (int i = 0; i < 3; i++) {
        s_pipes[i].x -= scroll_spd;
        if (s_pipes[i].x < -14.0f) {
            s_pipes[i].x = 128.0f + (rand() % 16);
            int freq_bin = 2 + i * 5;
            float mag = s_fft_real[freq_bin] / (float)s_peak_l;
            if (mag > 1.0f) mag = 1.0f;
            s_pipes[i].gap_y = 16 + (int)(mag * 22.0f);
            s_pipes[i].gap_h = 16;
        }

        int px = (int)s_pipes[i].x;
        int top_h = s_pipes[i].gap_y - s_pipes[i].gap_h / 2;
        int bot_y = s_pipes[i].gap_y + s_pipes[i].gap_h / 2;

        if (px >= -12 && px < 128) {
            // Top Pipe
            if (top_h > 0) {
                SafeDraw::drawFrame(px, 0, 10, top_h);
                SafeDraw::drawBox(px - 1, top_h - 3, 12, 3); // Pipe rim
            }
            // Bottom Pipe
            if (bot_y < 56) {
                SafeDraw::drawFrame(px, bot_y, 10, 56 - bot_y);
                SafeDraw::drawBox(px - 1, bot_y, 12, 3);     // Pipe rim
            }
        }
    }

    // 4. Draw Flappy Bird
    draw_bird_sprite(28, (int)s_bird_y, s_wing_state);

    // 5. Draw Ground & Clouds
    SafeDraw::drawHLine(0, 56, 128);
    for (int gx = 0; gx < 128; gx += 8) {
        int lx = gx - (int)s_ground_scroll;
        if (lx >= 0 && lx < 128) {
            SafeDraw::drawLine(lx, 56, lx + 4, 63);
        }
    }

    // Mini Clouds
    SafeDraw::drawCircle(70, 6, 4);
    SafeDraw::drawCircle(75, 5, 5);
    SafeDraw::drawCircle(81, 6, 4);

    // 6. HUD Telemetry
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 6, "FLAPPY AUDIO");
    SafeDraw::drawStr(98, 6, "MVT-BIRD");
}
