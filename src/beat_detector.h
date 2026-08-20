#pragma once
#include <Arduino.h>
#include "effects/effect_common.h"

/**
 * beat_detector.h — Realtime beat detection + BPM estimation
 *
 * Algorithm: Sub-band energy onset detection
 *   - Computes energy in 3 frequency bands each frame
 *   - Detects beat when current energy exceeds BEAT_THRESHOLD * sliding average
 *   - Estimates BPM from inter-beat intervals (IBI)
 *
 * Usage:
 *   beat_detector_init();                              // once at startup
 *   BeatInfo b = beat_detector_update(g_frame_bands); // every frame
 *   if (b.beat_now) { ... }
 */

// BPM valid range filter
#define BEAT_BPM_MIN   50
#define BEAT_BPM_MAX   220

// Energy history window (number of frames for average, ~43 frames @ ~43fps = ~1 second)
#define BEAT_HISTORY_LEN  48

// Onset threshold: beat fires when energy > BEAT_THRESHOLD * rolling average
#define BEAT_THRESHOLD    1.45f

// Minimum time between beats (ms) to avoid double-triggers  (60000/220BPM ~ 273ms)
#define BEAT_MIN_INTERVAL_MS 260

struct BeatInfo {
    bool    beat_now;      // true only in the exact frame a beat is detected
    float   bpm;           // estimated BPM (0 = not enough data yet)
    float   confidence;    // 0.0-1.0 — how rhythmically consistent the beats are
    uint8_t band;          // which band triggered: 0=sub-bass, 1=bass, 2=mid
    float   energy_ratio;  // current/average energy at detection moment
};

// Global beat state — readable by effects for animations
extern BeatInfo g_beat;

void beat_detector_init();

/**
 * Update beat detector with current frame's audio bands.
 * Call once per frame AFTER audio_compute_bands().
 * Returns updated BeatInfo (same as g_beat).
 */
const BeatInfo& beat_detector_update(const AudioBands& bands);

/** Returns true if beat system has enough data for reliable BPM. */
bool beat_detector_is_locked();

/** Reset all internal state (call when switching audio modes). */
void beat_detector_reset();
