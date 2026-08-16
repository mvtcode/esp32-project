#include "effects.h"

// -----------------------------------------------------------------------
// MODE 51 — 3D AUDIO MVT TEXT (Solid 3D Block Wireframe MVT with Torus-Style Ribs)
// -----------------------------------------------------------------------

static float s_text3d_rot_x = 0.0f;
static float s_text3d_rot_y = 0.0f;
static float s_text3d_rot_z = 0.0f;
static float s_text3d_vol = 0.0f;

// -----------------------------------------------------------------------
// 2D Polygon Outlines for Solid Block Letters (M, V, T)
// -----------------------------------------------------------------------

// Letter 'M' — 12 Vertices Block Polygon
static const float M_VERTS[12][2] = {
    {-26.0f, -11.0f}, // 0: Bottom-left outer
    {-26.0f,  11.0f}, // 1: Top-left outer
    {-20.0f,  11.0f}, // 2: Top-left inner
    {-16.0f,   1.0f}, // 3: Center dip top
    {-12.0f,  11.0f}, // 4: Top-right inner
    { -6.0f,  11.0f}, // 5: Top-right outer
    { -6.0f, -11.0f}, // 6: Bottom-right outer
    {-11.0f, -11.0f}, // 7: Bottom-right inner
    {-11.0f,   3.0f}, // 8: Right leg inner
    {-16.0f,  -4.5f}, // 9: Center dip bottom
    {-21.0f,   3.0f}, // 10: Left leg inner
    {-21.0f, -11.0f}  // 11: Bottom-left inner
};

// Letter 'V' — 6 Vertices Block Polygon
static const float V_VERTS[6][2] = {
    {-5.0f,  11.0f}, // 0: Top-left outer
    { 0.0f, -11.0f}, // 1: Bottom tip outer
    { 5.0f,  11.0f}, // 2: Top-right outer
    { 2.0f,  11.0f}, // 3: Top-right inner
    { 0.0f,  -5.5f}, // 4: Bottom dip inner
    {-2.0f,  11.0f}  // 5: Top-left inner
};

// Letter 'T' — 8 Vertices Block Polygon
static const float T_VERTS[8][2] = {
    { 7.0f,  11.0f}, // 0: Top-bar left outer
    {25.0f,  11.0f}, // 1: Top-bar right outer
    {25.0f,   6.0f}, // 2: Top-bar right bottom
    {18.5f,   6.0f}, // 3: Stem right top
    {18.5f, -11.0f}, // 4: Stem bottom right
    {13.5f, -11.0f}, // 5: Stem bottom left
    {13.5f,   6.0f}, // 6: Stem left top
    { 7.0f,   6.0f}  // 7: Top-bar left bottom
};

void effect_text3d_on_enter() {
    s_text3d_rot_x = 0.0f;
    s_text3d_rot_y = 0.0f;
    s_text3d_rot_z = 0.0f;
    s_text3d_vol = 0.0f;
}

void effect_text3d_on_exit() {}

