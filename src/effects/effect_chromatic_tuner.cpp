#include "effects.h"

// -----------------------------------------------------------------------
// MODE 67 — CHROMATIC TUNER
// Pitch detection via autocorrelation on left channel.
// Displays: note name + octave (large), cents deviation bar, frequency.
//
// Algorithm: Autocorrelation pitch detection (YIN-simplified)
//   SAMPLE_RATE=16000, FRAME_SIZE=128 → detects 125Hz–8000Hz
//   Period resolution: ±1 sample → ±6–12 Hz at mid range → ±25-50 cents
//   Suitable for: voice, guitar (open strings), wind instruments
// -----------------------------------------------------------------------

// Note names (12-tone equal temperament, A=0 → G#=11)
static const char *NOTE_NAMES[] = {
    "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"
};

// Reference frequency for A4 (standard tuning)
static const float A4_FREQ = 440.0f;

// Display smoothing
static float    s_freq_smooth  = 0.0f;
static float    s_cents_smooth = 0.0f;
static int      s_note_idx     = -1;    // -1 = no signal
static int      s_octave       = 4;
static uint32_t s_last_detect  = 0;     // last time a pitch was detected
static float    s_confidence   = 0.0f;  // autocorrelation confidence

// ── Autocorrelation pitch detection ─────────────────────────────────────
// Returns detected frequency in Hz, or 0 if no reliable pitch found.
static float detect_pitch(const int32_t *buf, size_t n) {
    // Minimum/maximum lag for valid pitch range (Hz = SAMPLE_RATE / lag)
    // Min lag → max freq: 16000/16 = 1000 Hz  (cover to ~2kHz: lag=8)
    // Max lag → min freq: 16000/80 = 200 Hz   (below guitar low E 82Hz, needs larger buffer)
    const int LAG_MIN = 8;    // ~2000 Hz max
    const int LAG_MAX = 100;  // ~160 Hz min (covers guitar high strings well)

    // Check signal level — skip if silence
    float sum_sq = 0.0f;
    for (size_t i = 0; i < n; i++) {
        float v = (float)buf[i] / AUDIO_NOMINAL_PEAK;
        sum_sq += v * v;
    }
    float rms = sqrtf(sum_sq / (float)n);
    if (rms < 0.04f) return 0.0f;   // silence threshold

    // Compute normalized autocorrelation r[k]
    // r[k] = sum(x[i]*x[i+k]) / sum(x[i]^2)
    // We use the first half of the buffer to allow lag lookup
    int valid_n = (int)n - LAG_MAX;
    if (valid_n <= 0) return 0.0f;

    // r[0] = self-correlation (energy)
    double r0 = 0.0;
    for (int i = 0; i < valid_n; i++) {
        double v = (double)buf[i];
        r0 += v * v;
    }
    if (r0 < 1.0) return 0.0f;

    // Find lag with highest normalized correlation (first peak after dip)
    float best_r   = 0.0f;
    int   best_lag = 0;
    bool  in_dip   = false;  // first find the dip, then look for the peak

    for (int k = LAG_MIN; k <= LAG_MAX; k++) {
        double rk = 0.0;
        for (int i = 0; i < valid_n; i++) {
            rk += (double)buf[i] * (double)buf[i + k];
        }
        float norm_r = (float)(rk / r0);

        // Track when we enter the first dip (r drops below 0.3)
        if (!in_dip && norm_r < 0.3f) in_dip = true;

        // After the dip, find first prominent peak
        if (in_dip && norm_r > best_r && norm_r > 0.45f) {
            best_r   = norm_r;
            best_lag = k;
        }
        // Stop searching after we've found a peak and it starts declining
        if (in_dip && best_lag > 0 && norm_r < best_r * 0.85f) break;
    }

    if (best_lag == 0 || best_r < 0.45f) return 0.0f;

    // Parabolic interpolation for sub-sample lag accuracy
    // Improves accuracy from ±1 sample to ~±0.1 sample
    float lag = (float)best_lag;
    if (best_lag > LAG_MIN && best_lag < LAG_MAX) {
        // Compute r at lag-1 and lag+1
        double r_prev = 0.0, r_next = 0.0;
        int k_prev = best_lag - 1, k_next = best_lag + 1;
        for (int i = 0; i < valid_n; i++) {
            r_prev += (double)buf[i] * (double)buf[i + k_prev];
            r_next += (double)buf[i] * (double)buf[i + k_next];
        }
        float rp = (float)(r_prev / r0);
        float rc = best_r;
        float rn = (float)(r_next / r0);
        // Parabolic peak: shift = 0.5 * (rp - rn) / (rp - 2*rc + rn)
        float denom = rp - 2.0f * rc + rn;
        if (fabsf(denom) > 0.001f) {
            lag += 0.5f * (rp - rn) / denom;
        }
    }

    s_confidence = best_r;
    return (float)SAMPLE_RATE / lag;
}

