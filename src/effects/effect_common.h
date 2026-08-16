#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include <arduinoFFT.h>
#include <math.h>
#include "../display.h"
#include "safe_draw.h"

// -----------------------------------------------------------------------
// Math constants (avoid hard-coded 3.14159f everywhere)
// -----------------------------------------------------------------------
static constexpr float kPi    = 3.14159265f;
static constexpr float kTwoPi = 6.28318530f;
static constexpr float kHalfPi = 1.57079632f;

// -----------------------------------------------------------------------
// Shared resources for all visual effects
// -----------------------------------------------------------------------

// Shared U8g2 display instance
extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

// Shared AGC peak levels (computed per frame in display.cpp)
extern int32_t s_peak_l;
extern int32_t s_peak_r;

// Standard audio scaling constants across effects (Fixed scale for 1:1 INMP441 raw 16-bit audio)
#define FFT_MAG_FLOOR       25000.0f  // Standard FFT magnitude reference floor (scaled for N=128 FFT)
#define AUDIO_NOMINAL_PEAK  2500.0f   // Nominal full scale reference for VU meters and waveforms

// Shared FFT instance and buffers (reused across spectrum-based effects)
extern float s_fft_real[FRAME_SIZE];
extern float s_fft_imag[FRAME_SIZE];
extern ArduinoFFT<float> s_fft;

// Shared vector graphics helper: spinning MVT text (used in Fusion and Cyber modes)
void draw_spinning_mvt(int cx, int cy, float angle);

// -----------------------------------------------------------------------
// AudioBands: pre-computed per-frame audio analysis
// Updated once per frame by display.cpp before render() is called.
// Effects that only need scalar energy levels should read these instead
// of running their own FFT.
// -----------------------------------------------------------------------
struct AudioBands {
    float bass;    // [0..1] normalized bass energy   (FFT mono bins 1-4)
    float mid;     // [0..1] normalized mid energy    (FFT mono bins 5-16)
    float treble;  // [0..1] normalized treble energy (FFT mono bins 17-40)
    float rms;     // [0..1] overall RMS amplitude level
};

// Pre-computed audio analysis for the current frame.
// Updated by display.cpp each frame BEFORE render() is called.
extern AudioBands g_frame_bands;

// Run FFT on stereo mono-mix and populate an AudioBands struct.
void audio_compute_bands(const int32_t *left, const int32_t *right, size_t n, AudioBands &out);

// -----------------------------------------------------------------------
// Visual effect descriptor
// -----------------------------------------------------------------------
struct VisualEffect {
    const char *name;
    void (*render)(const int32_t *left, const int32_t *right, size_t n);
    void (*on_enter)();
    void (*on_exit)();
};
