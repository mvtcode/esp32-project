#include "effects.h"

// -----------------------------------------------------------------------
// Visual effect lookup table (maps DisplayMode enum to effect handlers)
// -----------------------------------------------------------------------
const VisualEffect EFFECTS[MODE_COUNT] = {
    // Mode 0: WAVEFORM
    { "WAVEFORM",     effect_waveform_render,    nullptr,                    nullptr },
    // Mode 1: MIRROR
    { "MIRROR",       effect_mirror_render,      nullptr,                    nullptr },
    // Mode 2: SPECTRUM
    { "SPECTRUM",     effect_spectrum_render,    effect_spectrum_on_enter,   effect_spectrum_on_exit },
    // Mode 3: LISSAJOUS
    { "LISSAJOUS",    effect_lissajous_render,   nullptr,                    nullptr },
    // Mode 4: VU_METER
    { "VU METER",     effect_vu_meter_render,    effect_vu_meter_on_enter,   effect_vu_meter_on_exit },
    // Mode 5: ANALOG
    { "ANALOG VU",    effect_analog_vu_render,   effect_analog_vu_on_enter,  effect_analog_vu_on_exit },
    // Mode 6: CIRCLE
    { "CIRCLE MVT",   effect_circle_render,      effect_circle_on_enter,     effect_circle_on_exit },
    // Mode 7: HEART
    { "MVT HEART",    effect_heart_render,       effect_heart_on_enter,      effect_heart_on_exit },
    // Mode 8: FUSION
    { "MVT FUSION",   effect_fusion_render,      effect_fusion_on_enter,     effect_fusion_on_exit },
    // Mode 9: CYBER
    { "MVT CYBER",    effect_cyber_render,       effect_cyber_on_enter,      effect_cyber_on_exit },
    // Mode 10: CASSETTE
    { "MVT CASSETTE", effect_cassette_render,    effect_cassette_on_enter,   effect_cassette_on_exit },
    // Mode 11: TUNNEL
    { "MVT TUNNEL",   effect_tunnel_render,      nullptr,                    nullptr },
    // Mode 12: ORBIT
    { "MVT ORBIT",    effect_orbit_render,       effect_orbit_on_enter,      effect_orbit_on_exit },
    // Mode 13: MATRIX
    { "MVT MATRIX",   effect_matrix_render,      effect_matrix_on_enter,     effect_matrix_on_exit },
    // Mode 14: TERRAIN
    { "MVT TERRAIN",  effect_terrain_render,     effect_terrain_on_enter,    effect_terrain_on_exit },
    // Mode 15: RAIN_HEART
    { "HEART MATRIX", effect_rain_heart_render,  effect_rain_heart_on_enter, effect_rain_heart_on_exit },
    // Mode 16: TWIN_HEARTS
    { "TWIN HEARTS",  effect_twin_hearts_render, nullptr,                    nullptr },
    // Mode 17: DJ_DECK
    { "MVT DJ DECK",  effect_dj_render,          effect_dj_on_enter,         effect_dj_on_exit },
    // Mode 18: SPEAKER
    { "MVT SPEAKER",  effect_speaker_render,     effect_speaker_on_enter,    effect_speaker_on_exit },
    // Mode 19: HEADPHONE
    { "MVT HEADPHONE",effect_headphone_render,   effect_headphone_on_enter,  effect_headphone_on_exit },
    // Mode 20: SPIDERWEB
    { "MVT SPIDERWEB",effect_spiderweb_render,   effect_spiderweb_on_enter,  effect_spiderweb_on_exit },
    // Mode 21: SYNTHWAVE
    { "MVT SYNTHWAVE",effect_synthwave_render,   effect_synthwave_on_enter,  effect_synthwave_on_exit },
    // Mode 22: RADAR
    { "MVT RADAR",    effect_radar_render,       effect_radar_on_enter,      effect_radar_on_exit },
    // Mode 23: REACTOR
    { "MVT REACTOR",  effect_reactor_render,     effect_reactor_on_enter,    effect_reactor_on_exit },
    // Mode 24: BLACKHOLE
    { "MVT BLACKHOLE",effect_blackhole_render,   effect_blackhole_on_enter,  effect_blackhole_on_exit },
    // Mode 25: HIGHWAY
    { "MVT HIGHWAY",  effect_highway_render,     effect_highway_on_enter,    effect_highway_on_exit },
    // Mode 26: SOUND_ICON
    { "MVT SOUND",    effect_sound_icon_render,  effect_sound_icon_on_enter, effect_sound_icon_on_exit },
    // Mode 27: ROTATE_WAVE
    { "MVT ROTATE",   effect_rotate_wave_render, effect_rotate_wave_on_enter, effect_rotate_wave_on_exit },
    // Mode 28: BOUNCE_LINES
    { "MVT BOUNCE",   effect_bounce_lines_render, effect_bounce_lines_on_enter, effect_bounce_lines_on_exit },
    // Mode 29: STAGE_LASER
    { "MVT LASER",    effect_stage_laser_render,  effect_stage_laser_on_enter,  effect_stage_laser_on_exit },
    // Mode 30: DANCER
    { "MVT DANCER",   effect_dancer_render,       effect_dancer_on_enter,       effect_dancer_on_exit },
    // Mode 31: BALL_JUGGLE
    { "MVT JUGGLE",   effect_ball_juggle_render,  effect_ball_juggle_on_enter,  effect_ball_juggle_on_exit },
    // Mode 32: VECTORSCOPE
    { "MVT VECTORSCOPE", effect_vectorscope_render, effect_vectorscope_on_enter, effect_vectorscope_on_exit },
    // Mode 33: SPIROGRAPH
    { "MVT SPIROGRAPH",  effect_spirograph_render,  effect_spirograph_on_enter,  effect_spirograph_on_exit },
    // Mode 34: SUPERFORMULA
    { "MVT SUPERFORM",   effect_superformula_render, effect_superformula_on_enter, effect_superformula_on_exit },
    // Mode 35: LORENZ
    { "MVT LORENZ 3D",   effect_lorenz_render,       effect_lorenz_on_enter,       effect_lorenz_on_exit },
    // Mode 36: POLAR_WAVE
    { "MVT POLAR WAVE",  effect_polar_wave_render,   effect_polar_wave_on_enter,   effect_polar_wave_on_exit },
    // Mode 37: CHLADNI
    { "MVT CHLADNI",     effect_chladni_render,      effect_chladni_on_enter,      effect_chladni_on_exit },
    // Mode 38: TESSERACT
    { "MVT TESSERACT",   effect_tesseract_render,    effect_tesseract_on_enter,    effect_tesseract_on_exit },
    // Mode 39: WATERFALL
    { "MVT WATERFALL",   effect_waterfall_render,    effect_waterfall_on_enter,    effect_waterfall_on_exit },
    // Mode 40: TEXT_WARP
    { "MVT WARP TEXT",   effect_text_warp_render,    effect_text_warp_on_enter,    effect_text_warp_on_exit },
    // Mode 41: TEXT_PARTICLES
    { "MVT PARTICLE",    effect_text_particles_render, effect_text_particles_on_enter, effect_text_particles_on_exit },
    // Mode 42: STARFIELD
    { "MVT STARFIELD",   effect_starfield_render,    effect_starfield_on_enter,    effect_starfield_on_exit },
    // Mode 43: SPHERE3D
    { "MVT SPHERE 3D",   effect_sphere3d_render,     effect_sphere3d_on_enter,     effect_sphere3d_on_exit },
    // Mode 44: TORUS
    { "MVT TORUS 3D",    effect_torus_render,        effect_torus_on_enter,        effect_torus_on_exit },
    // Mode 45: DNA_HELIX
    { "MVT DNA HELIX",   effect_dna_helix_render,    effect_dna_helix_on_enter,    effect_dna_helix_on_exit },
    // Mode 46: CUBES3D
    { "MVT CUBES 3D",    effect_cubes3d_render,      effect_cubes3d_on_enter,      effect_cubes3d_on_exit },
    // Mode 47: GALAXY
    { "MVT GALAXY 3D",   effect_galaxy_render,       effect_galaxy_on_enter,       effect_galaxy_on_exit },
    // Mode 48: CRYSTAL3D
    { "MVT CRYSTAL 3D",  effect_crystal3d_render,    effect_crystal3d_on_enter,    effect_crystal3d_on_exit },
    // Mode 49: CYLINDER3D
    { "MVT CYLINDER",    effect_cylinder3d_render,   effect_cylinder3d_on_enter,   effect_cylinder3d_on_exit },
    // Mode 50: HEART3D
    { "MVT HEART 3D",    effect_heart3d_render,      effect_heart3d_on_enter,      effect_heart3d_on_exit },
    // Mode 51: TEXT3D
    { "MVT TEXT 3D",     effect_text3d_render,       effect_text3d_on_enter,       effect_text3d_on_exit },
    // Mode 52: SOLAR
    { "SOLAR SYSTEM",    effect_solar_render,        effect_solar_on_enter,        effect_solar_on_exit },
    // Mode 53: SUPERNOVA
    { "MVT SUPERNOVA",   effect_supernova_render,    effect_supernova_on_enter,    effect_supernova_on_exit },
    // Mode 54: THUNDER
    { "THUNDERSTORM",    effect_thunder_render,      effect_thunder_on_enter,      effect_thunder_on_exit },
    // Mode 55: COCKPIT
    { "CYBER COCKPIT",   effect_cockpit_render,      effect_cockpit_on_enter,      effect_cockpit_on_exit },
    // Mode 56: INVADERS
    { "SPACE INVADERS",  effect_invaders_render,     effect_invaders_on_enter,     effect_invaders_on_exit },
    // Mode 57: FLAPPY
    { "FLAPPY BEAT",     effect_flappy_render,       effect_flappy_on_enter,       effect_flappy_on_exit },
    // Mode 58: PACMAN
    { "PAC-BEAT",        effect_pacman_render,       effect_pacman_on_enter,       effect_pacman_on_exit },
    // Mode 59: DINO
    { "DINO RUNNER",     effect_dino_render,         effect_dino_on_enter,         effect_dino_on_exit },
    // Mode 60: XIAOZHI
    { "XIAOZHI AI",      effect_xiaozhi_render,      effect_xiaozhi_on_enter,      effect_xiaozhi_on_exit },
    // Mode 61: AI_BOT
    { "AI ROBOT",        effect_ai_bot_render,       effect_ai_bot_on_enter,       effect_ai_bot_on_exit },
    // Mode 62: PLASMA_BALL
    { "PLASMA BALL",     effect_plasma_ball_render,  effect_plasma_ball_on_enter,  effect_plasma_ball_on_exit },
    // Mode 63: ZIGZAG_STAGE
    { "LED ZIGZAG",      effect_zigzag_stage_render, effect_zigzag_stage_on_enter, effect_zigzag_stage_on_exit },
    // Mode 64: CAT
    { "CYBER NEKO",      effect_cat_render,          effect_cat_on_enter,          effect_cat_on_exit },
    // Mode 65: CLOCK & WEATHER
    { "CLOCK & WEATHER", effect_clock_render,        nullptr,                      nullptr }
};

