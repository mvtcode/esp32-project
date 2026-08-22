#pragma once
#include "effect_common.h"

// -----------------------------------------------------------------------
// Effect function declarations
// -----------------------------------------------------------------------

// Mode 0: WAVEFORM
void effect_waveform_render(const int32_t *left, const int32_t *right, size_t n);

// Mode 1: MIRROR
void effect_mirror_render(const int32_t *left, const int32_t *right, size_t n);

// Mode 2: SPECTRUM
void effect_spectrum_render(const int32_t *left, const int32_t *right, size_t n);
void effect_spectrum_on_enter();
void effect_spectrum_on_exit();

// Mode 3: LISSAJOUS
void effect_lissajous_render(const int32_t *left, const int32_t *right, size_t n);

// Mode 4: VU_METER
void effect_vu_meter_render(const int32_t *left, const int32_t *right, size_t n);
void effect_vu_meter_on_enter();
void effect_vu_meter_on_exit();

// Mode 5: ANALOG
void effect_analog_vu_render(const int32_t *left, const int32_t *right, size_t n);
void effect_analog_vu_on_enter();
void effect_analog_vu_on_exit();

// Mode 6: CIRCLE
void effect_circle_render(const int32_t *left, const int32_t *right, size_t n);
void effect_circle_on_enter();
void effect_circle_on_exit();

// Mode 7: HEART
void effect_heart_render(const int32_t *left, const int32_t *right, size_t n);
void effect_heart_on_enter();
void effect_heart_on_exit();

// Mode 8: FUSION
void effect_fusion_render(const int32_t *left, const int32_t *right, size_t n);
void effect_fusion_on_enter();
void effect_fusion_on_exit();

// Mode 9: CYBER
void effect_cyber_render(const int32_t *left, const int32_t *right, size_t n);
void effect_cyber_on_enter();
void effect_cyber_on_exit();

// Mode 10: CASSETTE
void effect_cassette_render(const int32_t *left, const int32_t *right, size_t n);
void effect_cassette_on_enter();
void effect_cassette_on_exit();

// Mode 11: TUNNEL
void effect_tunnel_render(const int32_t *left, const int32_t *right, size_t n);

// Mode 12: ORBIT
void effect_orbit_render(const int32_t *left, const int32_t *right, size_t n);
void effect_orbit_on_enter();
void effect_orbit_on_exit();

// Mode 13: MATRIX
void effect_matrix_render(const int32_t *left, const int32_t *right, size_t n);
void effect_matrix_on_enter();
void effect_matrix_on_exit();

// Mode 14: TERRAIN
void effect_terrain_render(const int32_t *left, const int32_t *right, size_t n);
void effect_terrain_on_enter();
void effect_terrain_on_exit();

// Mode 15: RAIN_HEART
void effect_rain_heart_render(const int32_t *left, const int32_t *right, size_t n);
void effect_rain_heart_on_enter();
void effect_rain_heart_on_exit();

// Mode 16: TWIN_HEARTS
void effect_twin_hearts_render(const int32_t *left, const int32_t *right, size_t n);

// Mode 17: DJ_DECK
void effect_dj_render(const int32_t *left, const int32_t *right, size_t n);
void effect_dj_on_enter();
void effect_dj_on_exit();

// Mode 18: SPEAKER
void effect_speaker_render(const int32_t *left, const int32_t *right, size_t n);
void effect_speaker_on_enter();
void effect_speaker_on_exit();

// Mode 19: HEADPHONE
void effect_headphone_render(const int32_t *left, const int32_t *right, size_t n);
void effect_headphone_on_enter();
void effect_headphone_on_exit();

// Mode 20: SPIDERWEB
void effect_spiderweb_render(const int32_t *left, const int32_t *right, size_t n);
void effect_spiderweb_on_enter();
void effect_spiderweb_on_exit();

// Mode 21: SYNTHWAVE
void effect_synthwave_render(const int32_t *left, const int32_t *right, size_t n);
void effect_synthwave_on_enter();
void effect_synthwave_on_exit();

// Mode 22: RADAR
void effect_radar_render(const int32_t *left, const int32_t *right, size_t n);
void effect_radar_on_enter();
void effect_radar_on_exit();

// Mode 23: REACTOR
void effect_reactor_render(const int32_t *left, const int32_t *right, size_t n);
void effect_reactor_on_enter();
void effect_reactor_on_exit();

