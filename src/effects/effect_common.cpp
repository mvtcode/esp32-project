#include "effect_common.h"

void draw_spinning_mvt(int cx, int cy, float angle) {
    float ca = cosf(angle);
    float sa = sinf(angle);

    // Vector line segments for 'M', 'V', 'T' relative to (0,0)
    // Coords designed to be centered and fit within R=8 px
    static const int8_t STROKES[][4] = {
        // 'M'
        {-7, -3, -7,  3},
        {-7, -3, -5,  0},
        {-5,  0, -3, -3},
        {-3, -3, -3,  3},
        // 'V'
        {-2, -3,  0,  3},
        { 0,  3,  2, -3},
        // 'T'
        { 3, -3,  7, -3},
        { 5, -3,  5,  3}
    };
    const int NUM_STROKES = 8;

    for (int i = 0; i < NUM_STROKES; i++) {
        float x1 = STROKES[i][0];
        float y1 = STROKES[i][1];
        float x2 = STROKES[i][2];
        float y2 = STROKES[i][3];

        // 2D Clockwise rotation: x' = x*cos - y*sin, y' = x*sin + y*cos
        int rx1 = cx + (int)(x1 * ca - y1 * sa + (x1 * ca - y1 * sa >= 0 ? 0.5f : -0.5f));
        int ry1 = cy + (int)(x1 * sa + y1 * ca + (x1 * sa + y1 * ca >= 0 ? 0.5f : -0.5f));
        int rx2 = cx + (int)(x2 * ca - y2 * sa + (x2 * ca - y2 * sa >= 0 ? 0.5f : -0.5f));
        int ry2 = cy + (int)(x2 * sa + y2 * ca + (x2 * sa + y2 * ca >= 0 ? 0.5f : -0.5f));

        SafeDraw::drawLine(rx1, ry1, rx2, ry2);
    }
}
