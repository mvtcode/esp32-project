#include "effect_common.h"

// -----------------------------------------------------------------------
// Pre-computed per-frame audio analysis (shared by all effects)
// -----------------------------------------------------------------------
AudioBands g_frame_bands = {};

// Run FFT on stereo mono-mix and populate AudioBands.
// NOTE: This overwrites s_fft_real/imag. Effects that need per-bin FFT
// data for visualization must run their own FFT after this.
void audio_compute_bands(const int32_t *left, const int32_t *right, size_t n, AudioBands &out) {
    // 1. Fill FFT buffer with stereo mono mix
    for (size_t i = 0; i < n; i++) {
        s_fft_real[i] = (float)((left[i] + right[i]) / 2);
        s_fft_imag[i] = 0.0f;
    }

    // 2. FFT
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();

    // 3. Peak reference (avoid division by zero)
    float peak_ref = (float)s_peak_l;
    if (peak_ref < 1.0f) peak_ref = 1.0f;

    // 4. Bass: bins 1-4 (~0-500 Hz @ 16kHz/128pt)
    float bass_sum = 0.0f;
    for (int b = 1; b <= 4; b++) bass_sum += s_fft_real[b];
    out.bass = bass_sum / (4.0f * peak_ref);
    if (out.bass > 1.0f) out.bass = 1.0f;

    // 5. Mid: bins 5-16 (~500 Hz-2kHz)
    float mid_sum = 0.0f;
    for (int b = 5; b <= 16; b++) mid_sum += s_fft_real[b];
    out.mid = mid_sum / (12.0f * peak_ref);
    if (out.mid > 1.0f) out.mid = 1.0f;

    // 6. Treble: bins 17-40 (~2kHz-5kHz)
    float treble_sum = 0.0f;
    for (int b = 17; b <= 40; b++) treble_sum += s_fft_real[b];
    out.treble = treble_sum / (24.0f * peak_ref);
    if (out.treble > 1.0f) out.treble = 1.0f;

    // 7. RMS from raw samples
    int64_t sum_sq = 0;
    for (size_t i = 0; i < n; i++) {
        int32_t m = (left[i] + right[i]) / 2;
        sum_sq += (int64_t)m * m;
    }
    float rms_raw = sqrtf((float)sum_sq / (float)n);
    out.rms = rms_raw / peak_ref;
    if (out.rms > 1.0f) out.rms = 1.0f;
}

void draw_spinning_mvt(int cx, int cy, float angle) {
    float ca = cosf(angle);
    float sa = sinf(angle);

    // Vector line segments for 'M', 'V', 'T' relative to (0,0)
    // Coords designed to be centered and fit within R=8 px
    static const int8_t STROKES[][4] = {
        // 'M'
        {-7, -3, -7,  3},
        {-7, -3, -5,  0},
        {-5,  0, -3, -3},
        {-3, -3, -3,  3},
        // 'V'
        {-2, -3,  0,  3},
        { 0,  3,  2, -3},
        // 'T'
        { 3, -3,  7, -3},
        { 5, -3,  5,  3}
    };
    const int NUM_STROKES = 8;

    for (int i = 0; i < NUM_STROKES; i++) {
        float x1 = STROKES[i][0];
        float y1 = STROKES[i][1];
        float x2 = STROKES[i][2];
        float y2 = STROKES[i][3];

        // 2D Clockwise rotation: x' = x*cos - y*sin, y' = x*sin + y*cos
        int rx1 = cx + (int)(x1 * ca - y1 * sa + (x1 * ca - y1 * sa >= 0 ? 0.5f : -0.5f));
        int ry1 = cy + (int)(x1 * sa + y1 * ca + (x1 * sa + y1 * ca >= 0 ? 0.5f : -0.5f));
        int rx2 = cx + (int)(x2 * ca - y2 * sa + (x2 * ca - y2 * sa >= 0 ? 0.5f : -0.5f));
        int ry2 = cy + (int)(x2 * sa + y2 * ca + (x2 * sa + y2 * ca >= 0 ? 0.5f : -0.5f));

        SafeDraw::drawLine(rx1, ry1, rx2, ry2);
    }
}
