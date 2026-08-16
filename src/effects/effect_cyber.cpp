#include "effects.h"

// -----------------------------------------------------------------------
// MODE 9 — MVT CYBER (Cyber Pulse: Inward horizontal bars + rotating MVT + tech radar rings)
// -----------------------------------------------------------------------
#define CYBER_BARS 10
static float s_cyber_angle = 0.0f;
static float s_cyber_angle_mid = 0.0f;
static float s_cyber_angle_out = 0.0f;
static float s_cyber_scale = FFT_MAG_FLOOR;
static float s_cyber_pk_l[CYBER_BARS] = {0};
static float s_cyber_pk_r[CYBER_BARS] = {0};

void effect_cyber_on_enter() {
    s_cyber_scale = FFT_MAG_FLOOR;
    s_cyber_angle = 0.0f;
    s_cyber_angle_mid = 0.0f;
    s_cyber_angle_out = 0.0f;
    memset(s_cyber_pk_l, 0, sizeof(s_cyber_pk_l));
    memset(s_cyber_pk_r, 0, sizeof(s_cyber_pk_r));
}

void effect_cyber_on_exit() {
    s_cyber_scale = FFT_MAG_FLOOR;
    s_cyber_angle = 0.0f;
    s_cyber_angle_mid = 0.0f;
    s_cyber_angle_out = 0.0f;
    memset(s_cyber_pk_l, 0, sizeof(s_cyber_pk_l));
    memset(s_cyber_pk_r, 0, sizeof(s_cyber_pk_r));
}

