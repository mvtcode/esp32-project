#include "effects.h"

// -----------------------------------------------------------------------
// MODE 60 — XIAOZHI AI FACE (Cheerful Robot Eyes & Animated Singing Smile)
// -----------------------------------------------------------------------
static int s_blink_timer = 0;
static bool s_is_blinking = false;
static float s_eye_look_x = 0.0f;
static float s_eye_look_y = 0.0f;
static float s_mouth_open = 0.0f;
static float s_mouth_smooth_w = 12.0f;
static float s_beat_pulse = 0.0f;

void effect_xiaozhi_on_enter() {
    s_blink_timer = 0;
    s_is_blinking = false;
    s_eye_look_x = 0.0f;
    s_eye_look_y = 0.0f;
    s_mouth_open = 0.0f;
    s_mouth_smooth_w = 12.0f;
    s_beat_pulse = 0.0f;
}

void effect_xiaozhi_on_exit() {}

// Draw expressive Xiaozhi eye (Warm, cheerful robot screen eye)
static void draw_xiaozhi_eye(int cx, int cy, int eye_w, int eye_h, int eye_r,
                             float look_x, float look_y, int pupil_r,
                             bool happy_crescent, bool wink) {
    if (wink) {
        // Playful Wink (> or ^)
        SafeDraw::drawLine(cx - 8, cy - 2, cx, cy + 3);
        SafeDraw::drawLine(cx, cy + 3, cx + 8, cy - 2);
        SafeDraw::drawLine(cx - 8, cy - 1, cx, cy + 4);
        SafeDraw::drawLine(cx, cy + 4, cx + 8, cy - 1);
        return;
    }

    if (happy_crescent) {
        // Ultra-cute happy smiling arch eye (^ ^)
        int half_w = eye_w / 2 - 2;
        for (int t = -half_w; t <= half_w; t++) {
            float norm_t = (float)t / (float)half_w;
            int px = cx + t;
            int py = cy - (int)((1.0f - norm_t * norm_t) * 8.0f) + 2;
            SafeDraw::drawPixel(px, py);
            SafeDraw::drawPixel(px, py + 1);
            SafeDraw::drawPixel(px, py + 2);
        }
        // Cute upward eye corners
        SafeDraw::drawPixel(cx - half_w - 1, cy + 2);
        SafeDraw::drawPixel(cx + half_w + 1, cy + 2);
        return;
    }

    // 1. Fixed Outer Eye Screen (Soft pill-shaped robot screen eye)
    int left_x = cx - eye_w / 2;
    int top_y  = cy - eye_h / 2;
    SafeDraw::drawRBox(left_x, top_y, eye_w, eye_h, eye_r);

    // 2. Gentle friendly eyelid curvature (Soft top lid cut for warm friendly look)
    SafeDraw::setDrawColor(0);
    SafeDraw::drawPixel(left_x, top_y);
    SafeDraw::drawPixel(left_x + eye_w - 1, top_y);

    // 3. Dynamic Moving Pupil (Con ngươi to tròn đảo mượt mà theo nhạc)
    int max_offset_x = eye_w / 2 - pupil_r - 2;
    int max_offset_y = eye_h / 2 - pupil_r - 2;
    if (max_offset_x < 1) max_offset_x = 1;
    if (max_offset_y < 1) max_offset_y = 1;

    int px_offset = (int)look_x;
    int py_offset = (int)look_y;
    if (px_offset > max_offset_x) px_offset = max_offset_x;
    if (px_offset < -max_offset_x) px_offset = -max_offset_x;
    if (py_offset > max_offset_y) py_offset = max_offset_y;
    if (py_offset < -max_offset_y) py_offset = -max_offset_y;

    int pupil_cx = cx + px_offset;
    int pupil_cy = cy + py_offset;

    // Draw solid dark pupil (Color 0)
    SafeDraw::drawDisc(pupil_cx, pupil_cy, pupil_r);

    // 4. Sparkling Anime Glints (Ánh mắt long lanh đáng yêu - Color 1)
    SafeDraw::setDrawColor(1);
    // Primary glossy light reflection
    SafeDraw::drawDisc(pupil_cx - 2, pupil_cy - 2, 1);
    SafeDraw::drawPixel(pupil_cx - 2, pupil_cy - 2);

    // Secondary cute glimmer
    SafeDraw::drawPixel(pupil_cx + 2, pupil_cy + 1);
}

