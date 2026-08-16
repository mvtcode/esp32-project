#include "effects.h"

// -----------------------------------------------------------------------
// MODE 48 — 3D FLOATING CRYSTAL (Icosahedron Wireframe with Energy Rays)
// -----------------------------------------------------------------------

static float s_crys_rx = 0.0f;
static float s_crys_ry = 0.0f;
static float s_crys_rz = 0.0f;
static float s_crys_vol = 0.0f;
static float s_ray_length = 0.0f;

// 12 Vertices of regular Icosahedron (golden ratio phi = 1.618034)
#define PHI 1.618034f
static const float ICO_VERTS[12][3] = {
    {-1.0f,  PHI, 0.0f}, { 1.0f,  PHI, 0.0f}, {-1.0f, -PHI, 0.0f}, { 1.0f, -PHI, 0.0f},
    { 0.0f, -1.0f,  PHI}, { 0.0f,  1.0f,  PHI}, { 0.0f, -1.0f, -PHI}, { 0.0f,  1.0f, -PHI},
    {  PHI, 0.0f, -1.0f}, {  PHI, 0.0f,  1.0f}, { -PHI, 0.0f, -1.0f}, { -PHI, 0.0f,  1.0f}
};

// 30 Edges of regular Icosahedron
static const uint8_t ICO_EDGES[30][2] = {
    {0,1}, {0,5}, {0,7}, {0,10}, {0,11},
    {1,5}, {1,7}, {1,8}, {1,9},
    {2,3}, {2,4}, {2,6}, {2,10}, {2,11},
    {3,4}, {3,6}, {3,8}, {3,9},
    {4,5}, {4,9}, {4,11},
    {5,9}, {5,11},
    {6,7}, {6,8}, {6,10},
    {7,8}, {7,10},
    {8,9}, {10,11}
};

void effect_crystal3d_on_enter() {
    s_crys_rx = 0.0f;
    s_crys_ry = 0.0f;
    s_crys_rz = 0.0f;
    s_crys_vol = 0.0f;
    s_ray_length = 0.0f;
}

void effect_crystal3d_on_exit() {}

void effect_crystal3d_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = SCREEN_W / 2;
    const int cy = SCREEN_H / 2;

    // 1. Audio volume measurement
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += abs(left[i]) + abs(right[i]);
    }
    float vol = (float)sum / (float)(n * (s_peak_l + s_peak_r + 1));
    if (vol > 1.0f) vol = 1.0f;
    s_crys_vol = s_crys_vol * 0.7f + vol * 0.3f;

    // Energy ray flare trigger
    if (vol > 0.4f && s_ray_length < 2.0f) {
        s_ray_length = 12.0f * vol;
    } else {
        s_ray_length *= 0.8f;
    }

    // Continuous 3D rotation
    s_crys_rx += 0.024f + s_crys_vol * 0.05f;
    s_crys_ry += 0.032f + s_crys_vol * 0.06f;
    s_crys_rz += 0.018f + s_crys_vol * 0.03f;

    float cx_rot = cosf(s_crys_rx), sx_rot = sinf(s_crys_rx);
    float cy_rot = cosf(s_crys_ry), sy_rot = sinf(s_crys_ry);
    float cz_rot = cosf(s_crys_rz), sz_rot = sinf(s_crys_rz);

    float scale = 11.5f + s_crys_vol * 4.0f;
    float cam_dist = 60.0f;
    float fov = 65.0f;

    static int proj_x[12]; // static: avoid per-frame stack allocation
    static int proj_y[12]; // static: avoid per-frame stack allocation
    static int ray_x[12];  // static: avoid per-frame stack allocation
    static int ray_y[12];  // static: avoid per-frame stack allocation

    for (int i = 0; i < 12; i++) {
        float vx = ICO_VERTS[i][0] * scale;
        float vy = ICO_VERTS[i][1] * scale;
        float vz = ICO_VERTS[i][2] * scale;

        // 3D Rotations (X -> Y -> Z)
        float y1 = vy * cx_rot - vz * sx_rot;
        float z1 = vy * sx_rot + vz * cx_rot;
        float x1 = vx;

        float x2 = x1 * cy_rot + z1 * sy_rot;
        float z2 = -x1 * sy_rot + z1 * cy_rot;
        float y2 = y1;

        float x3 = x2 * cz_rot - y2 * sz_rot;
        float y3 = x2 * sz_rot + y2 * cz_rot;
        float z3 = z2;

        // Perspective Projection
        float z_cam = cam_dist - z3;
        if (z_cam < 1.0f) z_cam = 1.0f;

        proj_x[i] = cx + (int)((x3 * fov) / z_cam);
        proj_y[i] = cy - (int)((y3 * fov) / z_cam);

        // Ray tip projection
        if (s_ray_length > 0.5f) {
            float ray_scale = (scale + s_ray_length) / scale;
            float rx3 = x3 * ray_scale;
            float ry3 = y3 * ray_scale;
            float rz3 = z3 * ray_scale;
            float rz_cam = cam_dist - rz3;
            if (rz_cam < 1.0f) rz_cam = 1.0f;

            ray_x[i] = cx + (int)((rx3 * fov) / rz_cam);
            ray_y[i] = cy - (int)((ry3 * fov) / rz_cam);
        }
    }

    // 2. Draw 30 Wireframe Edges
    for (int e = 0; e < 30; e++) {
        int v0 = ICO_EDGES[e][0];
        int v1 = ICO_EDGES[e][1];
        SafeDraw::drawLine(proj_x[v0], proj_y[v0], proj_x[v1], proj_y[v1]);
    }

    // 3. Draw Vertex energy nodes & burst rays
    for (int i = 0; i < 12; i++) {
        SafeDraw::drawPixel(proj_x[i], proj_y[i]);
        if (s_ray_length > 1.0f) {
            SafeDraw::drawLine(proj_x[i], proj_y[i], ray_x[i], ray_y[i]);
        }
    }

    // Inner glowing core
    if (s_crys_vol > 0.35f) {
        int core_r = (int)(s_crys_vol * 3.5f);
        SafeDraw::drawDisc(cx, cy, core_r);
    }
}
