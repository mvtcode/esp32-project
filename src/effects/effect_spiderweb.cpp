#include "effects.h"

// -----------------------------------------------------------------------
// MODE 20 — MVT SPIDERWEB (Flat 2D Clockwise Rotating & Audio-Vibrating Web)
// -----------------------------------------------------------------------

static uint32_t s_web_start_ms = 0;
static float    s_web_vib_time  = 0.0f;

void effect_spiderweb_on_enter() {
    s_web_start_ms = millis();
    s_web_vib_time  = 0.0f;
}

void effect_spiderweb_on_exit() {}

void effect_spiderweb_render(const int32_t *left, const int32_t *right, size_t n) {
    const int cx = 64;
    const int cy = 31;

    // 1. FFT Frequency Analysis
    // Note: s_fft_real already contains mono-mix magnitudes computed by audio_compute_bands in display.cpp

    float peak_ref = (float)(s_peak_l > 100 ? s_peak_l : 100);

    // Bass energy (bins 1..3)
    float bass = (s_fft_real[1] + s_fft_real[2] + s_fft_real[3]) / (3.0f * peak_ref);
    if (bass > 1.0f) bass = 1.0f;

    // Mid frequency energy (bins 4..8)
    float mids = (s_fft_real[4] + s_fft_real[6] + s_fft_real[8]) / (3.0f * peak_ref);
    if (mids > 1.0f) mids = 1.0f;

    // High frequency energy (bins 9..14)
    float treble = (s_fft_real[9] + s_fft_real[11] + s_fft_real[13]) / (3.0f * peak_ref);
    if (treble > 1.0f) treble = 1.0f;

    // Smooth Clockwise Rotation: Exactly 1 full revolution (360 deg) every 20 seconds (20,000 ms)
    uint32_t elapsed = millis() - s_web_start_ms;
    float s_web_rot_angle = (float)(elapsed % 20000) * (6.2831853f / 20000.0f);

    // Audio vibration timer (accelerates with beat intensity)
    s_web_vib_time += 0.20f + bass * 0.40f + treble * 0.30f;

    // 2. 2D Spiderweb Structure: 12 Spokes x 6 Concentric Rings
    const int SPOKES = 12;
    const int RINGS  = 6;

    static int node_x[12][7]; // static: SPOKES=12, RINGS+1=7, avoid stack allocation
    static int node_y[12][7]; // static: SPOKES=12, RINGS+1=7, avoid stack allocation

    // Center Hub (r = 0)
    for (int k = 0; k < SPOKES; k++) {
        node_x[k][0] = cx;
        node_y[k][0] = cy;
    }

    // Compute 2D node positions with audio reactive vibration
    for (int k = 0; k < SPOKES; k++) {
        float spoke_angle = s_web_rot_angle + (float)k * (6.2831853f / (float)SPOKES);
        float cos_a = cosf(spoke_angle);
        float sin_a = sinf(spoke_angle);

        // FFT energy for this specific spoke
        int bin = 1 + (k * 2) % 24;
        float spoke_fft = s_fft_real[bin] / peak_ref;
        if (spoke_fft > 1.0f) spoke_fft = 1.0f;

        for (int r = 1; r <= RINGS; r++) {
            float rf = (float)r;

            // Base radius of concentric ring
            float base_r = rf * 4.6f;

            // Bass dynamic expansion
            float pulse_r = bass * 3.5f * (rf / (float)RINGS);

            // Harmonic acoustic silk vibration along spoke
            float vib = sinf(s_web_vib_time * 4.0f + (float)k * 1.8f + rf * 1.2f) * (spoke_fft * 3.0f + bass * 1.8f);

            float total_r = base_r + pulse_r + vib;
            if (total_r < 2.0f) total_r = 2.0f;

            // Aspect ratio scaling for 128x64 display (fills width nicely while fitting height)
            float x_scale = 1.35f;
            float y_scale = 0.95f;

            node_x[k][r] = cx + (int)(total_r * x_scale * cos_a + 0.5f);
            node_y[k][r] = cy + (int)(total_r * y_scale * sin_a + 0.5f);
        }
    }

    // 3. Draw 12 Radial Spokes (from Center Hub to Outer Rim)
    for (int k = 0; k < SPOKES; k++) {
        for (int r = 0; r < RINGS; r++) {
            SafeDraw::drawLine(node_x[k][r], node_y[k][r], node_x[k][r + 1], node_y[k][r + 1]);
        }
    }

    // 4. Draw Concentric Silk Rings with Inward Sag & Audio Strand Vibration
    for (int r = 1; r <= RINGS; r++) {
        for (int k = 0; k < SPOKES; k++) {
            int next_k = (k + 1) % SPOKES;

            // Midpoint angle between adjacent spokes
            float mid_angle = s_web_rot_angle + ((float)k + 0.5f) * (6.2831853f / (float)SPOKES);
            float mid_cos = cosf(mid_angle);
            float mid_sin = sinf(mid_angle);

            // Inward catenary sag (natural curved spiderweb strand geometry)
            float rf = (float)r;
            float base_mid_r = (rf * 4.6f + bass * 3.5f * (rf / (float)RINGS)) * 0.88f;

            // High-frequency strand string vibration on audio impact
            int bin = 1 + (k * 2) % 24;
            float strand_fft = s_fft_real[bin] / peak_ref;
            if (strand_fft > 1.0f) strand_fft = 1.0f;

            float strand_vib = sinf(s_web_vib_time * 6.5f + (float)k * 2.2f + rf * 1.5f) * (strand_fft * 2.8f + treble * 1.5f);
            float total_mid_r = base_mid_r + strand_vib;
            if (total_mid_r < 1.5f) total_mid_r = 1.5f;

            int mid_x = cx + (int)(total_mid_r * 1.35f * mid_cos + 0.5f);
            int mid_y = cy + (int)(total_mid_r * 0.95f * mid_sin + 0.5f);

            // Draw 2 sub-segments per strand for curved catenary silk effect
            SafeDraw::drawLine(node_x[k][r], node_y[k][r], mid_x, mid_y);
            SafeDraw::drawLine(mid_x, mid_y, node_x[next_k][r], node_y[next_k][r]);

            // Dew drops / glistening dew beads on outer nodes when music hits
            if ((r >= 4 && bass > 0.45f) || (treble > 0.50f && (k + r) % 3 == 0)) {
                SafeDraw::drawDisc(node_x[k][r], node_y[k][r], 1);
            }
        }
    }

    // 5. Center Nucleus / Spiderweb Core (pulses with bass)
    int hub_r = 1 + (int)(bass * 2.2f);
    SafeDraw::drawDisc(cx, cy, hub_r);

    // 6. Cyber HUD Telemetry Header
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 6, "MVT-SILK");
    SafeDraw::drawStr(98, 6, "SPIN-CW");
}
