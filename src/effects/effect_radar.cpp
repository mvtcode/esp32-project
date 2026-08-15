#include "effects.h"

// -----------------------------------------------------------------------
// MODE 22 — MVT RADAR (Cyber Sonar Scanner & Frequency Target Blips)
// -----------------------------------------------------------------------
struct RadarBlip {
    int x;
    int y;
    uint8_t life; // 0 = inactive, > 0 = fading out
};

static float s_radar_angle = 0.0f;
static RadarBlip s_blips[12];

void effect_radar_on_enter() {
    s_radar_angle = 0.0f;
    for (int i = 0; i < 12; i++) s_blips[i].life = 0;
}

void effect_radar_on_exit() {}

void effect_radar_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 64, cy = 31;

    // 1. FFT to detect audio peaks for radar blips
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

    // Advance sweep angle
    s_radar_angle += 0.08f + bass * 0.08f;
    if (s_radar_angle > 6.28318f) s_radar_angle -= 6.28318f;

    // 2. Map current angle to FFT bin and spawn blip if strong
    int bin_idx = 1 + (int)((s_radar_angle / 6.28318f) * 28.0f);
    float bin_mag = s_fft_real[bin_idx] / (float)s_peak_l;
    if (bin_mag > 0.35f) {
        float blip_dist = 6.0f + bin_mag * 22.0f;
        if (blip_dist > 28.0f) blip_dist = 28.0f;
        int bx = cx + (int)(cosf(s_radar_angle) * blip_dist);
        int by = cy + (int)(sinf(s_radar_angle) * blip_dist);

        // Find empty or oldest slot
        for (int i = 0; i < 12; i++) {
            if (s_blips[i].life == 0) {
                s_blips[i].x = bx;
                s_blips[i].y = by;
                s_blips[i].life = 18;
                break;
            }
        }
    }

    // 3. Draw Radar Grid HUD
    SafeDraw::drawCircle(cx, cy, 30);
    SafeDraw::drawCircle(cx, cy, 20);
    SafeDraw::drawCircle(cx, cy, 10);
    // Crosshairs
    SafeDraw::drawHLine(cx - 30, cy, 61);
    SafeDraw::drawVLine(cx, cy - 30, 61);

    // 4. Draw Rotating Sweep Line + Sweep beam fan
    int sx = cx + (int)(cosf(s_radar_angle) * 30.0f);
    int sy = cy + (int)(sinf(s_radar_angle) * 30.0f);
    SafeDraw::drawLine(cx, cy, sx, sy);

    float trail_ang = s_radar_angle - 0.12f;
    int tx = cx + (int)(cosf(trail_ang) * 26.0f);
    int ty = cy + (int)(sinf(trail_ang) * 26.0f);
    SafeDraw::drawLine(cx, cy, tx, ty);

    // 5. Draw Blips & Fade trails
    for (int i = 0; i < 12; i++) {
        if (s_blips[i].life > 0) {
            if (s_blips[i].life > 8) {
                SafeDraw::drawDisc(s_blips[i].x, s_blips[i].y, 2);
                SafeDraw::drawFrame(s_blips[i].x - 3, s_blips[i].y - 3, 7, 7); // Target lock box
            } else {
                SafeDraw::drawPixel(s_blips[i].x, s_blips[i].y);
            }
            s_blips[i].life--;
        }
    }

    // 6. HUD Telemetry Corners
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 8, "SONAR");
    SafeDraw::drawStr(2, 60, "SYS:OK");
    SafeDraw::drawStr(100, 8, "MVT-99");
    char ang_str[10];
    snprintf(ang_str, sizeof(ang_str), "%3d DEG", (int)(s_radar_angle * 57.2958f));
    SafeDraw::drawStr(94, 60, ang_str);
}
