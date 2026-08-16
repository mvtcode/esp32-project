#include "effects.h"

// -----------------------------------------------------------------------
// MODE 18 — MVT SPEAKER (Hi-Fi Bouncing Woofer & Shockwave Ripples)
// -----------------------------------------------------------------------
static float s_ripple_r[4] = {0.0f, 0.0f, 0.0f, 0.0f};

void effect_speaker_on_enter() {
    for (int i = 0; i < 4; i++) s_ripple_r[i] = 0.0f;
}

void effect_speaker_on_exit() {}

void effect_speaker_render(const int32_t *left, const int32_t *right, size_t n) {
    // 1. Audio analysis — use pre-computed frame bands
    float bass_norm = g_frame_bands.bass;
    float treble_norm = g_frame_bands.treble;

    // 2. Animate expanding shockwave sound ripples on bass
    for (int i = 0; i < 4; i++) {
        if (s_ripple_r[i] > 0.0f) {
            s_ripple_r[i] += 1.8f;
            if (s_ripple_r[i] > 36.0f) s_ripple_r[i] = 0.0f;
        }
    }
    if (bass_norm > 0.45f) {
        for (int i = 0; i < 4; i++) {
            if (s_ripple_r[i] == 0.0f) {
                s_ripple_r[i] = 16.0f;
                break;
            }
        }
    }

    // 3. Draw Left Speaker Cabinet (x=2..54, y=2..61)
    SafeDraw::drawRFrame(2, 2, 53, 60, 3);
    // Tweeter
    int tw_r = 4 + (int)(treble_norm * 3.0f);
    SafeDraw::drawCircle(28, 16, tw_r);
    SafeDraw::drawDisc(28, 16, 2);
    // Big Woofer
    int wf_r = 13 + (int)(bass_norm * 7.0f);
    SafeDraw::drawCircle(28, 42, wf_r);
    SafeDraw::drawCircle(28, 42, wf_r - 4 > 2 ? wf_r - 4 : 2);
    SafeDraw::drawDisc(28, 42, 4 + (int)(bass_norm * 3.0f));

    // 4. Draw Right Speaker Cabinet (x=73..125, y=2..61)
    SafeDraw::drawRFrame(73, 2, 53, 60, 3);
    // Tweeter
    SafeDraw::drawCircle(99, 16, tw_r);
    SafeDraw::drawDisc(99, 16, 2);
    // Big Woofer
    SafeDraw::drawCircle(99, 42, wf_r);
    SafeDraw::drawCircle(99, 42, wf_r - 4 > 2 ? wf_r - 4 : 2);
    SafeDraw::drawDisc(99, 42, 4 + (int)(bass_norm * 3.0f));

    // 5. Draw Shockwave Ripple Arcs radiating into center
    for (int i = 0; i < 4; i++) {
        if (s_ripple_r[i] > 0.0f) {
            int rip = (int)s_ripple_r[i];
            // Sound waves between speakers
            SafeDraw::drawCircle(28, 42, rip);
            SafeDraw::drawCircle(99, 42, rip);
        }
    }

    // 6. Middle Center Equalizer Column (x=58..70)
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(58, 8, "MVT");
    // SafeDraw::drawStr(58, 62, "BASS");
    int eq_h = (int)(bass_norm * 40.0f);
    for (int yb = 0; yb < eq_h; yb += 3) {
        SafeDraw::drawHLine(60, 52 - yb, 8);
    }
}
