#include "effects.h"

// -----------------------------------------------------------------------
// MODE 31 — MVT JUGGLE (Physics Ball Juggling on Live Audio Waveform)
// -----------------------------------------------------------------------
struct JuggleBall {
    float x, y;
    float vx, vy;
    float spin;
    float spin_v;
};

struct BounceRipple {
    int x, y;
    int r;
    int life;
};

static JuggleBall   s_ball;
static const int    MAX_RIPPLES = 4;
static BounceRipple s_ripples[MAX_RIPPLES];
static bool         s_ball_init = false;

static void init_juggle_ball() {
    s_ball.x = 64.0f;
    s_ball.y = 15.0f;
    s_ball.vx = 0.8f;
    s_ball.vy = 0.0f;
    s_ball.spin = 0.0f;
    s_ball.spin_v = 0.05f;

    for (int i = 0; i < MAX_RIPPLES; i++) {
        s_ripples[i].life = 0;
    }
    s_ball_init = true;
}

void effect_ball_juggle_on_enter() {
    init_juggle_ball();
}

void effect_ball_juggle_on_exit() {}

void effect_ball_juggle_render(const int32_t *left, const int32_t *right, size_t n) {
    if (!s_ball_init) init_juggle_ball();

    const int WAVE_Y = 48;
    const float MAX_WAVE_AMP = 14.0f;
    const int BALL_R = 4;

    int32_t pk = s_peak_l > s_peak_r ? s_peak_l : s_peak_r;
    if (pk < 1) pk = 1;

    // 1. Build and Draw Live Audio Waveform Floor across X = 0..127
    static int wave_y[SCREEN_W]; // static: avoid 512B stack allocation per frame
    int prev_wx = 0, prev_wy = WAVE_Y;

    for (int x = 0; x < SCREEN_W; x++) {
        int idx = x * (int)(n - 1) / (SCREEN_W - 1);
        int32_t samp = (left[idx] + right[idx]) / 2;
        float norm = (float)samp / (float)pk;
        
        int wy = WAVE_Y - (int)(norm * MAX_WAVE_AMP + 0.5f);
        if (wy < 28) wy = 28;
        if (wy > 62) wy = 62;
        wave_y[x] = wy;

        if (x > 0) {
            SafeDraw::drawLine(prev_wx, prev_wy, x, wy);
        }
        prev_wx = x;
        prev_wy = wy;
    }

    // Subtle wave mesh fill below the waveform line
    for (int x = 0; x < SCREEN_W; x += 4) {
        SafeDraw::drawVLine(x, wave_y[x], 63 - wave_y[x]);
    }

    // 2. Ball Physics Update
    const float GRAVITY = 0.26f;
    s_ball.vy += GRAVITY;
    s_ball.x  += s_ball.vx;
    s_ball.y  += s_ball.vy;
    s_ball.spin += s_ball.spin_v;

    // Side wall bounce
    if (s_ball.x - BALL_R < 0.0f) {
        s_ball.x = (float)BALL_R;
        s_ball.vx = -s_ball.vx * 0.90f;
        s_ball.spin_v = -s_ball.spin_v;
    } else if (s_ball.x + BALL_R > 127.0f) {
        s_ball.x = 127.0f - (float)BALL_R;
        s_ball.vx = -s_ball.vx * 0.90f;
        s_ball.spin_v = -s_ball.spin_v;
    }

    // Ceiling clamp
    if (s_ball.y - BALL_R < 0.0f) {
        s_ball.y = (float)BALL_R;
        s_ball.vy = fabsf(s_ball.vy) * 0.5f;
    }

    // 3. Collision with Live Waveform Surface
    int bx_int = (int)s_ball.x;
    if (bx_int < 1) bx_int = 1;
    if (bx_int > 126) bx_int = 126;

    int floor_y = wave_y[bx_int];

    if (s_ball.y + BALL_R >= floor_y) {
        s_ball.y = (float)(floor_y - BALL_R);

        // Surface slope (gradient) at contact point
        float slope = (float)(wave_y[bx_int + 1] - wave_y[bx_int - 1]) * 0.5f;

        // Audio impact kick energy
        int sample_idx = bx_int * (int)(n - 1) / (SCREEN_W - 1);
        int32_t local_amp = left[sample_idx] < 0 ? -left[sample_idx] : left[sample_idx];
        float kick = ((float)local_amp / (float)pk) * 4.2f;

        // Upward bounce impulse
        s_ball.vy = -(fabsf(s_ball.vy) * 0.80f + 2.2f + kick);
        if (s_ball.vy < -7.0f) s_ball.vy = -7.0f; // Max jump limit

        // Horizontal velocity shift by wave surface slope
        s_ball.vx += slope * 0.85f;
        if (s_ball.vx > 2.8f)  s_ball.vx = 2.8f;
        if (s_ball.vx < -2.8f) s_ball.vx = -2.8f;

        s_ball.spin_v = -slope * 0.15f + s_ball.vx * 0.05f;

        // Spawn bounce ripple ring
        for (int i = 0; i < MAX_RIPPLES; i++) {
            if (s_ripples[i].life <= 0) {
                s_ripples[i].x = bx_int;
                s_ripples[i].y = floor_y;
                s_ripples[i].r = 2;
                s_ripples[i].life = 6;
                break;
            }
        }
    }

    // 4. Draw Bounce Ripples
    for (int i = 0; i < MAX_RIPPLES; i++) {
        if (s_ripples[i].life > 0) {
            SafeDraw::drawCircle(s_ripples[i].x, s_ripples[i].y, s_ripples[i].r);
            s_ripples[i].r += 2;
            s_ripples[i].life--;
        }
    }

    // 5. Draw 3D Shaded Rotating Ball
    int bx = (int)(s_ball.x + 0.5f);
    int by = (int)(s_ball.y + 0.5f);

    // Ball solid body & border
    SafeDraw::drawDisc(bx, by, BALL_R);
    SafeDraw::drawCircle(bx, by, BALL_R);

    // Ball rotating seams / pattern
    SafeDraw::setDrawColor(0);
    float ca = cosf(s_ball.spin);
    float sa = sinf(s_ball.spin);
    int sx1 = bx - (int)(ca * (BALL_R - 1) + 0.5f);
    int sy1 = by - (int)(sa * (BALL_R - 1) + 0.5f);
    int sx2 = bx + (int)(ca * (BALL_R - 1) + 0.5f);
    int sy2 = by + (int)(sa * (BALL_R - 1) + 0.5f);
    SafeDraw::drawLine(sx1, sy1, sx2, sy2);
    // Specular highlight dot
    SafeDraw::drawPixel(bx - 1, by - 1);
    SafeDraw::setDrawColor(1);

    // 6. HUD Telemetry
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 7, "JUGGLE");

    char h_str[10];
    int height_val = 58 - by;
    if (height_val < 0) height_val = 0;
    snprintf(h_str, sizeof(h_str), "ALT:%02d", height_val);
    SafeDraw::drawStr(94, 7, h_str);

    // SafeDraw::drawStr(2, 22, "MVT-WAVE");
}