void effect_cyber_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 64;
    const int cy = 31;
    const int R_IN = 13;
    const int MAX_BAR_W = 26;

    float bands_l[CYBER_BARS] = {0};
    float bands_r[CYBER_BARS] = {0};

    // 1. FFT on Left Channel
    for (size_t i = 0; i < n; i++) {
        s_fft_real[i] = (float)left[i];
        s_fft_imag[i] = 0.0f;
    }
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();

    float max_mag = 0.0f;
    for (int k = 0; k < CYBER_BARS; k++) {
        float sum = 0.0f;
        int start_bin = 1 + k * 4;
        for (int b = 0; b < 4 && (start_bin + b) < (int)(n / 2); b++) {
            sum += s_fft_real[start_bin + b];
        }
        bands_l[k] = sum / 4.0f;
        if (bands_l[k] > max_mag) max_mag = bands_l[k];
    }

    // 2. FFT on Right Channel
    for (size_t i = 0; i < n; i++) {
        s_fft_real[i] = (float)right[i];
        s_fft_imag[i] = 0.0f;
    }
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();

    for (int k = 0; k < CYBER_BARS; k++) {
        float sum = 0.0f;
        int start_bin = 1 + k * 4;
        for (int b = 0; b < 4 && (start_bin + b) < (int)(n / 2); b++) {
            sum += s_fft_real[start_bin + b];
        }
        bands_r[k] = sum / 4.0f;
        if (bands_r[k] > max_mag) max_mag = bands_r[k];
    }

    // Dynamic AGC
    s_cyber_scale = max_mag > s_cyber_scale ? max_mag : (s_cyber_scale * 0.95f);
    if (s_cyber_scale < FFT_MAG_FLOOR) s_cyber_scale = FFT_MAG_FLOOR;

    // Clockwise rotation: 10s per revolution (2*PI / 10s = 0.6283 rad/s)
    static uint32_t s_last_cyber_ts = 0;
    uint32_t now = millis();
    float dt = (now - s_last_cyber_ts) * 0.001f;
    if (dt > 0.1f || dt < 0.001f) dt = 0.035f;
    s_last_cyber_ts = now;

    float bass_energy = (bands_l[0] + bands_r[0]) / (2.0f * s_cyber_scale);
    float rot_rate = (6.2831853f / 10.0f) + bass_energy * 0.35f;

    // Independent smooth continuous angle accumulators
    s_cyber_angle += rot_rate * dt;
    if (s_cyber_angle >= 6.2831853f) s_cyber_angle -= 6.2831853f;

    s_cyber_angle_mid -= (rot_rate * 1.2f) * dt;
    if (s_cyber_angle_mid <= -6.2831853f) s_cyber_angle_mid += 6.2831853f;

    s_cyber_angle_out += (rot_rate * 0.8f) * dt;
    if (s_cyber_angle_out >= 6.2831853f) s_cyber_angle_out -= 6.2831853f;

    // Frequency mapping: Bass in the center bars (i=4,5), Treble on outer bars (i=0,9)
    static const uint8_t BAND_MAP[CYBER_BARS] = {8, 6, 4, 2, 0, 1, 3, 5, 7, 9};

    // 3. Left Inward Horizontal Bars (shoot from Left X=2 inward to center)
    for (int i = 0; i < CYBER_BARS; i++) {
        int yi = 8 + i * 5;
        int band = BAND_MAP[i];
        float norm = bands_l[band] / s_cyber_scale;
        if (norm > 1.0f) norm = 1.0f;
        int len = (int)(powf(norm, 0.60f) * MAX_BAR_W);
        if (len < 0) len = 0;
        if (len > MAX_BAR_W) len = MAX_BAR_W;

        if ((float)len >= s_cyber_pk_l[i]) s_cyber_pk_l[i] = (float)len;
        else s_cyber_pk_l[i] *= 0.93f;

        if (len > 0) {
            SafeDraw::drawFrame(2, yi, len, 3);
        }

        int pkw = (int)(s_cyber_pk_l[i] + 0.5f);
        if (s_cyber_pk_l[i] >= 2.0f && pkw > len + 1 && pkw <= MAX_BAR_W + 4) {
            SafeDraw::drawVLine(2 + pkw, yi, 3);
        }
    }

    // 4. Right Inward Horizontal Bars (shoot from Right X=125 inward to center)
    for (int i = 0; i < CYBER_BARS; i++) {
        int yi = 8 + i * 5;
        int band = BAND_MAP[i];
        float norm = bands_r[band] / s_cyber_scale;
        if (norm > 1.0f) norm = 1.0f;
        int len = (int)(powf(norm, 0.60f) * MAX_BAR_W);
        if (len < 0) len = 0;
        if (len > MAX_BAR_W) len = MAX_BAR_W;

        if ((float)len >= s_cyber_pk_r[i]) s_cyber_pk_r[i] = (float)len;
        else s_cyber_pk_r[i] *= 0.93f;

        if (len > 0) {
            SafeDraw::drawFrame(126 - len, yi, len, 3);
        }

        int pkw = (int)(s_cyber_pk_r[i] + 0.5f);
        if (s_cyber_pk_r[i] >= 2.0f && pkw > len + 1 && pkw <= MAX_BAR_W + 4) {
            SafeDraw::drawVLine(125 - pkw, yi, 3);
        }
    }

    // 5. Central Circle & Concentric Cyber Radar Rings
    SafeDraw::drawCircle(cx, cy, R_IN);

    // Inner Dotted Radar Track (R = 16)
    for (int d = 0; d < 16; d++) {
        float a = s_cyber_angle + d * (6.2831853f / 16.0f);
        SafeDraw::drawPixel(cx + (int)(cosf(a) * 16 + 0.5f), cy + (int)(sinf(a) * 16 + 0.5f));
    }

    // Middle Segmented Cyber Arcs with Radial Ticks (R = 19)
    for (int seg = 0; seg < 3; seg++) {
        float base_a = s_cyber_angle_mid + seg * (6.2831853f / 3.0f);
        for (int j = -3; j <= 3; j++) {
            float a = base_a + j * 0.09f;
            int px = cx + (int)(cosf(a) * 19 + 0.5f);
            int py = cy + (int)(sinf(a) * 19 + 0.5f);
            SafeDraw::drawPixel(px, py);
        }
        // Radial tick mark at segment center
        int tx1 = cx + (int)(cosf(base_a) * 18 + 0.5f);
        int ty1 = cy + (int)(sinf(base_a) * 18 + 0.5f);
        int tx2 = cx + (int)(cosf(base_a) * 21 + 0.5f);
        int ty2 = cy + (int)(sinf(base_a) * 21 + 0.5f);
        SafeDraw::drawLine(tx1, ty1, tx2, ty2);
    }

    // Outer Tech Dashed Arc Track (R = 23)
    for (int seg = 0; seg < 4; seg++) {
        float base_a = s_cyber_angle_out + seg * (6.2831853f / 4.0f);
        for (int j = -2; j <= 2; j++) {
            float a = base_a + j * 0.08f;
            SafeDraw::drawPixel(cx + (int)(cosf(a) * 23 + 0.5f), cy + (int)(sinf(a) * 23 + 0.5f));
        }
    }

    // 6. Clockwise Rotating "MVT" Text inside central circle
    draw_spinning_mvt(cx, cy, s_cyber_angle);

    // 7. Channel labels
    SafeDraw::setFont(u8g2_font_04b_03_tr);
    SafeDraw::drawStr(1, 6, "L");
    SafeDraw::drawStr(123, 6, "R");
}
