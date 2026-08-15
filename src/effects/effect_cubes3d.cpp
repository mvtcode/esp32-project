#include "effects.h"

// -----------------------------------------------------------------------
// MODE 46 — 3D NESTED & EXPLODING CUBES (Multi-axis Wireframe Matrix)
// -----------------------------------------------------------------------

static float s_cube_rx = 0.0f;
static float s_cube_ry = 0.0f;
static float s_cube_rz = 0.0f;
static float s_cube_vol = 0.0f;
static float s_explode_offset = 0.0f;

// 8 vertices of unit cube (+-1, +-1, +-1)
static const float CUBE_VERTS[8][3] = {
    {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1},
    {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1}
};

// 12 edges connecting the 8 vertices
static const uint8_t CUBE_EDGES[12][2] = {
    {0,1}, {1,2}, {2,3}, {3,0}, // Bottom face
    {4,5}, {5,6}, {6,7}, {7,4}, // Top face
    {0,4}, {1,5}, {2,6}, {3,7}  // Vertical pillars
};

void effect_cubes3d_on_enter() {
    s_cube_rx = 0.0f;
    s_cube_ry = 0.0f;
    s_cube_rz = 0.0f;
    s_cube_vol = 0.0f;
    s_explode_offset = 0.0f;
}

void effect_cubes3d_on_exit() {}

static void render_wireframe_cube(int cx, int cy, float size, float rx, float ry, float rz, float explode, float fov, float cam_dist) {
    float cx_rot = cosf(rx), sx_rot = sinf(rx);
    float cy_rot = cosf(ry), sy_rot = sinf(ry);
    float cz_rot = cosf(rz), sz_rot = sinf(rz);

    int proj_x[8];
    int proj_y[8];

    for (int i = 0; i < 8; i++) {
        // Vertex position + outward explosion offset
        float vx = CUBE_VERTS[i][0] * (size + explode);
        float vy = CUBE_VERTS[i][1] * (size + explode);
        float vz = CUBE_VERTS[i][2] * (size + explode);

        // 3D Rotations (X -> Y -> Z)
        // Rot X
        float y1 = vy * cx_rot - vz * sx_rot;
        float z1 = vy * sx_rot + vz * cx_rot;
        float x1 = vx;

        // Rot Y
        float x2 = x1 * cy_rot + z1 * sy_rot;
        float z2 = -x1 * sy_rot + z1 * cy_rot;
        float y2 = y1;

        // Rot Z
        float x3 = x2 * cz_rot - y2 * sz_rot;
        float y3 = x2 * sz_rot + y2 * cz_rot;
        float z3 = z2;

        // Perspective Projection
        float z_cam = cam_dist - z3;
        if (z_cam < 1.0f) z_cam = 1.0f;

        proj_x[i] = cx + (int)((x3 * fov) / z_cam);
        proj_y[i] = cy - (int)((y3 * fov) / z_cam);
    }

    // Draw 12 edges
    for (int e = 0; e < 12; e++) {
        int v0 = CUBE_EDGES[e][0];
        int v1 = CUBE_EDGES[e][1];
        SafeDraw::drawLine(proj_x[v0], proj_y[v0], proj_x[v1], proj_y[v1]);
    }
}

void effect_cubes3d_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = SCREEN_W / 2;
    const int cy = SCREEN_H / 2;

    // 1. Audio volume measurement
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += abs(left[i]) + abs(right[i]);
    }
    float vol = (float)sum / (float)(n * (s_peak_l + s_peak_r + 1));
    if (vol > 1.0f) vol = 1.0f;
    s_cube_vol = s_cube_vol * 0.7f + vol * 0.3f;

    // Beat detection for explosion trigger
    if (vol > 0.45f && s_explode_offset < 1.0f) {
        s_explode_offset = 7.0f * vol;
    } else {
        s_explode_offset *= 0.85f; // Fast elastic decay
    }

    // Continuous 3D rotation
    s_cube_rx += 0.022f + s_cube_vol * 0.05f;
    s_cube_ry += 0.038f + s_cube_vol * 0.07f;
    s_cube_rz += 0.015f + s_cube_vol * 0.03f;

    float cam_dist = 65.0f;
    float fov = 65.0f;

    // Outer Cube (large, forward rotation)
    float outer_size = 18.0f + s_cube_vol * 4.0f;
    render_wireframe_cube(cx, cy, outer_size, s_cube_rx, s_cube_ry, s_cube_rz, s_explode_offset, fov, cam_dist);

    // Inner Cube (small, counter-rotating at faster rate)
    float inner_size = 9.0f + s_cube_vol * 3.0f;
    render_wireframe_cube(cx, cy, inner_size, -s_cube_ry * 1.4f, -s_cube_rx * 1.4f, s_cube_rz * 1.2f, -s_explode_offset * 0.5f, fov, cam_dist);

    // Core pulsing energy point
    if (s_cube_vol > 0.3f) {
        int r = (int)(s_cube_vol * 3.0f);
        SafeDraw::drawDisc(cx, cy, r);
    }
}
