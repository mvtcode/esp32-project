#include "effects.h"

// -----------------------------------------------------------------------
// MODE 38 — TESSERACT (4D Hypercube Wireframe Rotating Projection)
// -----------------------------------------------------------------------

// 16 vertices of 4D hypercube (+-1, +-1, +-1, +-1)
static const float TESSERACT_VERTS[16][4] = {
    {-1,-1,-1,-1}, { 1,-1,-1,-1}, {-1, 1,-1,-1}, { 1, 1,-1,-1},
    {-1,-1, 1,-1}, { 1,-1, 1,-1}, {-1, 1, 1,-1}, { 1, 1, 1,-1},
    {-1,-1,-1, 1}, { 1,-1,-1, 1}, {-1, 1,-1, 1}, { 1, 1,-1, 1},
    {-1,-1, 1, 1}, { 1,-1, 1, 1}, {-1, 1, 1, 1}, { 1, 1, 1, 1}
};

static float s_tess_ang_xy = 0.0f;
static float s_tess_ang_zw = 0.0f;
static float s_tess_ang_xw = 0.0f;
static float s_tess_vol = 0.0f;

void effect_tesseract_on_enter() {
    s_tess_ang_xy = 0.0f;
    s_tess_ang_zw = 0.0f;
    s_tess_ang_xw = 0.0f;
    s_tess_vol = 0.0f;
}

void effect_tesseract_on_exit() {}

void effect_tesseract_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 63, cy = 31;

    // 1. Audio volume measurement
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += abs(left[i]) + abs(right[i]);
    }
    float vol = (float)sum / (float)(n * (s_peak_l + s_peak_r + 1));
    s_tess_vol = s_tess_vol * 0.7f + vol * 0.3f;

    // 4D Rotation speeds driven by audio
    s_tess_ang_xy += 0.02f + s_tess_vol * 0.04f;
    s_tess_ang_zw += 0.03f + s_tess_vol * 0.05f;
    s_tess_ang_xw += 0.015f + s_tess_vol * 0.03f;

    float cos_xy = cosf(s_tess_ang_xy), sin_xy = sinf(s_tess_ang_xy);
    float cos_zw = cosf(s_tess_ang_zw), sin_zw = sinf(s_tess_ang_zw);
    float cos_xw = cosf(s_tess_ang_xw), sin_xw = sinf(s_tess_ang_xw);

    // 2. Transform 16 vertices from 4D -> 3D -> 2D
    static int proj_x[16]; // static: avoid per-frame stack allocation
    static int proj_y[16]; // static: avoid per-frame stack allocation

    float dist_4d = 2.4f - s_tess_vol * 0.5f; // Audio pulse pulls 4D depth
    float scale = 65.0f + s_tess_vol * 20.0f;

    for (int i = 0; i < 16; i++) {
        float x = TESSERACT_VERTS[i][0];
        float y = TESSERACT_VERTS[i][1];
        float z = TESSERACT_VERTS[i][2];
        float w = TESSERACT_VERTS[i][3];

        // 4D Rotation: XY plane
        float x1 = x * cos_xy - y * sin_xy;
        float y1 = x * sin_xy + y * cos_xy;
        float z1 = z;
        float w1 = w;

        // 4D Rotation: ZW plane
        float z2 = z1 * cos_zw - w1 * sin_zw;
        float w2 = z1 * sin_zw + w1 * cos_zw;

        // 4D Rotation: XW plane
        float x3 = x1 * cos_xw - w2 * sin_xw;
        float w3 = x1 * sin_xw + w2 * cos_xw;
        float y3 = y1;
        float z3 = z2;

        // 4D -> 3D Perspective Projection
        float inv_w = 1.0f / (dist_4d - w3);
        float x3d = x3 * inv_w;
        float y3d = y3 * inv_w;
        float z3d = z3 * inv_w;

        // 3D -> 2D Perspective Projection
        float dist_3d = 3.0f;
        float inv_z = 1.0f / (dist_3d - z3d);

        int sx = cx + (int)(x3d * scale * inv_z);
        int sy = cy - (int)(y3d * scale * inv_z);

        proj_x[i] = sx;
        proj_y[i] = sy;
    }

    // 3. Render 32 Edges (connect vertices that differ by exactly 1 bit)
    for (int i = 0; i < 16; i++) {
        for (int b = 0; b < 4; b++) {
            int j = i ^ (1 << b);
            if (i < j) { // Draw each unique edge once
                int x1 = proj_x[i], y1 = proj_y[i];
                int x2 = proj_x[j], y2 = proj_y[j];
                SafeDraw::drawLine(x1, y1, x2, y2);
            }
        }
    }

    // 4. Draw vertex node dots
    for (int i = 0; i < 16; i++) {
        SafeDraw::drawDisc(proj_x[i], proj_y[i], 1);
    }

    // 5. HUD
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 6, "4D HYPER");
    SafeDraw::drawStr(104, 6, "TESSERACT");
}
