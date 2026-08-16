#include "effects.h"

// -----------------------------------------------------------------------
// MODE 61 — MVT AI ROBOT (Cute AI Companion, Antenna Broadcast & Ear EQs)
// -----------------------------------------------------------------------
static float s_head_sway = 0.0f;
static float s_antenna_wave = 0.0f;

void effect_ai_bot_on_enter() {
    s_head_sway = 0.0f;
    s_antenna_wave = 0.0f;
}

void effect_ai_bot_on_exit() {}

void effect_ai_bot_render(const int32_t *left, const int32_t *right, size_t n) {
    // 1. Audio Processing: RMS & Stereo
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

    // FFT for Bass Beat
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

    // Head sway animation
    float target_sway = (norm_r - norm_l) * 10.0f + sinf((float)millis() * 0.004f) * (2.0f + avg * 4.0f);
    s_head_sway += (target_sway - s_head_sway) * 0.2f;

    int hcx = 64 + (int)s_head_sway;
    int hcy = 36;

    // 2. Antenna Broadcast Radio Waves (Beat-reactive)
    s_antenna_wave += 0.8f + bass * 2.0f;
    if (s_antenna_wave > 20.0f) s_antenna_wave = 2.0f;

    int ant_top_x = hcx;
    int ant_top_y = hcy - 24;

    // Antenna stem & bulb
    SafeDraw::drawLine(hcx, hcy - 16, ant_top_x, ant_top_y);
    SafeDraw::drawDisc(ant_top_x, ant_top_y, 3 + (int)(bass * 2.0f));

    // Radio broadcast ripples
    if (bass > 0.35f || avg > 0.25f) {
        int wave_r = (int)s_antenna_wave;
        SafeDraw::drawCircle(ant_top_x, ant_top_y, wave_r, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
        if (wave_r > 8) {
            SafeDraw::drawCircle(ant_top_x, ant_top_y, wave_r - 6, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
        }
    }

    // 3. Robot Head Outer Frame & Ears
    // Head Helmet Outer Shell
    SafeDraw::drawRFrame(hcx - 26, hcy - 16, 52, 38, 12);
    // Visor Face Plate Inner Frame
    SafeDraw::drawRFrame(hcx - 22, hcy - 12, 44, 30, 8);

    // Left Earphone Headset (Stereo L EQ)
    int lear_x = hcx - 30;
    SafeDraw::drawRBox(lear_x - 3, hcy - 8, 4, 18, 2);
    int l_bar = (int)(norm_l * 12.0f);
    if (l_bar > 0) SafeDraw::drawVLine(lear_x - 5, hcy + 5 - l_bar, l_bar);

    // Right Earphone Headset (Stereo R EQ)
    int rear_x = hcx + 30;
    SafeDraw::drawRBox(rear_x - 1, hcy - 8, 4, 18, 2);
    int r_bar = (int)(norm_r * 12.0f);
    if (r_bar > 0) SafeDraw::drawVLine(rear_x + 4, hcy + 5 - r_bar, r_bar);

    // 4. Robot Eyes (Cute glowing circular screen eyes)
    int eye_lx = hcx - 11;
    int eye_rx = hcx + 11;
    int eye_y = hcy - 2;

    int eye_r = 5 + (int)(bass * 1.5f);
    SafeDraw::drawDisc(eye_lx, eye_y, eye_r);
    SafeDraw::drawDisc(eye_rx, eye_y, eye_r);

    // Eye Pupils (Dark sparkles)
    SafeDraw::setDrawColor(0);
    SafeDraw::drawPixel(eye_lx - 1, eye_y - 1);
    SafeDraw::drawPixel(eye_lx - 2, eye_y - 2);
    SafeDraw::drawPixel(eye_rx - 1, eye_y - 1);
    SafeDraw::drawPixel(eye_rx - 2, eye_y - 2);
    SafeDraw::setDrawColor(1);

    // 5. Robot Mouth (Happy Talking Smile)
    int mouth_w = 8 + (int)(avg * 8.0f);
    int mouth_my = hcy + 10;
    if (avg < 0.15f) {
        // Idle smile
        SafeDraw::drawHLine(hcx - 3, mouth_my, 7);
        SafeDraw::drawPixel(hcx - 4, mouth_my - 1);
        SafeDraw::drawPixel(hcx + 4, mouth_my - 1);
    } else {
        // Dynamic talking oval mouth
        int mouth_h = 2 + (int)(avg * 6.0f);
        SafeDraw::drawEllipse(hcx, mouth_my, mouth_w / 2, mouth_h);
    }

    // 6. Chest / Body Collar
    SafeDraw::drawHLine(hcx - 14, hcy + 22, 28);
    SafeDraw::drawLine(hcx - 14, hcy + 22, hcx - 18, 63);
    SafeDraw::drawLine(hcx + 14, hcy + 22, hcx + 18, 63);
    // Pulsing chest core light
    if (bass > 0.4f) {
        SafeDraw::drawDisc(hcx, 58, 2 + (int)(bass * 2.0f));
    } else {
        SafeDraw::drawCircle(hcx, 58, 2);
    }

    // 7. HUD Telemetry
    // SafeDraw::setFont(u8g2_font_4x6_tr);
    // SafeDraw::drawStr(2, 6, "AI COMPANION");
    // SafeDraw::drawStr(98, 62, "MVT-BOT");
}
