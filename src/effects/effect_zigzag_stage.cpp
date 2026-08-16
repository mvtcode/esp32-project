#include "effects.h"

// -----------------------------------------------------------------------
// MODE 63 — EDM ZIGZAG STAGE (Multi-Tier Chevron LED Arrays & Chasing Pulses)
// -----------------------------------------------------------------------
static float s_pulse_phase = 0.0f;
static int s_strobe_timer = 0;

void effect_zigzag_stage_on_enter() {
    s_pulse_phase = 0.0f;
    s_strobe_timer = 0;
}

void effect_zigzag_stage_on_exit() {}

// Helper to draw a dashed / segmented zigzag line
static void draw_segmented_zigzag(int x1, int y1, int x2, int y2, int x3, int y3, float phase, bool invert_phase) {
    auto draw_dashed_seg = [](int ax, int ay, int bx, int by, float p) {
        int steps = 12;
        for (int i = 0; i < steps; i++) {
            float t1 = (float)i / (float)steps;
            float t2 = (float)(i + 0.6f) / (float)steps;
            float pulse_val = fmodf(t1 + p, 1.0f);
            if (pulse_val < 0.55f) {
                int px1 = ax + (int)((bx - ax) * t1);
                int py1 = ay + (int)((by - ay) * t1);
                int px2 = ax + (int)((bx - ax) * t2);
                int py2 = ay + (int)((by - ay) * t2);
                SafeDraw::drawLine(px1, py1, px2, py2);
            }
        }
    };

    float p = invert_phase ? (1.0f - phase) : phase;
    draw_dashed_seg(x1, y1, x2, y2, p);
    draw_dashed_seg(x2, y2, x3, y3, p + 0.25f);
}

void effect_zigzag_stage_render(const int32_t *left, const int32_t *right, size_t n) {
    // 1. RMS & Stereo
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

    // FFT for Bass Beat & Central Towers
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

    if (bass > 0.55f) {
        s_strobe_timer = 2; // Strobe flash
    }
    if (s_strobe_timer > 0) s_strobe_timer--;

    // Update chasing pulse phase
    s_pulse_phase += 0.04f + avg * 0.12f;
    if (s_pulse_phase > 1.0f) s_pulse_phase -= 1.0f;

    // 2. Left Wing Multi-Tier Zigzag LED Arrays
    // Tier 1 (Top Left)
    draw_segmented_zigzag(0, 10, 22, 2, 44, 12, s_pulse_phase * (1.0f + norm_l), false);
    // Tier 2 (Mid Left)
    draw_segmented_zigzag(0, 26, 26, 16, 44, 28, s_pulse_phase * (1.0f + norm_l), false);
    // Tier 3 (Bottom Left)
    draw_segmented_zigzag(0, 42, 22, 32, 44, 46, s_pulse_phase * (1.0f + norm_l), false);

    // 3. Right Wing Multi-Tier Zigzag LED Arrays
    // Tier 1 (Top Right)
    draw_segmented_zigzag(127, 10, 105, 2, 83, 12, s_pulse_phase * (1.0f + norm_r), true);
    // Tier 2 (Mid Right)
    draw_segmented_zigzag(127, 26, 101, 16, 83, 28, s_pulse_phase * (1.0f + norm_r), true);
    // Tier 3 (Bottom Right)
    draw_segmented_zigzag(127, 42, 105, 32, 83, 46, s_pulse_phase * (1.0f + norm_r), true);

    // 4. Center Stage Arch & Vertical LED Matrix Towers (5 columns)
    // Overhead Stage Arch
    SafeDraw::drawLine(46, 14, 64, 4);
    SafeDraw::drawLine(64, 4, 81, 14);

    for (int col = 0; col < 5; col++) {
        int tx = 52 + col * 6;
        int bin = 2 + col * 3;
        float mag = s_fft_real[bin] / (float)s_peak_l;
        if (mag > 1.0f) mag = 1.0f;

        int tower_h = 4 + (int)(mag * 36.0f);
        if (tower_h > 38) tower_h = 38;

        // Draw dotted LED tower column
        for (int y = 52; y >= 52 - tower_h; y -= 3) {
            SafeDraw::drawPixel(tx, y);
            SafeDraw::drawPixel(tx + 1, y);
        }
    }

    // 5. Stage Base & DJ / Dancer Silhouettes
    SafeDraw::drawHLine(44, 54, 40);

    // DJ Booth & Figures
    SafeDraw::drawBox(58, 55, 12, 5); // DJ table
    // Center DJ silhouette
    SafeDraw::drawDisc(64, 49, 2);
    SafeDraw::drawLine(64, 51, 64, 55);
    // Left & Right Dancers on bass
    if (bass > 0.3f) {
        SafeDraw::drawDisc(48, 50, 2);
        SafeDraw::drawLine(48, 52, 48, 56);
        SafeDraw::drawDisc(80, 50, 2);
        SafeDraw::drawLine(80, 52, 80, 56);
    }

    // 6. Bass Strobe Flashes along Wing Tips
    if (s_strobe_timer > 0) {
        SafeDraw::drawCircle(22, 2, 4);
        SafeDraw::drawCircle(105, 2, 4);
        SafeDraw::drawCircle(26, 16, 4);
        SafeDraw::drawCircle(101, 16, 4);
    }

    // 7. HUD Telemetry
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 62, "EDM ZIGZAG");
    SafeDraw::drawStr(96, 62, "MVT-STAGE");
}
