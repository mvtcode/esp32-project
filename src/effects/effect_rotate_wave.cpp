#include "effects.h"

// -----------------------------------------------------------------------
// MODE 27 — MVT ROTATE WAVE (Rotating Stereo Waveform with 20s Period)
// -----------------------------------------------------------------------
static float s_rot_angle = 0.0f;

void effect_rotate_wave_on_enter() {
    s_rot_angle = 0.0f;
}

void effect_rotate_wave_on_exit() {}

void effect_rotate_wave_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 64;
    const int cy = 31;

    // 1. Calculate angle for exact 20-second per revolution rotation
    // 20,000 ms = 1 full 360-degree (2*PI) rotation
    uint32_t now = millis();
    s_rot_angle = fmodf((float)(now % 20000) / 20000.0f * 6.2831853f, 6.2831853f);

    float ca = cosf(s_rot_angle);
    float sa = sinf(s_rot_angle);

    // Normal vector perpendicular to the rotating diameter
    float nx = -sa;
    float ny = ca;

    // 2. Compute the 2 boundary points reaching the screen perimeter
    // Screen bounds: [0, 127] x [0, 63], Center = (64, 31)
    float rx = 62.0f / (fabsf(ca) + 1e-5f);
    float ry = 29.0f / (fabsf(sa) + 1e-5f);
    float r = (rx < ry ? rx : ry);

    int p1x = cx + (int)(ca * r + 0.5f);
    int p1y = cy + (int)(sa * r + 0.5f);
    int p2x = cx - (int)(ca * r + 0.5f);
    int p2y = cy - (int)(sa * r + 0.5f);

    // 3. Audio RMS / Volume for dynamic HUD & node response
    int32_t max_peak = s_peak_l > s_peak_r ? s_peak_l : s_peak_r;
    if (max_peak < 1) max_peak = 1;



    // 5. Render Stereo Waveforms (Left & Right) along the rotating axis
    const int SAMPLES_COUNT = 64;
    const float MAX_AMP = 14.0f;

    int prev_lx = 0, prev_ly = 0;
    int prev_rx = 0, prev_ry = 0;
    bool has_prev = false;

    for (int i = 0; i < SAMPLES_COUNT; i++) {
        float t = (float)i / (float)(SAMPLES_COUNT - 1);

        // Hann window envelope so waveform converges precisely at P2 (t=0) and P1 (t=1)
        float window = sinf(t * 3.14159265f);

        // Base coordinate along the diameter
        float bx = (float)p2x + t * (float)(p1x - p2x);
        float by = (float)p2y + t * (float)(p1y - p2y);

        // Map t to audio sample index
        int idx = (int)(t * (float)(n - 1) + 0.5f);
        if (idx < 0) idx = 0;
        if (idx >= (int)n) idx = (int)n - 1;

        // Normalized Left and Right displacements
        float val_l = ((float)left[idx] / (float)s_peak_l) * MAX_AMP * window;
        float val_r = ((float)right[idx] / (float)s_peak_r) * MAX_AMP * window;

        // Left channel deflects in +normal direction
        int cur_lx = (int)(bx + nx * val_l + 0.5f);
        int cur_ly = (int)(by + ny * val_l + 0.5f);

        // Right channel deflects in -normal direction
        int cur_rx = (int)(bx - nx * val_r + 0.5f);
        int cur_ry = (int)(by - ny * val_r + 0.5f);

        // Clamp to screen
        if (cur_lx < 0) cur_lx = 0; if (cur_lx >= SCREEN_W) cur_lx = SCREEN_W - 1;
        if (cur_ly < 0) cur_ly = 0; if (cur_ly >= SCREEN_H) cur_ly = SCREEN_H - 1;
        if (cur_rx < 0) cur_rx = 0; if (cur_rx >= SCREEN_W) cur_rx = SCREEN_W - 1;
        if (cur_ry < 0) cur_ry = 0; if (cur_ry >= SCREEN_H) cur_ry = SCREEN_H - 1;

        if (has_prev) {
            SafeDraw::drawLine(prev_lx, prev_ly, cur_lx, cur_ly);
            SafeDraw::drawLine(prev_rx, prev_ry, cur_rx, cur_ry);

            // Occasional vertical rib connectors for stereo depth on large pulses
            if ((i % 6 == 0) && (fabsf(val_l) > 3.0f || fabsf(val_r) > 3.0f)) {
                SafeDraw::drawLine(cur_lx, cur_ly, cur_rx, cur_ry);
            }
        }

        prev_lx = cur_lx;
        prev_ly = cur_ly;
        prev_rx = cur_rx;
        prev_ry = cur_ry;
        has_prev = true;
    }

    // 6. Draw the 2 Rotating Boundary Anchor Points
    // Point 1
    SafeDraw::drawCircle(p1x, p1y, 3);
    SafeDraw::drawDisc(p1x, p1y, 1);
    // Point 2
    SafeDraw::drawCircle(p2x, p2y, 3);
    SafeDraw::drawDisc(p2x, p2y, 1);

    // 7. HUD Telemetry Information
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 7, "20s/ROT");
    
    char deg_str[10];
    snprintf(deg_str, sizeof(deg_str), "%3d DEG", (int)(s_rot_angle * 57.2957795f));
    SafeDraw::drawStr(94, 7, deg_str);

    SafeDraw::drawStr(2, 62, "MVT");
    SafeDraw::drawStr(98, 62, "STEREO");
}
