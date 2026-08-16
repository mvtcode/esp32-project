#include "effects.h"

// -----------------------------------------------------------------------
// MODE 28 — MVT BOUNCE LINES (Audio-Reactive Free-Flying Bouncing Lines)
// -----------------------------------------------------------------------
struct BounceLine {
    float x, y;
    float vx, vy;
    float length;
    float angle;
    float v_angle;
};

struct WallSpark {
    int x, y;
    int life; // frames remaining
};

struct LineTrail {
    int p1x, p1y;
    int p2x, p2y;
};

static const int NUM_LINES = 3;
static const int MAX_SPARKS = 10;
static const int TRAIL_LEN = 3;

static BounceLine s_lines[NUM_LINES];
static LineTrail  s_trails[NUM_LINES][TRAIL_LEN];
static WallSpark  s_sparks[MAX_SPARKS];
static bool       s_initialized = false;

static void init_bounce_lines() {
    // Distinct diagonal launch angles for the 3 lines (e.g., ~35°, ~145°, ~310°)
    float initial_angles[NUM_LINES] = { 0.65f, 2.45f, 4.25f };

    for (int i = 0; i < NUM_LINES; i++) {
        s_lines[i].x = (float)(30 + (i * 34));
        s_lines[i].y = (float)(20 + (i * 12));
        
        float dir_angle = initial_angles[i] + (((float)(rand() % 40) - 20.0f) * 0.01745f);
        float base_spd  = 0.9f + ((float)(rand() % 100) / 100.0f) * 0.4f;

        s_lines[i].vx = cosf(dir_angle) * base_spd;
        s_lines[i].vy = sinf(dir_angle) * base_spd;

        // Ensure minimum speed on both axes to avoid 1D vertical/horizontal lock
        if (fabsf(s_lines[i].vx) < 0.45f) s_lines[i].vx = (s_lines[i].vx < 0 ? -0.55f : 0.55f);
        if (fabsf(s_lines[i].vy) < 0.45f) s_lines[i].vy = (s_lines[i].vy < 0 ? -0.55f : 0.55f);

        // Length between 11 and 15 px
        s_lines[i].length = 11.0f + (float)(rand() % 5);
        s_lines[i].angle  = (float)(rand() % 360) * 0.0174532925f;
        // Continuous spinning velocity
        float spin_dir = (i % 2 == 0) ? 1.0f : -1.0f;
        s_lines[i].v_angle = spin_dir * (0.06f + ((float)(rand() % 50) / 1000.0f));

        for (int t = 0; t < TRAIL_LEN; t++) {
            s_trails[i][t] = { -1, -1, -1, -1 };
        }
    }

    for (int i = 0; i < MAX_SPARKS; i++) {
        s_sparks[i].life = 0;
    }
    s_initialized = true;
}

static void add_spark(int x, int y) {
    for (int i = 0; i < MAX_SPARKS; i++) {
        if (s_sparks[i].life <= 0) {
            s_sparks[i].x = x;
            s_sparks[i].y = y;
            s_sparks[i].life = 4; // lives for 4 frames
            break;
        }
    }
}

void effect_bounce_lines_on_enter() {
    init_bounce_lines();
}

void effect_bounce_lines_on_exit() {}

