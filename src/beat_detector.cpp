#include "beat_detector.h"
#include "log.h"

// -----------------------------------------------------------------------
// Global beat state (readable by effects)
// -----------------------------------------------------------------------
BeatInfo g_beat = { false, 0.0f, 0.0f, 0, 0.0f };

// -----------------------------------------------------------------------
// Internal state
// -----------------------------------------------------------------------

// 3-band energy history ring buffers
// Band 0 = sub-bass (bass dominant, kick drum)
// Band 1 = bass+mid (snare, bass guitar)
// Band 2 = mid (melody, general beat feel)
static float s_energy_history[3][BEAT_HISTORY_LEN];
static uint8_t s_hist_idx = 0;
static bool s_hist_full = false;

// Inter-beat interval tracking (up to 8 recent beats for BPM average)
#define BPM_IBI_COUNT 8
static uint32_t s_ibi_ms[BPM_IBI_COUNT];   // stored inter-beat intervals
static uint8_t  s_ibi_idx = 0;
static bool     s_ibi_full = false;
static uint32_t s_last_beat_ms = 0;

// BPM smoothing
static float s_bpm_smooth = 0.0f;
static float s_confidence = 0.0f;

// -----------------------------------------------------------------------
void beat_detector_init() {
    beat_detector_reset();
    LOG_I("Beat", "Beat detector initialized (threshold=%.2f, window=%d frames)", 
          BEAT_THRESHOLD, BEAT_HISTORY_LEN);
}

void beat_detector_reset() {
    memset(s_energy_history, 0, sizeof(s_energy_history));
    memset(s_ibi_ms, 0, sizeof(s_ibi_ms));
    s_hist_idx   = 0;
    s_ibi_idx    = 0;
    s_hist_full  = false;
    s_ibi_full   = false;
    s_last_beat_ms = 0;
    s_bpm_smooth = 0.0f;
    s_confidence = 0.0f;
    g_beat = { false, 0.0f, 0.0f, 0, 0.0f };
}

bool beat_detector_is_locked() {
    return s_ibi_full && s_confidence > 0.4f;
}

// -----------------------------------------------------------------------
// Compute rolling average energy for a band
// -----------------------------------------------------------------------
static float band_average(uint8_t band) {
    float sum = 0.0f;
    int count = s_hist_full ? BEAT_HISTORY_LEN : (int)s_hist_idx;
    if (count == 0) return 0.0f;
    for (int i = 0; i < count; i++) {
        sum += s_energy_history[band][i];
    }
    return sum / (float)count;
}

// -----------------------------------------------------------------------
// Estimate BPM from IBI buffer + compute confidence
// -----------------------------------------------------------------------
static float compute_bpm_and_confidence() {
    int count = s_ibi_full ? BPM_IBI_COUNT : (int)s_ibi_idx;
    if (count < 2) return 0.0f;

    // Average IBI
    float sum = 0.0f;
    for (int i = 0; i < count; i++) sum += (float)s_ibi_ms[i];
    float avg_ibi = sum / (float)count;

    float bpm = 60000.0f / avg_ibi;

    // Reject implausible BPM
    if (bpm < BEAT_BPM_MIN || bpm > BEAT_BPM_MAX) return 0.0f;

    // Confidence: 1 - (std_dev / avg_ibi), clamped to [0,1]
    // High confidence = IBIs are consistent (rhythmic music)
    float var = 0.0f;
    for (int i = 0; i < count; i++) {
        float d = (float)s_ibi_ms[i] - avg_ibi;
        var += d * d;
    }
    float std_dev = sqrtf(var / (float)count);
    float cv = std_dev / avg_ibi;  // coefficient of variation
    s_confidence = 1.0f - constrain(cv * 3.0f, 0.0f, 1.0f);

    return bpm;
}

// -----------------------------------------------------------------------
// Main update — call once per frame
// -----------------------------------------------------------------------
const BeatInfo& beat_detector_update(const AudioBands& bands) {
    // Reset beat_now each frame
    g_beat.beat_now = false;

    // Silence guard: don't process if there's no audio signal
    if (bands.rms < 0.01f) {
        // Store zeros in history to keep window fresh during silence
        s_energy_history[0][s_hist_idx] = 0.0f;
        s_energy_history[1][s_hist_idx] = 0.0f;
        s_energy_history[2][s_hist_idx] = 0.0f;
        s_hist_idx = (s_hist_idx + 1) % BEAT_HISTORY_LEN;
        if (s_hist_idx == 0) s_hist_full = true;
        return g_beat;
    }

    // Map AudioBands to our 3 detection bands:
    // Band 0: sub-bass (low end of bass — kick drum energy)
    // Band 1: bass     (mid bass — snare, bass guitar)
    // Band 2: mid      (general melodic energy)
    float cur[3] = {
        bands.bass * bands.bass,          // squared = more sensitive to transients
        (bands.bass * 0.4f + bands.mid * 0.6f),
        bands.mid
    };

    // Get rolling averages before updating history
    float avg[3] = {
        band_average(0),
        band_average(1),
        band_average(2)
    };

    // Update ring buffer
    for (int b = 0; b < 3; b++) {
        s_energy_history[b][s_hist_idx] = cur[b];
    }
    s_hist_idx = (s_hist_idx + 1) % BEAT_HISTORY_LEN;
    if (s_hist_idx == 0) s_hist_full = true;

    // Need at least half a window to detect reliably
    int filled = s_hist_full ? BEAT_HISTORY_LEN : (int)s_hist_idx;
    if (filled < BEAT_HISTORY_LEN / 2) return g_beat;

    // Beat detection: find which band has the strongest onset
    uint32_t now = millis();
    uint32_t dt = now - s_last_beat_ms;

    // Cooldown to prevent double-triggers
    if (dt < BEAT_MIN_INTERVAL_MS) return g_beat;

    float best_ratio = 0.0f;
    int   best_band  = -1;

    for (int b = 0; b < 3; b++) {
        if (avg[b] < 0.001f) continue;   // avoid division by near-zero average
        float ratio = cur[b] / avg[b];
        if (ratio > BEAT_THRESHOLD && ratio > best_ratio) {
            best_ratio = ratio;
            best_band  = b;
        }
    }

    if (best_band >= 0) {
        // Beat detected!
        g_beat.beat_now    = true;
        g_beat.band        = (uint8_t)best_band;
        g_beat.energy_ratio = best_ratio;

        // Record IBI
        if (s_last_beat_ms > 0 && dt >= BEAT_MIN_INTERVAL_MS && dt < 2000) {
            s_ibi_ms[s_ibi_idx] = dt;
            s_ibi_idx = (s_ibi_idx + 1) % BPM_IBI_COUNT;
            if (s_ibi_idx == 0) s_ibi_full = true;
        }
        s_last_beat_ms = now;

        // Compute BPM
        float raw_bpm = compute_bpm_and_confidence();
        if (raw_bpm > 0.0f) {
            // Smooth BPM with exponential moving average
            float alpha = s_ibi_full ? 0.15f : 0.35f;
            s_bpm_smooth = (s_bpm_smooth < 10.0f)
                ? raw_bpm
                : s_bpm_smooth * (1.0f - alpha) + raw_bpm * alpha;
        }

        g_beat.bpm        = s_bpm_smooth;
        g_beat.confidence = s_confidence;

        LOG_D("Beat", "BEAT! band=%d ratio=%.2f BPM=%.1f conf=%.2f",
              best_band, best_ratio, s_bpm_smooth, s_confidence);
    }

    return g_beat;
}
