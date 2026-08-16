#include "effects.h"

// -----------------------------------------------------------------------
// MODE 30 — MVT DANCER (Articulated Rhythm Stickman Dancing to Audio)
// -----------------------------------------------------------------------
struct DanceNote {
    float x, y;
    float vy;
    int life;
    bool is_double;
};

static const int MAX_DANCE_NOTES = 6;
static DanceNote s_notes[MAX_DANCE_NOTES];
static float s_dance_phase = 0.0f;

void effect_dancer_on_enter() {
    s_dance_phase = 0.0f;
    for (int i = 0; i < MAX_DANCE_NOTES; i++) s_notes[i].life = 0;
}

void effect_dancer_on_exit() {}

void effect_dancer_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 64;

    // 1. Audio Frequency & Bass Detection — use pre-computed frame bands
    const float bass = g_frame_bands.bass;

    int32_t pk = s_peak_l > s_peak_r ? s_peak_l : s_peak_r;
    if (pk < 1) pk = 1;

    int32_t cl = 0, cr = 0;
    for (size_t i = 0; i < n; i++) {
        int32_t vl = left[i] < 0 ? -left[i] : left[i];
        int32_t vr = right[i] < 0 ? -right[i] : right[i];
        if (vl > cl) cl = vl;
        if (vr > cr) cr = vr;
    }
    float vol_l = (float)cl / (float)pk;
    float vol_r = (float)cr / (float)pk;
    if (vol_l > 1.0f) vol_l = 1.0f;
    if (vol_r > 1.0f) vol_r = 1.0f;
    float vol = (vol_l + vol_r) * 0.5f;

    // Rhythm step phase
    s_dance_phase += 0.12f + bass * 0.25f;
    if (s_dance_phase > 6.2831853f) s_dance_phase -= 6.2831853f;

    // 2. Dancer Kinematics (Compact & Chibi Proportions)
    // Body vertical bob (squatting / jumping)
    int squat = (int)(bass * 4.0f + fabsf(sinf(s_dance_phase * 2.0f)) * 2.0f);
    int hip_y = 48 - squat;       // Lower hip position
    int neck_y = hip_y - 9;       // Compact torso (9 px instead of 15 px)
    int head_y = neck_y - 6;      // Compact neck & head position

    // Lateral sway
    float sway = sinf(s_dance_phase) * (3.0f + bass * 3.0f);
    int spine_x = cx + (int)(sway * 0.5f + 0.5f);
    int head_x  = cx + (int)(sway + 0.5f);

    // Head (radius 4 px)
    SafeDraw::drawCircle(head_x, head_y, 4);
    // Eyes
    SafeDraw::drawPixel(head_x - 1, head_y - 1);
    SafeDraw::drawPixel(head_x + 1, head_y - 1);
    // Smile
    SafeDraw::drawPixel(head_x, head_y + 1);
    // DJ Headphones band & earcups
    SafeDraw::drawCircle(head_x, head_y, 5, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT); // Headband
    SafeDraw::drawBox(head_x - 6, head_y - 2, 2, 4); // Left earcup
    SafeDraw::drawBox(head_x + 5, head_y - 2, 2, 4); // Right earcup

    // Spine / Torso (thick line)
    SafeDraw::drawLine(head_x, neck_y, spine_x, hip_y);
    SafeDraw::drawLine(head_x + 1, neck_y, spine_x + 1, hip_y);

    // Left Arm (Shorter 4.5px + 4.5px segments)
    float l_arm_ang = -1.2f - vol_l * 1.5f + sinf(s_dance_phase) * 0.6f;
    int l_elbow_x = head_x - 4 + (int)(cosf(l_arm_ang) * 4.5f);
    int l_elbow_y = neck_y + 2 + (int)(sinf(l_arm_ang) * 4.5f);
    int l_hand_x  = l_elbow_x + (int)(cosf(l_arm_ang - 0.6f) * 4.5f);
    int l_hand_y  = l_elbow_y + (int)(sinf(l_arm_ang - 0.6f) * 4.5f);
    SafeDraw::drawLine(head_x - 1, neck_y + 1, l_elbow_x, l_elbow_y);
    SafeDraw::drawLine(l_elbow_x, l_elbow_y, l_hand_x, l_hand_y);
    SafeDraw::drawPixel(l_hand_x, l_hand_y);

    // Right Arm (Shorter 4.5px + 4.5px segments)
    float r_arm_ang = 4.34f + vol_r * 1.5f - cosf(s_dance_phase) * 0.6f;
    int r_elbow_x = head_x + 4 + (int)(cosf(r_arm_ang) * 4.5f);
    int r_elbow_y = neck_y + 2 + (int)(sinf(r_arm_ang) * 4.5f);
    int r_hand_x  = r_elbow_x + (int)(cosf(r_arm_ang + 0.6f) * 4.5f);
    int r_hand_y  = r_elbow_y + (int)(sinf(r_arm_ang + 0.6f) * 4.5f);
    SafeDraw::drawLine(head_x + 1, neck_y + 1, r_elbow_x, r_elbow_y);
    SafeDraw::drawLine(r_elbow_x, r_elbow_y, r_hand_x, r_hand_y);
    SafeDraw::drawPixel(r_hand_x, r_hand_y);

    // Left Leg (Shorter legs: hip -> knee -> foot at y = 56)
    float step_l = sinf(s_dance_phase);
    int l_knee_x = spine_x - 4 - (int)(step_l * 2.0f);
    int l_knee_y = hip_y + 4;
    int l_foot_x = spine_x - 7 - (int)(step_l * 3.0f);
    int l_foot_y = 56;
    SafeDraw::drawLine(spine_x, hip_y, l_knee_x, l_knee_y);
    SafeDraw::drawLine(l_knee_x, l_knee_y, l_foot_x, l_foot_y);
    SafeDraw::drawHLine(l_foot_x - 2, l_foot_y, 4); // Shoe

    // Right Leg (Shorter legs: hip -> knee -> foot at y = 56)
    float step_r = -sinf(s_dance_phase);
    int r_knee_x = spine_x + 4 + (int)(step_r * 2.0f);
    int r_knee_y = hip_y + 4;
    int r_foot_x = spine_x + 7 + (int)(step_r * 3.0f);
    int r_foot_y = 56;
    SafeDraw::drawLine(spine_x, hip_y, r_knee_x, r_knee_y);
    SafeDraw::drawLine(r_knee_x, r_knee_y, r_foot_x, r_foot_y);
    SafeDraw::drawHLine(r_foot_x - 1, r_foot_y, 4); // Shoe

    // 3. Disco Stage Floor & Spectrum Blocks
    SafeDraw::drawHLine(0, 57, SCREEN_W);
    for (int col = 0; col < 16; col++) {
        int fx = col * 8;
        int bin = 1 + col;
        float mag = s_fft_real[bin] / (float)s_peak_l;
        int bh = (int)(mag * 5.0f);
        if (bh > 5) bh = 5;
        if (bh > 0) {
            SafeDraw::drawBox(fx + 1, 58, 6, bh);
        }
    }

    // 4. Spawn Floating Musical Notes on beats
    if (bass > 0.45f || vol > 0.4f) {
        for (int i = 0; i < MAX_DANCE_NOTES; i++) {
            if (s_notes[i].life <= 0) {
                s_notes[i].x = (float)(head_x + (rand() % 40 - 20));
                s_notes[i].y = (float)(head_y - 4);
                s_notes[i].vy = -0.8f - ((float)(rand() % 100) / 100.0f) * 0.8f;
                s_notes[i].life = 18;
                s_notes[i].is_double = (rand() % 2 == 0);
                break;
            }
        }
    }

    // Render Musical Notes
    for (int i = 0; i < MAX_DANCE_NOTES; i++) {
        if (s_notes[i].life > 0) {
            s_notes[i].y += s_notes[i].vy;
            s_notes[i].x += (sinf(s_notes[i].y * 0.2f) * 0.5f);
            s_notes[i].life--;

            int nx = (int)s_notes[i].x;
            int ny = (int)s_notes[i].y;
            // Note symbol: Disc note head + stem
            SafeDraw::drawDisc(nx, ny, 2);
            SafeDraw::drawVLine(nx + 2, ny - 6, 7);
            if (s_notes[i].is_double) {
                SafeDraw::drawDisc(nx + 6, ny - 2, 2);
                SafeDraw::drawVLine(nx + 8, ny - 8, 7);
                SafeDraw::drawHLine(nx + 2, ny - 7, 7); // Note bridge
            } else {
                SafeDraw::drawHLine(nx + 2, ny - 6, 3); // Note flag
            }
        }
    }

    // 5. Tech HUD
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 8, "MVT DANCE");
    SafeDraw::drawStr(2, 54, "DJ GROOVE");
    SafeDraw::drawStr(96, 8, "STEREO");
}
