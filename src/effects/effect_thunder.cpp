#include "effects.h"

// -----------------------------------------------------------------------
// MODE 54 — MVT THUNDERSTORM (Fractal Lightning, Stereo Rain & Ocean Waves)
// -----------------------------------------------------------------------
struct RainDrop {
    float x, y;
    float spd;
    float len;
};

static RainDrop s_rain[30];
static int s_lightning_trigger = 0;
static int s_lightning_x = 64;

void effect_thunder_on_enter() {
    s_lightning_trigger = 0;
    s_lightning_x = 64;
    for (int i = 0; i < 30; i++) {
        s_rain[i].x = (float)(rand() % 128);
        s_rain[i].y = (float)(rand() % 50);
        s_rain[i].spd = 2.0f + (float)(rand() % 20) / 10.0f;
        s_rain[i].len = 3.0f + (float)(rand() % 4);
    }
}

void effect_thunder_on_exit() {}

void effect_thunder_render(const int32_t *left, const int32_t *right, size_t n) {
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
    float wind = (norm_r - norm_l) * 2.5f; // Stereo wind shear

    // Audio analysis — use pre-computed frame bands
    const float bass = g_frame_bands.bass;

    if (bass > 0.45f && s_lightning_trigger == 0) {
        s_lightning_trigger = 3; // Active for 3 frames
        s_lightning_x = 24 + rand() % 80;
    }

    // 2. Storm Clouds (Top layer)
    for (int c = 0; c < 7; c++) {
        int cx = c * 20 + 8;
        int cr = 10 + (c % 3) * 3 + (int)(bass * 2.0f);
        SafeDraw::drawCircle(cx, 2, cr);
    }
    SafeDraw::drawHLine(0, 8, 128);

    // 3. Lightning Strike (Fractal zig-zag branch)
    if (s_lightning_trigger > 0) {
        int lx = s_lightning_x;
        int ly = 8;
        while (ly < 50) {
            int nlx = lx + (rand() % 9 - 4);
            int nly = ly + 4 + rand() % 5;
            SafeDraw::drawLine(lx, ly, nlx, nly);
            SafeDraw::drawLine(lx + 1, ly, nlx + 1, nly); // Bold beam

            // Side branch
            if (rand() % 3 == 0) {
                int bx = nlx + (rand() % 11 - 5);
                int by = nly + 4;
                SafeDraw::drawLine(nlx, nly, bx, by);
            }

            lx = nlx;
            ly = nly;
        }
        s_lightning_trigger--;
    }

    // 4. Stereo Wind Rain Drops
    for (int i = 0; i < 30; i++) {
        s_rain[i].y += s_rain[i].spd * (1.0f + avg * 1.5f);
        s_rain[i].x += wind;

        if (s_rain[i].y > 52.0f) {
            s_rain[i].y = 9.0f;
            s_rain[i].x = (float)(rand() % 138 - 5);
        }
        if (s_rain[i].x < 0) s_rain[i].x += 128.0f;
        if (s_rain[i].x > 127) s_rain[i].x -= 128.0f;

        int x1 = (int)s_rain[i].x;
        int y1 = (int)s_rain[i].y;
        int x2 = x1 + (int)(wind * 1.2f);
        int y2 = y1 + (int)s_rain[i].len;
        SafeDraw::drawLine(x1, y1, x2, y2);
    }

    // 5. Ocean Waveform (Bottom 52..63)
    int prev_ox = 0, prev_oy = 56;
    for (int col = 0; col < 128; col += 2) {
        int s_idx = (col * (int)n) / 128;
        float sample = (float)left[s_idx] / (float)AUDIO_NOMINAL_PEAK;
        int oy = 56 + (int)(sample * 6.0f + sinf(col * 0.15f + (float)millis() * 0.005f) * 2.0f);
        if (oy < 50) oy = 50;
        if (oy > 63) oy = 63;
        if (col > 0) {
            SafeDraw::drawLine(prev_ox, prev_oy, col, oy);
        }
        prev_ox = col;
        prev_oy = oy;
    }

    // 6. HUD Telemetry
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 7, "THUNDER TEMPEST");
    SafeDraw::drawStr(98, 62, "MVT-STORM");
}