// Draw cute smiling & talking mouth
static void draw_xiaozhi_mouth(int cx, int cy, float open_amount, float smooth_w) {
    int mw = (int)smooth_w;
    if (mw < 10) mw = 10;
    if (mw > 24) mw = 24;

    if (open_amount < 1.2f) {
        // Sweet resting smile: ( ◡ )
        int half = mw / 2;
        for (int x = -half; x <= half; x++) {
            float norm = (float)x / (float)half;
            int y_curve = cy + (int)(norm * norm * 3.0f);
            SafeDraw::drawPixel(cx + x, y_curve);
            if (abs(x) < half - 1) {
                SafeDraw::drawPixel(cx + x, y_curve + 1);
            }
        }
        // Cute smiling mouth corners
        SafeDraw::drawPixel(cx - half - 1, cy - 1);
        SafeDraw::drawPixel(cx + half + 1, cy - 1);
    } else {
        // Singing / Talking open smiling mouth (D-shape smile :D)
        int mh = 3 + (int)(open_amount * 1.0f);
        if (mh > 11) mh = 11;
        int half = mw / 2;

        int top_y = cy - mh / 2;
        int bot_y = cy + mh / 2;

        // Top lip line
        SafeDraw::drawHLine(cx - half + 1, top_y, mw - 2);

        // Parabolic lower lip curve
        for (int x = -half; x <= half; x++) {
            float norm = (float)x / (float)half;
            int cur_y = top_y + (int)((1.0f - norm * norm) * (float)mh);
            SafeDraw::drawPixel(cx + x, cur_y);
            SafeDraw::drawPixel(cx + x, cur_y - 1);
        }

        // Cute tongue inside open mouth
        if (mh >= 6) {
            SafeDraw::drawDisc(cx, bot_y - 2, 2);
        }

        // Smiling dimples at corners
        SafeDraw::drawPixel(cx - half - 1, top_y - 1);
        SafeDraw::drawPixel(cx + half + 1, top_y - 1);
    }
}