void effect_bounce_lines_render(const int32_t *left, const int32_t *right, size_t n) {
    if (!s_initialized) {
        init_bounce_lines();
    }

    // 1. Audio Intensity & FFT Bass Analysis — use pre-computed frame bands
    const float bass = g_frame_bands.bass;

    int32_t max_peak = s_peak_l > s_peak_r ? s_peak_l : s_peak_r;
    if (max_peak < 1) max_peak = 1;

    int32_t cur_amp = 0;
    for (size_t i = 0; i < n; i++) {
        int32_t vl = left[i] < 0 ? -left[i] : left[i];
        int32_t vr = right[i] < 0 ? -right[i] : right[i];
        if (vl > cur_amp) cur_amp = vl;
        if (vr > cur_amp) cur_amp = vr;
    }
    float vol = (float)cur_amp / (float)max_peak;
    if (vol > 1.0f) vol = 1.0f;

    // Music intensity multiplier: base 1.0x, rushes up to 4.5x on loud beats
    float speed_mult = 0.8f + vol * 2.2f + bass * 2.5f;
    if (speed_mult > 5.0f) speed_mult = 5.0f;

    // 2. Update and Draw each Bouncing Line
    for (int i = 0; i < NUM_LINES; i++) {
        BounceLine &line = s_lines[i];

        // Move position and rotate
        line.x += line.vx * speed_mult;
        line.y += line.vy * speed_mult;
        line.angle += line.v_angle * speed_mult;

        float half_len = (line.length + bass * 3.0f) * 0.5f;
        float hx = fabsf(cosf(line.angle) * half_len);
        float hy = fabsf(sinf(line.angle) * half_len);

        // Bass kick adds subtle trajectory steering
        if (bass > 0.4f) {
            line.angle += (i % 2 == 0 ? 0.03f : -0.03f);
        }

        // Collision with Left & Right borders (X: 0..127)
        if (line.x - hx < 0.0f) {
            line.x = hx;
            line.vx = fabsf(line.vx); // bounce right
            line.vy += ((float)(rand() % 50) - 25.0f) * 0.01f; // deflect vy
            line.v_angle = -line.v_angle;
            add_spark(0, (int)line.y);
        } else if (line.x + hx > 127.0f) {
            line.x = 127.0f - hx;
            line.vx = -fabsf(line.vx); // bounce left
            line.vy += ((float)(rand() % 50) - 25.0f) * 0.01f; // deflect vy
            line.v_angle = -line.v_angle;
            add_spark(127, (int)line.y);
        }

        // Collision with Top & Bottom borders (Y: 0..63)
        if (line.y - hy < 0.0f) {
            line.y = hy;
            line.vy = fabsf(line.vy); // bounce down
            line.vx += ((float)(rand() % 50) - 25.0f) * 0.01f; // deflect vx
            line.v_angle = -line.v_angle;
            add_spark((int)line.x, 0);
        } else if (line.y + hy > 63.0f) {
            line.y = 63.0f - hy;
            line.vy = -fabsf(line.vy); // bounce up
            line.vx += ((float)(rand() % 50) - 25.0f) * 0.01f; // deflect vx
            line.v_angle = -line.v_angle;
            add_spark((int)line.x, 63);
        }

        // Ensure minimum 2D velocity components to prevent vertical/horizontal lock
        if (fabsf(line.vx) < 0.45f) line.vx = (line.vx < 0 ? -0.55f : 0.55f);
        if (fabsf(line.vy) < 0.45f) line.vy = (line.vy < 0 ? -0.55f : 0.55f);
        if (fabsf(line.vx) > 1.80f) line.vx = (line.vx < 0 ? -1.80f : 1.80f);
        if (fabsf(line.vy) > 1.80f) line.vy = (line.vy < 0 ? -1.80f : 1.80f);
        if (fabsf(line.v_angle) < 0.04f) line.v_angle = (line.v_angle < 0 ? -0.06f : 0.06f);

        // Calculate line endpoints
        float ca = cosf(line.angle);
        float sa = sinf(line.angle);
        int p1x = (int)(line.x - ca * half_len + 0.5f);
        int p1y = (int)(line.y - sa * half_len + 0.5f);
        int p2x = (int)(line.x + ca * half_len + 0.5f);
        int p2y = (int)(line.y + sa * half_len + 0.5f);

        // Clamp to screen
        if (p1x < 0) p1x = 0; if (p1x >= SCREEN_W) p1x = SCREEN_W - 1;
        if (p1y < 0) p1y = 0; if (p1y >= SCREEN_H) p1y = SCREEN_H - 1;
        if (p2x < 0) p2x = 0; if (p2x >= SCREEN_W) p2x = SCREEN_W - 1;
        if (p2y < 0) p2y = 0; if (p2y >= SCREEN_H) p2y = SCREEN_H - 1;

        // Render motion ghost trails
        for (int t = 0; t < TRAIL_LEN; t++) {
            if (s_trails[i][t].p1x >= 0) {
                // Draw dotted trail points
                int mid_x = (s_trails[i][t].p1x + s_trails[i][t].p2x) / 2;
                int mid_y = (s_trails[i][t].p1y + s_trails[i][t].p2y) / 2;
                SafeDraw::drawPixel(mid_x, mid_y);
                if (t == 0 && speed_mult > 1.8f) {
                    SafeDraw::drawPixel(s_trails[i][t].p1x, s_trails[i][t].p1y);
                    SafeDraw::drawPixel(s_trails[i][t].p2x, s_trails[i][t].p2y);
                }
            }
        }

        // Shift trail history
        for (int t = TRAIL_LEN - 1; t > 0; t--) {
            s_trails[i][t] = s_trails[i][t - 1];
        }
        s_trails[i][0] = { p1x, p1y, p2x, p2y };

        // Draw primary laser line
        SafeDraw::drawLine(p1x, p1y, p2x, p2y);

        // Highlight endpoints with glowing heads
        SafeDraw::drawPixel(p1x, p1y);
        SafeDraw::drawPixel(p2x, p2y);

        // Double laser beam / motion glow on high speed/bass
        if (speed_mult > 2.0f) {
            int off_x = (int)(-sa * 1.0f + 0.5f);
            int off_y = (int)(ca * 1.0f + 0.5f);
            SafeDraw::drawLine(p1x + off_x, p1y + off_y, p2x + off_x, p2y + off_y);
        }
    }

    // 3. Draw and update Wall Impact Sparks
    for (int i = 0; i < MAX_SPARKS; i++) {
        if (s_sparks[i].life > 0) {
            int sx = s_sparks[i].x;
            int sy = s_sparks[i].y;
            // Draw spark cross / blip
            SafeDraw::drawPixel(sx, sy);
            SafeDraw::drawPixel(sx - 1, sy);
            SafeDraw::drawPixel(sx + 1, sy);
            SafeDraw::drawPixel(sx, sy - 1);
            SafeDraw::drawPixel(sx, sy + 1);
            s_sparks[i].life--;
        }
    }

    // 4. Tech HUD Telemetry
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 7, "BOUNCE");

    char spd_str[10];
    snprintf(spd_str, sizeof(spd_str), "SPD:%0.1fX", speed_mult);
    SafeDraw::drawStr(88, 7, spd_str);

    SafeDraw::drawStr(2, 62, "MVT-FX");

    // Dynamic mini volume bar (bottom right)
    int bar_w = (int)(vol * 32.0f);
    if (bar_w > 32) bar_w = 32;
    SafeDraw::drawFrame(92, 57, 34, 5);
    if (bar_w > 0) {
        SafeDraw::drawBox(93, 58, bar_w, 3);
    }
}
