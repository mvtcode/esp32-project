#include "effects.h"

// -----------------------------------------------------------------------
// MODE 64 — CYBER NEKO (Stereo Ear Twitches, Waveform Meow & Wagging Tail)
// -----------------------------------------------------------------------
static float s_cat_bob_y = 0.0f;
static float s_tail_angle = 0.0f;
static float s_left_ear_twitch = 0.0f;
static float s_right_ear_twitch = 0.0f;

void effect_cat_on_enter() {
    s_cat_bob_y = 0.0f;
    s_tail_angle = 0.0f;
    s_left_ear_twitch = 0.0f;
    s_right_ear_twitch = 0.0f;
}

void effect_cat_on_exit() {}

void effect_cat_render(const int32_t *left, const int32_t *right, size_t n) {
    // 1. RMS & Stereo Balance
    int64_t sum_l = 0, sum_r = 0;
    for (size_t i = 0; i < n; i++) {
        sum_l += (left[i] < 0 ? -left[i] : left[i]);
        sum_r += (right[i] < 0 ? -right[i] : right[i]);
    }
    float norm_l = (float)sum_l / ((float)n * (float)s_peak_l);
    float norm_r = (float)sum_r / ((float)n * (float)s_peak_r);
    if (norm_l > 1.0f) norm_l = 1.0f;
    if (norm_r > 1.0f) norm_r = 1.0f;
    float avg = (norm_l + norm_r) * 0.5f;

    // FFT for Bass Beat (Triggers Headbob)
    for (size_t i = 0; i < n; i++) {
        s_fft_real[i] = (float)((left[i] + right[i]) / 2);
        s_fft_imag[i] = 0.0f;
    }
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();

    float bass = 0.0f;
    for (int b = 1; b <= 4; b++) bass += s_fft_real[b];
    bass /= (4.0f * (float)s_peak_l);
    if (bass > 1.0f) bass = 1.0f;

    // Headbobbing animation
    float target_bob = bass * 5.0f;
    s_cat_bob_y += (target_bob - s_cat_bob_y) * 0.35f;

    // Stereo Ear Twitching
    s_left_ear_twitch += ((norm_l * 6.0f) - s_left_ear_twitch) * 0.25f;
    s_right_ear_twitch += ((norm_r * 6.0f) - s_right_ear_twitch) * 0.25f;

    int hcx = 64;
    int hcy = 30 + (int)s_cat_bob_y;

    // 2. Ears (Left Ear reacts to Left channel, Right Ear to Right channel)
    // Left Ear
    int le_tip_x = hcx - 18 - (int)s_left_ear_twitch;
    int le_tip_y = hcy - 22 + (int)(s_left_ear_twitch * 0.5f);
    SafeDraw::drawTriangle(hcx - 22, hcy - 8, hcx - 8, hcy - 14, le_tip_x, le_tip_y);
    // Left inner ear
    SafeDraw::drawLine(hcx - 18, hcy - 8, le_tip_x + 2, le_tip_y + 3);

    // Right Ear
    int re_tip_x = hcx + 18 + (int)s_right_ear_twitch;
    int re_tip_y = hcy - 22 + (int)(s_right_ear_twitch * 0.5f);
    SafeDraw::drawTriangle(hcx + 8, hcy - 14, hcx + 22, hcy - 8, re_tip_x, re_tip_y);
    // Right inner ear
    SafeDraw::drawLine(hcx + 18, hcy - 8, re_tip_x - 2, re_tip_y + 3);

    // 3. Cat Head & Face Outline
    SafeDraw::drawDisc(hcx, hcy, 18);

    // 4. Cat Eyes (Big sparkling eyes)
    int eye_lx = hcx - 8;
    int eye_rx = hcx + 8;
    int eye_y = hcy - 2;

    SafeDraw::setDrawColor(0);
    SafeDraw::drawDisc(eye_lx, eye_y, 4);
    SafeDraw::drawDisc(eye_rx, eye_y, 4);
    SafeDraw::setDrawColor(1);

    // Slit or Round Pupil depending on bass
    if (bass > 0.4f) {
        // Dilated pupils (Excited)
        SafeDraw::drawDisc(eye_lx, eye_y, 2);
        SafeDraw::drawDisc(eye_rx, eye_y, 2);
    } else {
        // Vertical slit pupils
        SafeDraw::drawVLine(eye_lx, eye_y - 2, 5);
        SafeDraw::drawVLine(eye_rx, eye_y - 2, 5);
    }
    // Eye Sparkle
    SafeDraw::drawPixel(eye_lx - 1, eye_y - 2);
    SafeDraw::drawPixel(eye_rx - 1, eye_y - 2);

    // 5. Nose & Whiskers
    SafeDraw::setDrawColor(0);
    SafeDraw::drawPixel(hcx, hcy + 4); // Tiny nose dot
    SafeDraw::setDrawColor(1);

    // Whiskers Left
    SafeDraw::drawLine(hcx - 14, hcy + 3, hcx - 30, hcy + 1);
    SafeDraw::drawLine(hcx - 14, hcy + 5, hcx - 32, hcy + 6);
    SafeDraw::drawLine(hcx - 14, hcy + 7, hcx - 29, hcy + 11);
    // Whiskers Right
    SafeDraw::drawLine(hcx + 14, hcy + 3, hcx + 30, hcy + 1);
    SafeDraw::drawLine(hcx + 14, hcy + 5, hcx + 32, hcy + 6);
    SafeDraw::drawLine(hcx + 14, hcy + 7, hcx + 29, hcy + 11);

    // 6. Mouth: Idle 'w' or Open 'Meow'
    int mouth_y = hcy + 8;
    if (avg < 0.15f) {
        // Cute ':3' cat mouth
        SafeDraw::setDrawColor(0);
        SafeDraw::drawPixel(hcx - 3, mouth_y);
        SafeDraw::drawPixel(hcx - 2, mouth_y + 1);
        SafeDraw::drawPixel(hcx - 1, mouth_y);
        SafeDraw::drawPixel(hcx, mouth_y - 1);
        SafeDraw::drawPixel(hcx + 1, mouth_y);
        SafeDraw::drawPixel(hcx + 2, mouth_y + 1);
        SafeDraw::drawPixel(hcx + 3, mouth_y);
        SafeDraw::setDrawColor(1);
    } else {
        // Open Meow mouth modulated by audio
        int mw = 4 + (int)(avg * 8.0f);
        int mh = 2 + (int)(avg * 6.0f);
        SafeDraw::setDrawColor(0);
        SafeDraw::drawBox(hcx - mw / 2, mouth_y - 1, mw, mh);
        SafeDraw::setDrawColor(1);
        // Pink tongue inside
        SafeDraw::drawPixel(hcx, mouth_y + mh - 2);
    }

    // 7. Cat Paws at the bottom
    SafeDraw::drawDisc(hcx - 10, hcy + 18, 5);
    SafeDraw::drawDisc(hcx + 10, hcy + 18, 5);
    SafeDraw::setDrawColor(0);
    SafeDraw::drawVLine(hcx - 11, hcy + 17, 4);
    SafeDraw::drawVLine(hcx - 9, hcy + 17, 4);
    SafeDraw::drawVLine(hcx + 9, hcy + 17, 4);
    SafeDraw::drawVLine(hcx + 11, hcy + 17, 4);
    SafeDraw::setDrawColor(1);

    // 8. Wagging Tail on the right
    s_tail_angle += 0.08f + avg * 0.15f;
    int tail_root_x = hcx + 20;
    int tail_root_y = 56;
    int tail_tip_x = tail_root_x + 14 + (int)(sinf(s_tail_angle) * 8.0f);
    int tail_tip_y = tail_root_y - 12 + (int)(cosf(s_tail_angle) * 5.0f);

    SafeDraw::drawLine(tail_root_x, tail_root_y, tail_root_x + 8, tail_root_y - 6);
    SafeDraw::drawLine(tail_root_x + 8, tail_root_y - 6, tail_tip_x, tail_tip_y);
    SafeDraw::drawDisc(tail_tip_x, tail_tip_y, 2);

    // 9. HUD Telemetry
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 6, "CYBER NEKO");
    if (avg > 0.25f) {
        SafeDraw::drawStr(98, 6, "MEOW! :3");
    } else {
        SafeDraw::drawStr(98, 6, "PURR... ^w^");
    }
}