// Mode 24: BLACKHOLE
void effect_blackhole_render(const int32_t *left, const int32_t *right, size_t n);
void effect_blackhole_on_enter();
void effect_blackhole_on_exit();

// Mode 25: HIGHWAY
void effect_highway_render(const int32_t *left, const int32_t *right, size_t n);
void effect_highway_on_enter();
void effect_highway_on_exit();

// Mode 26: SOUND_ICON
void effect_sound_icon_render(const int32_t *left, const int32_t *right, size_t n);
void effect_sound_icon_on_enter();
void effect_sound_icon_on_exit();

// Mode 27: ROTATING_WAVE
void effect_rotate_wave_render(const int32_t *left, const int32_t *right, size_t n);
void effect_rotate_wave_on_enter();
void effect_rotate_wave_on_exit();

// Mode 28: BOUNCE_LINES
void effect_bounce_lines_render(const int32_t *left, const int32_t *right, size_t n);
void effect_bounce_lines_on_enter();
void effect_bounce_lines_on_exit();

// Mode 29: STAGE_LASER
void effect_stage_laser_render(const int32_t *left, const int32_t *right, size_t n);
void effect_stage_laser_on_enter();
void effect_stage_laser_on_exit();

// Mode 30: DANCER
void effect_dancer_render(const int32_t *left, const int32_t *right, size_t n);
void effect_dancer_on_enter();
void effect_dancer_on_exit();

// Mode 31: BALL_JUGGLE
void effect_ball_juggle_render(const int32_t *left, const int32_t *right, size_t n);
void effect_ball_juggle_on_enter();
void effect_ball_juggle_on_exit();

// Mode 32: VECTORSCOPE
void effect_vectorscope_render(const int32_t *left, const int32_t *right, size_t n);
void effect_vectorscope_on_enter();
void effect_vectorscope_on_exit();

// Mode 33: SPIROGRAPH
void effect_spirograph_render(const int32_t *left, const int32_t *right, size_t n);
void effect_spirograph_on_enter();
void effect_spirograph_on_exit();

// Mode 34: SUPERFORMULA
void effect_superformula_render(const int32_t *left, const int32_t *right, size_t n);
void effect_superformula_on_enter();
void effect_superformula_on_exit();

// Mode 35: LORENZ
void effect_lorenz_render(const int32_t *left, const int32_t *right, size_t n);
void effect_lorenz_on_enter();
void effect_lorenz_on_exit();

// Mode 36: POLAR_WAVE
void effect_polar_wave_render(const int32_t *left, const int32_t *right, size_t n);
void effect_polar_wave_on_enter();
void effect_polar_wave_on_exit();

// Mode 37: CHLADNI
void effect_chladni_render(const int32_t *left, const int32_t *right, size_t n);
void effect_chladni_on_enter();
void effect_chladni_on_exit();

// Mode 38: TESSERACT
void effect_tesseract_render(const int32_t *left, const int32_t *right, size_t n);
void effect_tesseract_on_enter();
void effect_tesseract_on_exit();

// Mode 39: WATERFALL
void effect_waterfall_render(const int32_t *left, const int32_t *right, size_t n);
void effect_waterfall_on_enter();
void effect_waterfall_on_exit();

// Mode 40: TEXT_WARP
void effect_text_warp_render(const int32_t *left, const int32_t *right, size_t n);
void effect_text_warp_on_enter();
void effect_text_warp_on_exit();

// Mode 41: TEXT_PARTICLES
void effect_text_particles_render(const int32_t *left, const int32_t *right, size_t n);
void effect_text_particles_on_enter();
void effect_text_particles_on_exit();

// Mode 42: STARFIELD
void effect_starfield_render(const int32_t *left, const int32_t *right, size_t n);
void effect_starfield_on_enter();
void effect_starfield_on_exit();

// Mode 43: SPHERE3D
void effect_sphere3d_render(const int32_t *left, const int32_t *right, size_t n);
void effect_sphere3d_on_enter();
void effect_sphere3d_on_exit();

// Mode 44: TORUS
void effect_torus_render(const int32_t *left, const int32_t *right, size_t n);
void effect_torus_on_enter();
void effect_torus_on_exit();

// Mode 45: DNA_HELIX
void effect_dna_helix_render(const int32_t *left, const int32_t *right, size_t n);
void effect_dna_helix_on_enter();
void effect_dna_helix_on_exit();

// Mode 46: CUBES3D
void effect_cubes3d_render(const int32_t *left, const int32_t *right, size_t n);
void effect_cubes3d_on_enter();
void effect_cubes3d_on_exit();

