#include "effects.h"

// -----------------------------------------------------------------------
// MODE 47 — 3D SPIRAL GALAXY (Perspective Cosmic Swarm with Audio Waves)
// -----------------------------------------------------------------------

#define GALAXY_STARS 96

struct GalaxyStar {
    float r;       // Radial distance from core
    float base_ang;// Base polar angle on spiral arm
    float z_height;// Vertical offset relative to galactic plane
    uint8_t arm;   // Spiral arm index (0 or 1)
};

static GalaxyStar s_galaxy_stars[GALAXY_STARS];
static float s_galaxy_rot = 0.0f;
static float s_galaxy_vol = 0.0f;

void effect_galaxy_on_enter() {
    s_galaxy_rot = 0.0f;
    s_galaxy_vol = 0.0f;

    for (int i = 0; i < GALAXY_STARS; i++) {
        GalaxyStar &s = s_galaxy_stars[i];
        s.arm = i % 2;
        // Non-linear radial distribution (denser near core)
        float u = (float)i / (float)GALAXY_STARS;
        s.r = 3.5f + sqrtf(u) * 33.0f;
        // Logarithmic spiral angle: theta = k * log(r)
        s.base_ang = s.r * 0.17f + (s.arm * (float)M_PI) + ((float)(rand() % 40) - 20.0f) * 0.01f;
        // Vertical thickness decreases towards edge
        float max_h = 5.0f * (1.0f - s.r / 38.0f);
        s.z_height = ((float)(rand() % 100) - 50.0f) * 0.02f * fmaxf(0.8f, max_h);
    }
}

void effect_galaxy_on_exit() {}

void effect_galaxy_render(const int32_t *left, const int32_t *right, size_t n) {
    // Shift center lower down (cy = 38) so tilted galaxy does not overflow the top edge
    const int cx = SCREEN_W / 2; // 64
    const int cy = 38;

    // 1. Audio volume measurement with enhanced sensitivity
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += abs(left[i]) + abs(right[i]);
    }
    float raw_vol = (float)sum / (float)(n * (s_peak_l + s_peak_r + 1));
    float vol = raw_vol * 2.0f;
    if (vol > 1.0f) vol = 1.0f;
    s_galaxy_vol = s_galaxy_vol * 0.7f + vol * 0.3f;

    // Galactic spin rate boosted by audio
    s_galaxy_rot += 0.022f + s_galaxy_vol * 0.07f;
    if (s_galaxy_rot > 6.2831853f) s_galaxy_rot -= 6.2831853f;

    // 3D Perspective Tilt (~58 degrees tilt)
    float pitch = 1.02f;
    float cos_p = cosf(pitch), sin_p = sinf(pitch);

    float cam_dist = 62.0f;
    float fov = 48.0f;

    float inv_pk_l = (s_peak_l > 0) ? (1.0f / (float)s_peak_l) : 0.0f;
    float inv_pk_r = (s_peak_r > 0) ? (1.0f / (float)s_peak_r) : 0.0f;

    // 2. Render 3D Relativistic Polar Jets (Perpendicular to galactic disc, pulsing with bass)
    float jet_len = 5.0f + s_galaxy_vol * 18.0f;
    // Top Jet (+Z)
    float top_y1 = -jet_len * sin_p;
    float top_z1 =  jet_len * cos_p;
    float top_zcam = cam_dist - top_z1;
    if (top_zcam < 1.0f) top_zcam = 1.0f;
    int top_jy = cy - (int)((top_y1 * fov) / top_zcam);

    // Bottom Jet (-Z)
    float bot_y1 =  jet_len * sin_p;
    float bot_z1 = -jet_len * cos_p;
    float bot_zcam = cam_dist - bot_z1;
    if (bot_zcam < 1.0f) bot_zcam = 1.0f;
    int bot_jy = cy - (int)((bot_y1 * fov) / bot_zcam);

    SafeDraw::drawLine(cx, cy, cx, top_jy);
    SafeDraw::drawLine(cx, cy, cx, bot_jy);
    if (s_galaxy_vol > 0.35f) {
        SafeDraw::drawLine(cx - 1, top_jy, cx + 1, top_jy);
        SafeDraw::drawLine(cx - 1, bot_jy, cx + 1, bot_jy);
    }

    // 3. Render Stars with Stereo Waveform Ripples
    for (int i = 0; i < GALAXY_STARS; i++) {
        const GalaxyStar &s = s_galaxy_stars[i];

        // Stereo audio waveform ripple along spiral arms
        int sample_idx = ((int)s.r * 4) % (n > 0 ? (int)n : 1);
        float wave_amp = 0.0f;
        if (s.arm == 0) {
            wave_amp = (float)left[sample_idx] * inv_pk_l * (2.0f + s_galaxy_vol * 3.5f);
        } else {
            wave_amp = (float)right[sample_idx] * inv_pk_r * (2.0f + s_galaxy_vol * 3.5f);
        }

        float current_r = s.r + wave_amp;
        if (current_r < 2.0f) current_r = 2.0f;
        float current_ang = s.base_ang + s_galaxy_rot;

        // Polar to Cartesian in galactic plane (X-Y)
        float x0 = current_r * cosf(current_ang);
        float y0 = current_r * sinf(current_ang);
        float z0 = s.z_height + wave_amp * 0.4f;

        // Tilt 3D around X axis
        float y1 = y0 * cos_p - z0 * sin_p;
        float z1 = y0 * sin_p + z0 * cos_p;
        float x1 = x0;

        // Perspective Projection
        float z_cam = cam_dist - z1;
        if (z_cam < 1.0f) z_cam = 1.0f;

        int sx = cx + (int)((x1 * fov) / z_cam);
        int sy = cy - (int)((y1 * fov) / z_cam);

        // Render point or disc based on proximity & volume
        if (s.r < 10.0f && s_galaxy_vol > 0.3f) {
            SafeDraw::drawDisc(sx, sy, 1);
        } else {
            SafeDraw::drawPixel(sx, sy);
        }
    }

    // 4. Galactic Supermassive Core & Pulsing Event Horizon Rings
    int core_r = 2 + (int)(s_galaxy_vol * 4.5f);
    SafeDraw::drawDisc(cx, cy, core_r);

    // Outer shockwave gravitational rings on beat
    if (s_galaxy_vol > 0.25f) {
        int ring_w = (int)(10.0f + s_galaxy_vol * 18.0f);
        int ring_h = (int)(ring_w * 0.45f);
        SafeDraw::drawEllipse(cx, cy, ring_w, ring_h);
    }
    if (s_galaxy_vol > 0.55f) {
        int ring_w2 = (int)(20.0f + s_galaxy_vol * 22.0f);
        int ring_h2 = (int)(ring_w2 * 0.45f);
        SafeDraw::drawEllipse(cx, cy, ring_w2, ring_h2);
    }

    // 5. HUD
    SafeDraw::setFont(u8g2_font_4x6_tr);
    SafeDraw::drawStr(2, 6, "GALAXY 3D");
    SafeDraw::drawStr(100, 6, "SPIRAL");
}