void effect_text3d_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = SCREEN_W / 2;
    const int cy = SCREEN_H / 2;

    // 1. Audio volume measurement
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += abs(left[i]) + abs(right[i]);
    }
    float vol = (float)sum / (float)(n * (s_peak_l + s_peak_r + 1));
    if (vol > 1.0f) vol = 1.0f;
    s_text3d_vol = s_text3d_vol * 0.7f + vol * 0.3f;

    // Continuous 3D rotation speed modulated by audio volume
    s_text3d_rot_x += 0.024f + s_text3d_vol * 0.045f;
    s_text3d_rot_y += 0.034f + s_text3d_vol * 0.055f;
    s_text3d_rot_z += 0.016f + s_text3d_vol * 0.025f;

    float cos_x = cosf(s_text3d_rot_x), sin_x = sinf(s_text3d_rot_x);
    float cos_y = cosf(s_text3d_rot_y), sin_y = sinf(s_text3d_rot_y);
    float cos_z = cosf(s_text3d_rot_z), sin_z = sinf(s_text3d_rot_z);

    // Dynamic 3D Dimensions (Enlarged & Thickened for Heavy 3D Block Feel)
    float scale = 1.15f + s_text3d_vol * 0.35f;
    float depth = 7.5f + s_text3d_vol * 5.5f; // Pronounced 3D depth extrusion
    float cam_dist = 60.0f;
    float fov = 60.0f;

    // 3D Projection Helper
    auto project_3d = [&](float x0, float y0, float z0, int &sx, int &sy) {
        // 1. Rotate X
        float y1 = y0 * cos_x - z0 * sin_x;
        float z1 = y0 * sin_x + z0 * cos_x;
        float x1 = x0;

        // 2. Rotate Y
        float x2 = x1 * cos_y + z1 * sin_y;
        float z2 = -x1 * sin_y + z1 * cos_y;
        float y2 = y1;

        // 3. Rotate Z
        float x3 = x2 * cos_z - y2 * sin_z;
        float y3 = x2 * sin_z + y2 * cos_z;
        float z3 = z2;

        // Perspective projection
        float z_cam = cam_dist - z3;
        if (z_cam < 1.0f) z_cam = 1.0f;

        sx = cx + (int)((x3 * fov) / z_cam);
        sy = cy - (int)((y3 * fov) / z_cam);
    };

    // Helper lambda to render a 3D block letter
    auto render_block_letter = [&](const float verts[][2], int num_verts, int sample_offset) {
        int fx[12], fy[12]; // Front face (+depth)
        int bx[12], by[12]; // Back face (-depth)
        int mx[12], my[12]; // Middle wireframe rib (Z=0, Torus-style)

        for (int i = 0; i < num_verts; i++) {
            float vx = verts[i][0] * scale;
            float vy = verts[i][1] * scale;

            // Audio waveform ripple deformation along vertices
            size_t s_idx = ((i + sample_offset) * 8) % (n > 0 ? n : 1);
            float wave_mod = 0.0f;
            if (s_peak_l > 0) {
                wave_mod = ((float)left[s_idx] / (float)s_peak_l) * (1.8f + s_text3d_vol * 2.5f);
            }

            project_3d(vx, vy + wave_mod,  depth, fx[i], fy[i]);
            project_3d(vx, vy - wave_mod, -depth, bx[i], by[i]);
            project_3d(vx, vy,             0.0f,  mx[i], my[i]);
        }

        // 1. Draw Front Face closed polygon
        for (int i = 0; i < num_verts; i++) {
            int next_i = (i + 1) % num_verts;
            SafeDraw::drawLine(fx[i], fy[i], fx[next_i], fy[next_i]);
        }

        // 2. Draw Back Face closed polygon
        for (int i = 0; i < num_verts; i++) {
            int next_i = (i + 1) % num_verts;
            SafeDraw::drawLine(bx[i], by[i], bx[next_i], by[next_i]);
        }

        // 3. Draw Depth Extrusion Struts (connecting Front to Back at each corner)
        for (int i = 0; i < num_verts; i++) {
            SafeDraw::drawLine(fx[i], fy[i], bx[i], by[i]);
        }

        // 4. Middle Torus-style Wireframe Ring Rib (Every other edge for clean density)
        if (s_text3d_vol > 0.2f) {
            for (int i = 0; i < num_verts; i += 2) {
                int next_i = (i + 1) % num_verts;
                SafeDraw::drawLine(mx[i], my[i], mx[next_i], my[next_i]);
            }
        }
    };

    // 2. Render 3D Block Letters 'M', 'V', 'T'
    render_block_letter(M_VERTS, 12, 0);
    render_block_letter(V_VERTS,  6, 4);
    render_block_letter(T_VERTS,  8, 8);

    // 3. Surrounding 3D Toroidal Orbit Ring (Torus-inspired cosmic halo)
    const int NUM_RING_PTS = 20;
    float ring_r = 33.0f + s_text3d_vol * 9.0f;
    int prev_rx = 0, prev_ry = 0;
    int first_rx = 0, first_ry = 0;

    for (int i = 0; i <= NUM_RING_PTS; i++) {
        float theta = (float)i * (2.0f * (float)M_PI / (float)NUM_RING_PTS);
        size_t s_idx = (i * 6) % (n > 0 ? n : 1);
        float rw_mod = 0.0f;
        if (s_peak_r > 0) {
            rw_mod = ((float)right[s_idx] / (float)s_peak_r) * (2.2f + s_text3d_vol * 3.5f);
        }

        float rx0 = (ring_r + rw_mod) * cosf(theta);
        float ry0 = rw_mod * 0.5f;
        float rz0 = (ring_r + rw_mod) * sinf(theta);

        int sx, sy;
        project_3d(rx0, ry0, rz0, sx, sy);

        if (i == 0) {
            first_rx = sx;
            first_ry = sy;
        } else {
            // Segmented glowing orbital particle halo
            if (i % 2 == 0) {
                SafeDraw::drawLine(prev_rx, prev_ry, sx, sy);
            }
        }
        prev_rx = sx;
        prev_ry = sy;
    }

    // 4. Center Glowing Bass Core
    if (s_text3d_vol > 0.40f) {
        int core_r = (int)(s_text3d_vol * 3.5f);
        SafeDraw::drawDisc(cx, cy, core_r);
    }
}
