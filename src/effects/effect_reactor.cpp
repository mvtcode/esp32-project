#include "effects.h"

// -----------------------------------------------------------------------
// MODE 23 — MVT REACTOR (Arc Reactor Core & High-Voltage Electric Discharge)
// -----------------------------------------------------------------------
static float s_rot_inner = 0.0f;
static float s_rot_outer = 0.0f;

void effect_reactor_on_enter() {
    s_rot_inner = 0.0f;
    s_rot_outer = 0.0f;
}

void effect_reactor_on_exit() {}

void effect_reactor_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 64, cy = 31;

    // 1. Audio amplitude analysis
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        int32_t smp = (left[i] + right[i]) / 2;
        sum += (smp < 0 ? -smp : smp);
    }
    float norm = (float)sum / ((float)n * (float)s_peak_l);
    if (norm > 1.0f) norm = 1.0f;

    // Speed up rotation based on power
    s_rot_inner += 0.04f + norm * 0.12f;
    s_rot_outer -= 0.02f + norm * 0.08f;
    if (s_rot_inner > 6.28318f) s_rot_inner -= 6.28318f;
    if (s_rot_outer < 0.0f) s_rot_outer += 6.28318f;

    // 2. Central Glowing Core (pulsing disc)
    int core_r = 5 + (int)(norm * 6.0f);
    SafeDraw::drawDisc(cx, cy, core_r);
    SafeDraw::drawCircle(cx, cy, core_r + 2);

    // 3. Inner Rotating Energy Nodes (6 nodes)
    for (int i = 0; i < 6; i++) {
        float a = s_rot_inner + i * (6.28318f / 6.0f);
        int nx = cx + (int)(cosf(a) * 16.0f);
        int ny = cy + (int)(sinf(a) * 16.0f);
        SafeDraw::drawDisc(nx, ny, 2);
        SafeDraw::drawLine(cx, cy, nx, ny);
    }
    SafeDraw::drawCircle(cx, cy, 16);

    // 4. Outer Counter-Rotating Segmented Gear Ring (10 segments)
    SafeDraw::drawCircle(cx, cy, 25);
    SafeDraw::drawCircle(cx, cy, 29);
    for (int i = 0; i < 10; i++) {
        float a = s_rot_outer + i * (6.28318f / 10.0f);
        int x1 = cx + (int)(cosf(a) * 24.0f);
        int y1 = cy + (int)(sinf(a) * 24.0f);
        int x2 = cx + (int)(cosf(a) * 30.0f);
        int y2 = cy + (int)(sinf(a) * 30.0f);
        SafeDraw::drawLine(x1, y1, x2, y2);
    }

    // 5. High-Voltage Lightning Discharges on Overcharge (norm > 0.45)
    if (norm > 0.45f) {
        // Jagged electric arcs from reactor edge to borders
        int mid_x1 = 30 + (rand() % 10);
        int mid_y1 = 15 + (rand() % 8);
        SafeDraw::drawLine(cx - 28, cy - 8, mid_x1, mid_y1);
        SafeDraw::drawLine(mid_x1, mid_y1, 2, 4);

        int mid_x2 = 98 + (rand() % 10);
        int mid_y2 = 45 + (rand() % 8);
        SafeDraw::drawLine(cx + 28, cy + 8, mid_x2, mid_y2);
        SafeDraw::drawLine(mid_x2, mid_y2, 125, 58);
    }

    // 6. Cyberpunk reactor telemetry
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 8, "ARC-CORE");
    SafeDraw::drawStr(2, 60, "PWR: 100%");
    SafeDraw::drawStr(100, 8, "MVT-MK1");
    int bar = (int)(norm * 24.0f);
    SafeDraw::drawFrame(100, 54, 26, 6);
    if (bar > 0) SafeDraw::drawBox(101, 55, bar > 24 ? 24 : bar, 4);
}