// Mode 47: GALAXY
void effect_galaxy_render(const int32_t *left, const int32_t *right, size_t n);
void effect_galaxy_on_enter();
void effect_galaxy_on_exit();

// Mode 48: CRYSTAL3D
void effect_crystal3d_render(const int32_t *left, const int32_t *right, size_t n);
void effect_crystal3d_on_enter();
void effect_crystal3d_on_exit();

// Mode 49: CYLINDER3D
void effect_cylinder3d_render(const int32_t *left, const int32_t *right, size_t n);
void effect_cylinder3d_on_enter();
void effect_cylinder3d_on_exit();

// Mode 50: HEART3D
void effect_heart3d_render(const int32_t *left, const int32_t *right, size_t n);
void effect_heart3d_on_enter();
void effect_heart3d_on_exit();

// Mode 51: TEXT3D
void effect_text3d_render(const int32_t *left, const int32_t *right, size_t n);
void effect_text3d_on_enter();
void effect_text3d_on_exit();

// Mode 52: SOLAR
void effect_solar_render(const int32_t *left, const int32_t *right, size_t n);
void effect_solar_on_enter();
void effect_solar_on_exit();

// Mode 53: SUPERNOVA
void effect_supernova_render(const int32_t *left, const int32_t *right, size_t n);
void effect_supernova_on_enter();
void effect_supernova_on_exit();

// Mode 54: THUNDER
void effect_thunder_render(const int32_t *left, const int32_t *right, size_t n);
void effect_thunder_on_enter();
void effect_thunder_on_exit();

// Mode 55: COCKPIT
void effect_cockpit_render(const int32_t *left, const int32_t *right, size_t n);
void effect_cockpit_on_enter();
void effect_cockpit_on_exit();

// Mode 56: INVADERS
void effect_invaders_render(const int32_t *left, const int32_t *right, size_t n);
void effect_invaders_on_enter();
void effect_invaders_on_exit();

// Mode 57: FLAPPY
void effect_flappy_render(const int32_t *left, const int32_t *right, size_t n);
void effect_flappy_on_enter();
void effect_flappy_on_exit();

// Mode 58: PACMAN
void effect_pacman_render(const int32_t *left, const int32_t *right, size_t n);
void effect_pacman_on_enter();
void effect_pacman_on_exit();

// Mode 59: DINO
void effect_dino_render(const int32_t *left, const int32_t *right, size_t n);
void effect_dino_on_enter();
void effect_dino_on_exit();

// Mode 60: XIAOZHI
void effect_xiaozhi_render(const int32_t *left, const int32_t *right, size_t n);
void effect_xiaozhi_on_enter();
void effect_xiaozhi_on_exit();
void effect_xiaozhi_set_state(int state, float tts_energy);

// Mode 61: AI_BOT
void effect_ai_bot_render(const int32_t *left, const int32_t *right, size_t n);
void effect_ai_bot_on_enter();
void effect_ai_bot_on_exit();

// Mode 62: PLASMA_BALL
void effect_plasma_ball_render(const int32_t *left, const int32_t *right, size_t n);
void effect_plasma_ball_on_enter();
void effect_plasma_ball_on_exit();

// Mode 63: ZIGZAG_STAGE
void effect_zigzag_stage_render(const int32_t *left, const int32_t *right, size_t n);
void effect_zigzag_stage_on_enter();
void effect_zigzag_stage_on_exit();

// Mode 64: CAT
void effect_cat_render(const int32_t *left, const int32_t *right, size_t n);
void effect_cat_on_enter();
void effect_cat_on_exit();

// Mode 65: BEAT_METER
void effect_beat_meter_render(const int32_t *left, const int32_t *right, size_t n);
void effect_beat_meter_on_enter();
void effect_beat_meter_on_exit();

// Mode 66: OSCILLOSCOPE
void effect_oscilloscope_render(const int32_t *left, const int32_t *right, size_t n);
void effect_oscilloscope_on_enter();
void effect_oscilloscope_on_exit();

// Mode 67: CHROMATIC_TUNER
void effect_chromatic_tuner_render(const int32_t *left, const int32_t *right, size_t n);
void effect_chromatic_tuner_on_enter();
void effect_chromatic_tuner_on_exit();

// -----------------------------------------------------------------------
// Visual effect lookup table
// -----------------------------------------------------------------------
extern const VisualEffect EFFECTS[MODE_COUNT];







