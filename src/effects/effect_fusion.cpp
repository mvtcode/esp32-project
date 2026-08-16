#include "effects.h"

// -----------------------------------------------------------------------
// MODE 8 — MVT FUSION (Cyber Fusion: 3D spinning MVT + radial rays + flank waves)
// -----------------------------------------------------------------------
#define FUSION_FLANK_BARS 13
#define FUSION_RAD_BARS   8
static float s_fusion_angle = 0.0f;
static float s_fusion_scale = FFT_MAG_FLOOR;
static float s_fusion_pk_t[FUSION_RAD_BARS] = {0};
static float s_fusion_pk_b[FUSION_RAD_BARS] = {0};
static float s_fusion_flank_pk_l[FUSION_FLANK_BARS] = {0};
static float s_fusion_flank_pk_r[FUSION_FLANK_BARS] = {0};

void effect_fusion_on_enter() {
    s_fusion_scale = FFT_MAG_FLOOR;
    memset(s_fusion_pk_t, 0, sizeof(s_fusion_pk_t));
    memset(s_fusion_pk_b, 0, sizeof(s_fusion_pk_b));
    memset(s_fusion_flank_pk_l, 0, sizeof(s_fusion_flank_pk_l));
    memset(s_fusion_flank_pk_r, 0, sizeof(s_fusion_flank_pk_r));
}

void effect_fusion_on_exit() {
    s_fusion_scale = FFT_MAG_FLOOR;
    memset(s_fusion_pk_t, 0, sizeof(s_fusion_pk_t));
    memset(s_fusion_pk_b, 0, sizeof(s_fusion_pk_b));
    memset(s_fusion_flank_pk_l, 0, sizeof(s_fusion_flank_pk_l));
    memset(s_fusion_flank_pk_r, 0, sizeof(s_fusion_flank_pk_r));
}

