#include "effects.h"

// -----------------------------------------------------------------------
// MODE 39 — WATERFALL (Stereo Butterfly Spectrogram Waterfall: Center Bass)
// -----------------------------------------------------------------------

#define WF_BINS_PER_CH 16
#define WF_COLS        32   // 16 Left (flank->center) + 16 Right (center->flank)
#define WF_ROWS        44

static uint8_t s_wf_history[WF_ROWS][WF_COLS]; // 2-bit intensity: 0..3
static int s_wf_head = 0;
static float s_wf_peak_l = FFT_MAG_FLOOR;
static float s_wf_peak_r = FFT_MAG_FLOOR;
static uint32_t s_last_row_time = 0;

void effect_waterfall_on_enter() {
    s_wf_head = 0;
    s_wf_peak_l = FFT_MAG_FLOOR;
    s_wf_peak_r = FFT_MAG_FLOOR;
    s_last_row_time = 0;
    for (int r = 0; r < WF_ROWS; r++) {
        for (int c = 0; c < WF_COLS; c++) {
            s_wf_history[r][c] = 0;
        }
    }
}

void effect_waterfall_on_exit() {}

void effect_waterfall_render(const int32_t *left, const int32_t *right, size_t n) {
    // 1. FFT for Left Channel
    for (size_t i = 0; i < n; i++) {
        s_fft_real[i] = (float)left[i];
        s_fft_imag[i] = 0.0f;
    }
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();

    static float mags_l[WF_BINS_PER_CH]; // static: avoid per-frame stack allocation
    float max_l = 0.0f;
    for (int b = 0; b < WF_BINS_PER_CH; b++) {
        int bin_idx = b + 1;
        float mag = s_fft_real[bin_idx];
        mags_l[b] = mag;
        if (mag > max_l) max_l = mag;
    }
    s_wf_peak_l = max_l > s_wf_peak_l ? max_l : (s_wf_peak_l * 0.94f);
    if (s_wf_peak_l < FFT_MAG_FLOOR) s_wf_peak_l = FFT_MAG_FLOOR;

    // 2. FFT for Right Channel
    for (size_t i = 0; i < n; i++) {
        s_fft_real[i] = (float)right[i];
        s_fft_imag[i] = 0.0f;
    }
    s_fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    s_fft.compute(FFTDirection::Forward);
    s_fft.complexToMagnitude();

    static float mags_r[WF_BINS_PER_CH]; // static: avoid per-frame stack allocation
    float max_r = 0.0f;
    for (int b = 0; b < WF_BINS_PER_CH; b++) {
        int bin_idx = b + 1;
        float mag = s_fft_real[bin_idx];
        mags_r[b] = mag;
        if (mag > max_r) max_r = mag;
    }
    s_wf_peak_r = max_r > s_wf_peak_r ? max_r : (s_wf_peak_r * 0.94f);
    if (s_wf_peak_r < FFT_MAG_FLOOR) s_wf_peak_r = FFT_MAG_FLOOR;

    // 3. Map into 32 columns (Left: col 0..15 Treble->Bass, Right: col 16..31 Bass->Treble)
    static float col_mags[WF_COLS]; // static: avoid per-frame stack allocation
    for (int c = 0; c < 16; c++) {
        int bin = 15 - c; // c=0 is Treble (bin 15), c=15 is Bass (bin 0)
        col_mags[c] = mags_l[bin] / s_wf_peak_l;
    }
    for (int c = 16; c < 32; c++) {
        int bin = c - 16; // c=16 is Bass (bin 0), c=31 is Treble (bin 15)
        col_mags[c] = mags_r[bin] / s_wf_peak_r;
    }

    // 4. Shift new row every 35ms (~28 FPS waterfall scroll)
    uint32_t now = millis();
    if (now - s_last_row_time >= 35) {
        s_last_row_time = now;
        s_wf_head = (s_wf_head - 1 + WF_ROWS) % WF_ROWS;

        for (int c = 0; c < WF_COLS; c++) {
            float norm = col_mags[c];
            uint8_t level = 0;
            if (norm > 0.65f) level = 3;
            else if (norm > 0.35f) level = 2;
            else if (norm > 0.12f) level = 1;
            s_wf_history[s_wf_head][c] = level;
        }
    }

    // 5. Draw Top Live Instant Spectrum Bar (y: 0..14)
    SafeDraw::drawHLine(0, 15, 128);
    // Center divider dot line between L & R
    SafeDraw::drawPixel(63, 14);
    SafeDraw::drawPixel(63, 15);

    for (int c = 0; c < WF_COLS; c++) {
        int bar_h = (int)(col_mags[c] * 14.0f);
        if (bar_h > 14) bar_h = 14;
        if (bar_h > 0) {
            int x = c * 4;
            SafeDraw::drawBox(x, 14 - bar_h, 3, bar_h);
        }
    }

    // 6. Render Waterfall History Rows (y: 17 .. 17 + WF_ROWS)
    const int start_y = 17;
    for (int r = 0; r < WF_ROWS; r++) {
        int hist_idx = (s_wf_head + r) % WF_ROWS;
        int y = start_y + r;
        if (y >= 64) break;

        for (int c = 0; c < WF_COLS; c++) {
            uint8_t lvl = s_wf_history[hist_idx][c];
            int x = c * 4;

            if (lvl == 3) {
                // Dense fill (3 pixels wide)
                SafeDraw::drawHLine(x, y, 3);
            } else if (lvl == 2) {
                // Medium dither (2 pixels alternating)
                if ((x + y) & 1) {
                    SafeDraw::drawPixel(x, y);
                    SafeDraw::drawPixel(x + 2, y);
                } else {
                    SafeDraw::drawPixel(x + 1, y);
                }
            } else if (lvl == 1) {
                // Sparse single pixel
                if ((y % 2 == 0) && ((c) % 2 == (y / 2) % 2)) {
                    SafeDraw::drawPixel(x + 1, y);
                }
            }
        }
    }

    // 7. Channel indicator labels
    SafeDraw::setFont(u8g2_font_04b_03_tr);
    SafeDraw::drawStr(1, 6, "L");
    SafeDraw::drawStr(123, 6, "R");
    // SafeDraw::drawStr(53, 6, "BASS");
}