// ── Map frequency to MIDI note number and cents deviation ───────────────
static void freq_to_note(float freq, int &note_idx, int &octave, float &cents) {
    // MIDI note: A4 = 69
    float midi = 12.0f * log2f(freq / A4_FREQ) + 69.0f;
    int   midi_round = (int)(midi + 0.5f);

    // note_idx: 0=C, 1=C#, ..., 11=B  (standard MIDI mapping)
    // Our NOTE_NAMES start at A=0, so remap:
    // MIDI note 69 = A4, note 70 = A#4, 71=B4, 72=C5...
    // name_idx = (midi_round - 69 + 120) % 12  maps to A=0
    int name_raw = (midi_round - 69 + 1200) % 12;
    note_idx = name_raw;

    // Octave: MIDI 60 = C4, so octave = (midi_round / 12) - 1
    // But our note numbering starts at A, so C is at offset +3 from A
    // Simplest: octave = (midi_round - 12) / 12  (MIDI convention: C-1 = 0)
    octave = (midi_round / 12) - 1;

    // Cents: deviation from nearest semitone (−50 to +50)
    cents = (midi - (float)midi_round) * 100.0f;
}

void effect_chromatic_tuner_on_enter() {
    s_freq_smooth  = 0.0f;
    s_cents_smooth = 0.0f;
    s_note_idx     = -1;
    s_confidence   = 0.0f;
    s_last_detect  = 0;
}

void effect_chromatic_tuner_on_exit() {
    s_freq_smooth  = 0.0f;
}