void effect_xiaozhi_render(const int32_t *left, const int32_t *right, size_t n) {
    const int eye_left_x  = 40;
    const int eye_right_x = 88;
    const int eye_y       = 24;
    const int mouth_cx    = 64;
    const int mouth_y     = 50;

    // -------------------------------------------------------------------
    // 1. Audio Energy & Stereo Analysis
    // -------------------------------------------------------------------
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

    // -------------------------------------------------------------------
    // 2. Frequency Bands (Bass groove & Vocal/Treble energy)
    // -------------------------------------------------------------------
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

    float treble = 0.0f;
    for (int b = 5; b <= 16; b++) treble += s_fft_real[b];
    treble /= (12.0f * (float)s_peak_l);
    if (treble > 1.0f) treble = 1.0f;

    // Beat pulse for bouncy rhythm groove
    if (bass > 0.40f && bass > s_beat_pulse) {
        s_beat_pulse = bass;
    } else {
        s_beat_pulse *= 0.82f;
    }

    // -------------------------------------------------------------------
    // 3. Dynamic Smooth Pupil Movement (Đảo con ngươi theo nhạc)
    // -------------------------------------------------------------------
    uint32_t now = millis();

    // Horizontal look: Stereo Pan direction + smooth rhythmic swaying
    float stereo_pan = (norm_r - norm_l) * 4.2f;
    float rhythm_sway_x = sinf((float)now * 0.0032f) * (avg * 3.2f + 1.0f);
    float target_look_x = stereo_pan + rhythm_sway_x;

    // Vertical look: Gently looking up on bright singing vocals, bouncy on bass
    float vocal_tilt_y = -(treble * 2.6f) + (bass * 1.5f);
    float groove_bounce_y = cosf((float)now * 0.0042f) * (avg * 1.8f);
    float target_look_y = vocal_tilt_y + groove_bounce_y;

    // Smooth organic interpolation
    s_eye_look_x += (target_look_x - s_eye_look_x) * 0.26f;
    s_eye_look_y += (target_look_y - s_eye_look_y) * 0.26f;

    // Pupil size (Cute large expressive pupil)
    int pupil_r = 6 + (int)(s_beat_pulse * 1.5f);
    if (pupil_r > 7) pupil_r = 7;

    // -------------------------------------------------------------------
    // 4. Natural Blinking & Cheerful Expressions
    // -------------------------------------------------------------------
    s_blink_timer++;
    int blink_period = (avg > 0.30f) ? 140 : 85;
    if (s_blink_timer > blink_period) {
        s_is_blinking = true;
        if (s_blink_timer > blink_period + 4) {
            s_is_blinking = false;
            s_blink_timer = 0;
        }
    }

    // Happy singing expression (when music has lively energy)
    bool is_happy = (avg > 0.30f && (treble > 0.22f || s_beat_pulse > 0.45f));
    bool wink_l = (!is_happy && norm_r > norm_l + 0.38f);
    bool wink_r = (!is_happy && norm_l > norm_r + 0.38f);

    const int eye_w = 26;
    const int eye_h = 28;
    const int eye_r = 8;

    // -------------------------------------------------------------------
    // 5. Render Cheerful Eyes
    // -------------------------------------------------------------------
    if (s_is_blinking) {
        // Cute smiling closed eyelids
        SafeDraw::drawHLine(eye_left_x - 10, eye_y, 20);
        SafeDraw::drawHLine(eye_left_x - 8, eye_y + 1, 16);
        SafeDraw::drawHLine(eye_right_x - 10, eye_y, 20);
        SafeDraw::drawHLine(eye_right_x - 8, eye_y + 1, 16);
    } else {
        // Left Eye
        draw_xiaozhi_eye(eye_left_x, eye_y, eye_w, eye_h, eye_r,
                         s_eye_look_x, s_eye_look_y, pupil_r,
                         is_happy, wink_l);
        // Right Eye
        draw_xiaozhi_eye(eye_right_x, eye_y, eye_w, eye_h, eye_r,
                         s_eye_look_x, s_eye_look_y, pupil_r,
                         is_happy, wink_r);
    }

    // -------------------------------------------------------------------
    // 6. Cute Cheeks Blush (Má hồng tỏa sáng vui vẻ)
    // -------------------------------------------------------------------
    int blush_len = 4 + (int)(avg * 5.0f);
    if (blush_len > 8) blush_len = 8;
    // Left cheek
    SafeDraw::drawHLine(18, 40, blush_len);
    SafeDraw::drawHLine(20, 42, blush_len - 2);
    // Right cheek
    SafeDraw::drawHLine(110 - blush_len, 40, blush_len);
    SafeDraw::drawHLine(110 - blush_len + 2, 42, blush_len - 2);

    // -------------------------------------------------------------------
    // 7. Cheerful Animated Singing & Smiling Mouth
    // -------------------------------------------------------------------
    float target_open = avg * 11.0f + s_beat_pulse * 4.0f;
    s_mouth_open += (target_open - s_mouth_open) * 0.32f;

    float target_w = 12.0f + avg * 8.0f + s_beat_pulse * 3.0f;
    s_mouth_smooth_w += (target_w - s_mouth_smooth_w) * 0.25f;

    draw_xiaozhi_mouth(mouth_cx, mouth_y, s_mouth_open, s_mouth_smooth_w);
}


