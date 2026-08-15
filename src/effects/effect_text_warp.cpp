#include "effects.h"

// -----------------------------------------------------------------------
// MODE 40 — TEXT WARP (3D Space Hyperspace Text Flying Tunnel: LOVE MAC TAN)
// -----------------------------------------------------------------------

#define NUM_WARP_STARS 32
#define MAX_WARP_ITEMS 4

struct WarpStar {
    float x, y, z;
};

struct WarpTextItem {
    const char *text;
    float x, y, z;
    bool active;
};

static const char *WARP_VOCAB[] = {
    "L", "O", "V", "E", "M", "A", "C", "T", "A", "N",
    "LOVE", "MAC TAN", "<3"
};
#define VOCAB_SIZE (sizeof(WARP_VOCAB) / sizeof(WARP_VOCAB[0]))

static WarpStar s_stars[NUM_WARP_STARS];
static WarpTextItem s_text_items[MAX_WARP_ITEMS];
static int s_vocab_idx = 0;
static uint32_t s_last_spawn_time = 0;
static float s_warp_vol = 0.0f;

void effect_text_warp_on_enter() {
    s_vocab_idx = 0;
    s_last_spawn_time = millis();
    s_warp_vol = 0.0f;

    // Init starfield
    for (int i = 0; i < NUM_WARP_STARS; i++) {
        s_stars[i].x = (float)random(-60, 60);
        s_stars[i].y = (float)random(-30, 30);
        s_stars[i].z = (float)random(10, 100);
    }

    // Init text items
    for (int i = 0; i < MAX_WARP_ITEMS; i++) {
        s_text_items[i].active = false;
        s_text_items[i].z = 100.0f;
    }
}

void effect_text_warp_on_exit() {}

void effect_text_warp_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 63, cy = 31;

    // 1. Audio volume measurement
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += abs(left[i]) + abs(right[i]);
    }
    float vol = (float)sum / (float)(n * (s_peak_l + s_peak_r + 1));
    s_warp_vol = s_warp_vol * 0.7f + vol * 0.3f;

    float speed = 1.6f + s_warp_vol * 4.5f;

    // 2. Starfield Warp Simulation
    for (int i = 0; i < NUM_WARP_STARS; i++) {
        s_stars[i].z -= speed * 1.2f;
        if (s_stars[i].z <= 2.0f) {
            s_stars[i].x = (float)random(-70, 70);
            s_stars[i].y = (float)random(-35, 35);
            s_stars[i].z = 100.0f;
        }

        // Perspective 3D -> 2D
        float inv_z = 32.0f / s_stars[i].z;
        int sx = cx + (int)(s_stars[i].x * inv_z);
        int sy = cy + (int)(s_stars[i].y * inv_z);

        // Draw warp speed streak if bass is high
        if (s_warp_vol > 0.4f) {
            float prev_inv_z = 32.0f / (s_stars[i].z + speed * 1.8f);
            int psx = cx + (int)(s_stars[i].x * prev_inv_z);
            int psy = cy + (int)(s_stars[i].y * prev_inv_z);
            SafeDraw::drawLine(psx, psy, sx, sy);
        } else {
            SafeDraw::drawPixel(sx, sy);
        }
    }

    // 3. Spawn Text Items
    uint32_t now = millis();
    uint32_t spawn_interval = (s_warp_vol > 0.6f) ? 350 : 700;
    if (now - s_last_spawn_time >= spawn_interval) {
        s_last_spawn_time = now;
        for (int i = 0; i < MAX_WARP_ITEMS; i++) {
            if (!s_text_items[i].active) {
                s_text_items[i].active = true;
                s_text_items[i].text = WARP_VOCAB[s_vocab_idx];
                s_vocab_idx = (s_vocab_idx + 1) % VOCAB_SIZE;
                // Slight random offset from center for dynamic floating
                s_text_items[i].x = (float)random(-8, 8);
                s_text_items[i].y = (float)random(-4, 4);
                s_text_items[i].z = 110.0f;
                break;
            }
        }
    }

    // 4. Update and Render 3D Text Items
    for (int i = 0; i < MAX_WARP_ITEMS; i++) {
        if (!s_text_items[i].active) continue;

        s_text_items[i].z -= speed;

        if (s_text_items[i].z <= 4.0f) {
            s_text_items[i].active = false;
            continue;
        }

        float z = s_text_items[i].z;
        float inv_z = 40.0f / z;

        int sx = cx + (int)(s_text_items[i].x * inv_z);
        int sy = cy + (int)(s_text_items[i].y * inv_z);

        const char *txt = s_text_items[i].text;

        // Choose font size based on 3D distance
        if (z > 65.0f) {
            SafeDraw::setFont(u8g2_font_4x6_tr);
            int w = SafeDraw::getStrWidth(txt);
            SafeDraw::drawStr(sx - w / 2, sy + 3, txt);
        } else if (z > 30.0f) {
            SafeDraw::setFont(u8g2_font_6x10_tf);
            int w = SafeDraw::getStrWidth(txt);
            SafeDraw::drawStr(sx - w / 2, sy + 4, txt);
        } else {
            // Close 3D distance -> Large Bold Font with Box outline
            SafeDraw::setFont(u8g2_font_helvB12_tr);
            int w = SafeDraw::getStrWidth(txt);
            int h = 12;
            int tx = sx - w / 2;
            int ty = sy + h / 2;

            if (tx > -20 && tx < 130 && ty > -10 && ty < 80) {
                // Glow frame around text
                SafeDraw::drawFrame(tx - 3, ty - h - 1, w + 6, h + 4);
                SafeDraw::drawStr(tx, ty, txt);
            }
        }
    }

    // 5. Corner Cyber HUD
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 6, "3D WARP");
    SafeDraw::drawStr(100, 6, "LOVE-MVT");
}