void effect_chromatic_tuner_render(const int32_t *left, const int32_t * /*right*/, size_t n) {
    uint32_t now = millis();

    // ── Run pitch detection on left channel ─────────────────────────────
    float raw_freq = detect_pitch(left, n);

    if (raw_freq > 0.0f) {
        // Valid pitch detected — smooth it
        float alpha = (s_freq_smooth < 10.0f) ? 1.0f : 0.25f;
        s_freq_smooth = s_freq_smooth * (1.0f - alpha) + raw_freq * alpha;
        s_last_detect = now;

        // Map to note
        float raw_cents;
        freq_to_note(s_freq_smooth, s_note_idx, s_octave, raw_cents);
        s_cents_smooth = s_cents_smooth * 0.7f + raw_cents * 0.3f;

    } else if (now - s_last_detect > 800) {
        // No pitch for >800ms → clear display
        s_note_idx    = -1;
        s_freq_smooth = 0.0f;
        s_confidence  = 0.0f;
    }

    // ════════════════════════════════════════════════════════════════════
    // DISPLAY LAYOUT
    //
    //  ┌──────────────────────────────────────┐
    //  │             TUNER                    │  ← title (tiny)
    //  │                                      │
    //  │              A                       │  ← note name (very large)
    //  │              4                       │  ← octave (small, below)
    //  │                                      │
    //  │  ◄───────────┼────────────►          │  ← cents bar (−50..0..+50)
    //  │              ▲                       │
    //  │           −8 cents                   │  ← cents value text
    //  │          440.0 Hz                    │  ← frequency
    //  └──────────────────────────────────────┘
    // ════════════════════════════════════════════════════════════════════

    // Title
    SafeDraw::setFont(u8g2_font_04b_03_tr);
    SafeDraw::drawStr(50, 6, "TUNER");

    if (s_note_idx < 0) {
        // ── No signal ────────────────────────────────────────────────────
        SafeDraw::setFont(u8g2_font_7x14B_tf);
        SafeDraw::drawStr(44, 35, "---");
        SafeDraw::setFont(u8g2_font_04b_03_tr);
        SafeDraw::drawStr(35, 50, "No Signal");
        SafeDraw::drawStr(35, 58, "Play a note");
        return;
    }

    // ── Note name (large center) ─────────────────────────────────────────
    const char *note_str = NOTE_NAMES[s_note_idx];
    bool is_sharp = (note_str[1] == '#');

    SafeDraw::setFont(u8g2_font_inr30_mf);   // very large font
    int nw = SafeDraw::getStrWidth(note_str[0] == 'A' ? "A" : note_str);
    // Draw just the letter (large)
    char letter[2] = { note_str[0], '\0' };
    SafeDraw::drawStr(32 - nw / 2, 42, letter);

    // Sharp symbol (if needed) — smaller, superscript style
    if (is_sharp) {
        SafeDraw::setFont(u8g2_font_7x14B_tf);
        SafeDraw::drawStr(32 - nw / 2 + 18, 24, "#");
    }

    // Octave number
    char oct_str[3];
    snprintf(oct_str, sizeof(oct_str), "%d", s_octave);
    SafeDraw::setFont(u8g2_font_6x10_tf);
    SafeDraw::drawStr(32 - nw / 2 + 18, 42, oct_str);

    // ── Cents bar ────────────────────────────────────────────────────────
    // Bar spans from x=70 to x=126, center at x=98
    const int BAR_CX = 98;
    const int BAR_Y  = 28;
    const int BAR_W  = 54;  // total width (±27px = ±50 cents)

    // Bar outline
    SafeDraw::drawHLine(BAR_CX - BAR_W/2, BAR_Y, BAR_W);

    // End tick marks
    SafeDraw::drawVLine(BAR_CX - BAR_W/2, BAR_Y - 3, 7);
    SafeDraw::drawVLine(BAR_CX,           BAR_Y - 3, 7);  // center (in-tune)
    SafeDraw::drawVLine(BAR_CX + BAR_W/2, BAR_Y - 3, 7);

    // Center label "♩" indicator (in-tune zone, ±5 cents = ±2.7px)
    SafeDraw::drawBox(BAR_CX - 2, BAR_Y - 1, 4, 3);

    // Needle position (cents → pixels)
    float cents_clamped = constrain(s_cents_smooth, -50.0f, 50.0f);
    int needle_x = BAR_CX + (int)(cents_clamped * (float)(BAR_W / 2) / 50.0f);
    needle_x = constrain(needle_x, BAR_CX - BAR_W/2 + 1, BAR_CX + BAR_W/2 - 1);

    // Draw needle (triangle pointing up)
    SafeDraw::drawVLine(needle_x, BAR_Y - 6, 9);
    SafeDraw::drawHLine(needle_x - 2, BAR_Y - 6, 5);

    // In-tune indicator: flash the center box when within ±8 cents
    bool in_tune = fabsf(s_cents_smooth) < 8.0f;
    if (in_tune) {
        // Double border to indicate "locked"
        SafeDraw::drawFrame(needle_x - 3, BAR_Y - 7, 7, 11);
    }

    // Cents value text
    SafeDraw::setFont(u8g2_font_04b_03_tr);
    char cents_str[10];
    snprintf(cents_str, sizeof(cents_str), "%+.0f ct", s_cents_smooth);
    int cw = SafeDraw::getStrWidth(cents_str);
    SafeDraw::drawStr(BAR_CX - cw / 2, BAR_Y + 10, cents_str);

    // ── Frequency display ─────────────────────────────────────────────────
    char freq_str[12];
    if (s_freq_smooth >= 1000.0f) {
        snprintf(freq_str, sizeof(freq_str), "%.3f kHz", s_freq_smooth / 1000.0f);
    } else {
        snprintf(freq_str, sizeof(freq_str), "%.1f Hz", s_freq_smooth);
    }
    int fw = SafeDraw::getStrWidth(freq_str);
    SafeDraw::drawStr(BAR_CX - fw / 2, BAR_Y + 18, freq_str);

    // ── Confidence bar (shows how pure/stable the pitch is) ──────────────
    // Small dot row at very bottom right
    int conf_w = (int)(s_confidence * 30.0f);
    if (conf_w > 0) {
        SafeDraw::drawHLine(BAR_CX - 15, 58, conf_w);
        SafeDraw::drawHLine(BAR_CX - 15, 59, conf_w);
    }
    SafeDraw::setFont(u8g2_font_04b_03_tr);
    SafeDraw::drawStr(70, 58, "STB");  // stability label

    // ── Left panel: VU bar (shows signal level) ───────────────────────────
    // Thin vertical bar on left edge to indicate input level
    float vu = g_frame_bands.rms;
    if (vu > 1.0f) vu = 1.0f;
    int vu_h = (int)(vu * 50.0f);
    SafeDraw::drawFrame(2, 7, 6, 52);
    if (vu_h > 0) SafeDraw::drawBox(3, 7 + (52 - vu_h), 4, vu_h);

    // Input level label
    SafeDraw::setFont(u8g2_font_04b_03_tr);
    SafeDraw::drawStr(1, 63, "IN");
}