void effect_fusion_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 64;
    const int cy = 31;
    const int R_IN = 11;
    const int R_MID = 14;
    const int MAX_FLANK_H = 18;
    const int MAX_RAD_LEN = 13;

    float flank_l[FUSION_FLANK_BARS] = {0};
    float flank_r[FUSION_FLANK_BARS] = {0};
    float rad_top[FUSION_RAD_BARS]   = {0};
    float rad_bot[FUSION_RAD_BARS]   = {0};

    // 1. FFT on Left Channel
    for (size_t i = 0; i < n; i++) {
        s_fft_real[i] = (float)left[i];
        s_fft_imag[i] = 0.0f;
    }
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();

    float max_mag = 0.0f;
    for (int k = 0; k < FUSION_FLANK_BARS; k++) {
        float sum = 0.0f;
        int start_bin = 1 + k * 3;
        for (int b = 0; b < 3 && (start_bin + b) < (int)(n / 2); b++) {
            sum += s_fft_real[start_bin + b];
        }
        flank_l[k] = sum / 3.0f;
        if (flank_l[k] > max_mag) max_mag = flank_l[k];
    }
    for (int k = 0; k < FUSION_RAD_BARS; k++) {
        rad_top[k] = s_fft_real[1 + k * 4];
        if (rad_top[k] > max_mag) max_mag = rad_top[k];
    }

    // 2. FFT on Right Channel
    for (size_t i = 0; i < n; i++) {
        s_fft_real[i] = (float)right[i];
        s_fft_imag[i] = 0.0f;
    }
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();

    for (int k = 0; k < FUSION_FLANK_BARS; k++) {
        float sum = 0.0f;
        int start_bin = 1 + k * 3;
        for (int b = 0; b < 3 && (start_bin + b) < (int)(n / 2); b++) {
            sum += s_fft_real[start_bin + b];
        }
        flank_r[k] = sum / 3.0f;
        if (flank_r[k] > max_mag) max_mag = flank_r[k];
    }
    for (int k = 0; k < FUSION_RAD_BARS; k++) {
        rad_bot[k] = s_fft_real[1 + k * 4];
        if (rad_bot[k] > max_mag) max_mag = rad_bot[k];
    }

    // Dynamic AGC
    s_fusion_scale = max_mag > s_fusion_scale ? max_mag : (s_fusion_scale * 0.95f);
    if (s_fusion_scale < FFT_MAG_FLOOR) s_fusion_scale = FFT_MAG_FLOOR;

    // Clockwise rotation: ~10s per revolution (2*PI / 10s = 0.6283 rad/s)
    static uint32_t s_last_rot_ts = 0;
    uint32_t now = millis();
    float dt = (now - s_last_rot_ts) * 0.001f;
    if (dt > 0.1f || dt < 0.001f) dt = 0.035f;
    s_last_rot_ts = now;

    // Base rotation rate: 1 round / 10 seconds, smoothly accelerated slightly with bass
    float bass_energy = (flank_l[0] + flank_r[0]) / (2.0f * s_fusion_scale);
    float rot_rate = (6.283185f / 10.0f) + bass_energy * 0.40f;
    s_fusion_angle += rot_rate * dt;
    if (s_fusion_angle > 6.283185f) s_fusion_angle -= 6.283185f;

    // 3. Baselines connecting to central circle
    SafeDraw::drawHLine(0, cy, 50);
    SafeDraw::drawHLine(78, cy, 50);

    // 4. Left Flank Waves (Bass near center, Treble toward left edge)
    for (int k = 0; k < FUSION_FLANK_BARS; k++) {
        int band_idx = (FUSION_FLANK_BARS - 1) - k;
        float norm_l = flank_l[band_idx] / s_fusion_scale;
        if (norm_l > 1.0f) norm_l = 1.0f;
        int h = (int)(powf(norm_l, 0.60f) * MAX_FLANK_H);
        if (h < 0) h = 0;
        if (h > MAX_FLANK_H) h = MAX_FLANK_H;

        if ((float)h >= s_fusion_flank_pk_l[k]) s_fusion_flank_pk_l[k] = (float)h;
        else s_fusion_flank_pk_l[k] *= 0.92f;

        if (h > 0) {
            int x = 6 + k * 3;
            SafeDraw::drawVLine(x, cy - h, 2 * h + 1);
            SafeDraw::drawVLine(x + 1, cy - h, 2 * h + 1);
        }

        int pkh = (int)(s_fusion_flank_pk_l[k] + 0.5f);
        if (pkh > h + 1 && pkh <= MAX_FLANK_H) {
            int x = 6 + k * 3;
            SafeDraw::drawPixel(x, cy - pkh);
            SafeDraw::drawPixel(x + 1, cy - pkh);
            SafeDraw::drawPixel(x, cy + pkh);
            SafeDraw::drawPixel(x + 1, cy + pkh);
        }
    }

    // 5. Right Flank Waves (Bass near center, Treble toward right edge)
    for (int k = 0; k < FUSION_FLANK_BARS; k++) {
        float norm_r = flank_r[k] / s_fusion_scale;
        if (norm_r > 1.0f) norm_r = 1.0f;
        int h = (int)(powf(norm_r, 0.60f) * MAX_FLANK_H);
        if (h < 0) h = 0;
        if (h > MAX_FLANK_H) h = MAX_FLANK_H;

        if ((float)h >= s_fusion_flank_pk_r[k]) s_fusion_flank_pk_r[k] = (float)h;
        else s_fusion_flank_pk_r[k] *= 0.92f;

        if (h > 0) {
            int x = 83 + k * 3;
            SafeDraw::drawVLine(x, cy - h, 2 * h + 1);
            SafeDraw::drawVLine(x + 1, cy - h, 2 * h + 1);
        }

        int pkh = (int)(s_fusion_flank_pk_r[k] + 0.5f);
        if (pkh > h + 1 && pkh <= MAX_FLANK_H) {
            int x = 83 + k * 3;
            SafeDraw::drawPixel(x, cy - pkh);
            SafeDraw::drawPixel(x + 1, cy - pkh);
            SafeDraw::drawPixel(x, cy + pkh);
            SafeDraw::drawPixel(x + 1, cy + pkh);
        }
    }

    // 6. Center Circles & Rotating Cyber Arcs
    SafeDraw::drawCircle(cx, cy, R_IN);
    for (int j = -4; j <= 4; j++) {
        float a1 = s_fusion_angle + j * 0.12f;
        float a2 = s_fusion_angle + 3.141592f + j * 0.12f;
        SafeDraw::drawPixel(cx + (int)(cosf(a1) * R_MID + 0.5f), cy + (int)(sinf(a1) * R_MID + 0.5f));
        SafeDraw::drawPixel(cx + (int)(cosf(a2) * R_MID + 0.5f), cy + (int)(sinf(a2) * R_MID + 0.5f));
    }

    // 7. Outer Radial Rays (Top & Bottom arcs)
    // Top rays: -150° to -30° (sweeping upward)
    for (int k = 0; k < FUSION_RAD_BARS; k++) {
        float norm = rad_top[k] / s_fusion_scale;
        if (norm > 1.0f) norm = 1.0f;
        float len = powf(norm, 0.60f) * (float)MAX_RAD_LEN;
        if (len < 0.0f) len = 0.0f;

        if (len >= s_fusion_pk_t[k]) s_fusion_pk_t[k] = len;
        else s_fusion_pk_t[k] *= 0.92f;

        float a = -2.61799f + (2.09439f / (FUSION_RAD_BARS + 1)) * (k + 1);
        float ca = cosf(a), sa = sinf(a);

        if (len >= 1.0f) {
            int x1 = cx + (int)(ca * (R_MID + 1) + 0.5f);
            int y1 = cy + (int)(sa * (R_MID + 1) + 0.5f);
            int x2 = cx + (int)(ca * (R_MID + 1 + len) + 0.5f);
            int y2 = cy + (int)(sa * (R_MID + 1 + len) + 0.5f);
            SafeDraw::drawLine(x1, y1, x2, y2);
        }

        if (s_fusion_pk_t[k] >= 1.0f) {
            int pk_r = R_MID + 2 + (int)(s_fusion_pk_t[k] + 0.5f);
            if (pk_r <= 31) {
                SafeDraw::drawPixel(cx + (int)(ca * pk_r + 0.5f), cy + (int)(sa * pk_r + 0.5f));
            }
        }
    }

    // Bottom rays: 30° to 150° (sweeping downward)
    for (int k = 0; k < FUSION_RAD_BARS; k++) {
        float norm = rad_bot[k] / s_fusion_scale;
        if (norm > 1.0f) norm = 1.0f;
        float len = powf(norm, 0.60f) * (float)MAX_RAD_LEN;
        if (len < 0.0f) len = 0.0f;

        if (len >= s_fusion_pk_b[k]) s_fusion_pk_b[k] = len;
        else s_fusion_pk_b[k] *= 0.92f;

        float a = 0.52359f + (2.09439f / (FUSION_RAD_BARS + 1)) * (k + 1);
        float ca = cosf(a), sa = sinf(a);

        if (len >= 1.0f) {
            int x1 = cx + (int)(ca * (R_MID + 1) + 0.5f);
            int y1 = cy + (int)(sa * (R_MID + 1) + 0.5f);
            int x2 = cx + (int)(ca * (R_MID + 1 + len) + 0.5f);
            int y2 = cy + (int)(sa * (R_MID + 1 + len) + 0.5f);
            SafeDraw::drawLine(x1, y1, x2, y2);
        }

        if (s_fusion_pk_b[k] >= 1.0f) {
            int pk_r = R_MID + 2 + (int)(s_fusion_pk_b[k] + 0.5f);
            if (pk_r <= 31) {
                SafeDraw::drawPixel(cx + (int)(ca * pk_r + 0.5f), cy + (int)(sa * pk_r + 0.5f));
            }
        }
    }

    // 8. 3D Spinning MVT Text at Center
    draw_spinning_mvt(cx, cy, s_fusion_angle);

    // 9. Channel labels
    SafeDraw::setFont(u8g2_font_04b_03_tr);
    SafeDraw::drawStr(1, 10, "L");
    SafeDraw::drawStr(123, 10, "R");
}
