#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "i2s_mic.h"    // for FRAME_SIZE

// -----------------------------------------------------------------------
// OLED pin assignments (confirmed working)
// -----------------------------------------------------------------------
#define OLED_SDA    8
#define OLED_SCL    9

// I2C clock speed:
//   400000 = 400 kHz  (safe, SH1106 datasheet max)
//   800000 = 800 kHz  (overclocked — faster, test stability first)
//   1000000 = 1 MHz   (aggressive, may cause display glitches)
#define I2C_CLOCK   400000

// -----------------------------------------------------------------------
// Screen geometry
// -----------------------------------------------------------------------
#define SCREEN_W      128
#define SCREEN_H       64

// Dual-channel waveform layout (MODE_WAVEFORM)
#define CH_L_CENTER    14   // Y center of LEFT waveform
#define CH_R_CENTER    48   // Y center of RIGHT waveform
#define CH_HALF_H      13   // Half-height of waveform area in pixels
#define SEP_Y          31   // Y position of separator line

// -----------------------------------------------------------------------
// Display modes
// -----------------------------------------------------------------------
enum DisplayMode {
    MODE_WAVEFORM  = 0,   // Dual channel L (top) / R (bottom)
    MODE_MIRROR    = 1,   // Symmetric bars from center — mono mix
    MODE_SPECTRUM  = 2,   // FFT frequency bars (128-pt → 64 bars)
    MODE_LISSAJOUS = 3,   // X=L, Y=R scatter plot
    MODE_VU_METER  = 4,   // Two vertical volume bars with peak hold
    MODE_ANALOG    = 5,   // Dual analog VU needle meter
    MODE_CIRCLE    = 6,   // Circular stereo spectrum with "MVT" center
    MODE_HEART     = 7,   // Heart stereo visualizer with "MVT" text
    MODE_FUSION    = 8,   // Cyber Fusion: 3D spinning MVT + radial rays + flank waves
    MODE_CYBER     = 9,   // Cyber Pulse: Inward horizontal bars + rotating MVT + tech radar rings
    MODE_CASSETTE  = 10,  // Vintage Cassette: Dual spinning spools + live tape waveform
    MODE_TUNNEL    = 11,  // 3D Warp Tunnel: Flying wireframe perspective + bass pulse
    MODE_ORBIT     = 12,  // 3D Atomic Orbit: Tilting orbital rings + particle trails
    MODE_MATRIX    = 13,  // Matrix Digital Rain: 16 FFT-reactive falling data streams
    MODE_TERRAIN   = 14,  // 3D Spectrum Terrain: Wireframe historical waterfall mesh
    MODE_RAIN_HEART = 15,  // Heart Matrix: Digital rain + expanding hollow heart + "MVT"
    MODE_TWIN_HEARTS = 16, // Twin Hearts: Dual solid beating hearts + connecting waveform lifeline
    MODE_DJ_DECK   = 17,  // Cyber DJ Mixer: Dual spinning vinyl turntables + crossfader + mini wave
    MODE_SPEAKER   = 18,  // Hi-Fi Bouncing Speaker: Pulsing subwoofer cone + sound shockwaves
    MODE_HEADPHONE = 19,  // Studio Headphone: Over-ear headset + dual-side stereo audio spectrum
    MODE_SPIDERWEB = 20,  // Cyber Spider Web: 3D geometric web rings vibrating to audio frequencies
    MODE_SYNTHWAVE = 21,  // Synthwave 80s: Neon sun + rolling 3D ground mesh + skyline FFT bars
    MODE_RADAR     = 22,  // Cyber Sonar/Radar: 360-deg sweep + target frequency blips
    MODE_REACTOR   = 23,  // Arc Reactor Core: Counter-rotating gear rings + lightning discharges
    MODE_BLACKHOLE = 24,  // Cosmic Black Hole: Accretion disk particle spiral + vertical relativistic jets
    MODE_HIGHWAY   = 25,  // Cyber Night Highway: Outrun car perspective + road stripes + audio lamp posts
    MODE_SOUND_ICON = 26, // Iconic Sound Megaphone: Speaker horn + blast shockwaves + particles
    MODE_ROTATE_WAVE = 27, // Rotating Wave: Dual boundary endpoints rotating 20s/rev + live stereo waveform
    MODE_BOUNCE_LINES = 28, // Bounce Lines: 10-15px flying lines bouncing off screen borders + audio speed boost
    MODE_STAGE_LASER  = 29, // Stage Lasers: Dual 1/3 and 2/3 overhead scanning laser projectors
    MODE_DANCER       = 30, // Rhythm Dancer: Articulated stickman dancing & headbobbing to audio beats
    MODE_BALL_JUGGLE  = 31, // Waveform Juggle: Physics ball bouncing and slope-deflecting on audio wave
    MODE_VECTORSCOPE  = 32, // Audio Vectorscope: Goniometer M/S polar audio scope rotated 45°
    MODE_SPIROGRAPH   = 33, // Spirograph: Hypotrochoid parametric flower blooming to bass & treble
    MODE_SUPERFORMULA = 34, // Superformula: Johan Gielis morphing bio-geometry polygon / starburst
    MODE_LORENZ       = 35, // Lorenz 3D: Strange attractor butterfly orbit with 3D projection & audio burst
    MODE_POLAR_WAVE   = 36, // Polar Wave Ring: Quantum arc reactor circular waveform & pulsing core
    MODE_CHLADNI      = 37, // Chladni Cymatics: Resonating acoustic plate nodal particle patterns
    MODE_TESSERACT    = 38, // 4D Tesseract: Rotating hypercube wireframe perspective projection
    MODE_WATERFALL    = 39, // Waterfall Spectrogram: Rolling historical FFT waterfall with dithered density
    MODE_TEXT_WARP    = 40, // 3D Space Warp Text: LOVE MAC TAN flying 3D starfield tunnel
    MODE_TEXT_PARTICLES = 41, // Kinetic Particle Assembly: Morphing particle swarm assembling LOVE MAC TAN
    MODE_STARFIELD    = 42, // 3D Warp Starfield: 3D stars flying towards camera with audio-driven warp streaks
    MODE_SPHERE3D     = 43, // 3D Wireframe Sphere: Dual-axis rotating globe pulsing and deforming to audio
    MODE_TORUS        = 44, // 3D Torus: Rotating 3D donut ring with audio-reactive ripple waves
    MODE_DNA_HELIX    = 45, // 3D DNA Helix: Double helix rotating 3D strands with frequency-reactive rungs
    MODE_CUBES3D      = 46, // 3D Nested Cubes: Counter-rotating wireframe cubes with beat-reactive explosion
    MODE_GALAXY       = 47, // 3D Spiral Galaxy: Perspective tilted spiral arm particles pulsing to bass
    MODE_CRYSTAL3D    = 48, // 3D Crystal Polyhedron: Floating geometric gem bursting particle rays on beats
    MODE_CYLINDER3D   = 49, // 3D Audio Cylinder: Perspective wireframe audio tube with live waveform skin
    MODE_HEART3D      = 50, // 3D Parametric Heart: Rotating 3D wireframe heart mesh with live waveform ripples
    MODE_TEXT3D       = 51, // 3D MVT Text: Extruded 3D wireframe MVT logo tumbling in space with audio deform
    MODE_SOLAR        = 52, // 3D Solar System: Keplerian planetary orbits with bass-reactive solar flares & wind
    MODE_SUPERNOVA    = 53, // MVT Supernova: Collapsing star & beat-drop cosmic explosion with plasma remnants
    MODE_THUNDER      = 54, // Cyber Thunderstorm: Fractal lightning strikes, stereo wind rain & ocean waveform
    MODE_COCKPIT      = 55, // Cyber Cockpit: Sci-Fi spaceship HUD with stereo radar, reactor & warp star speed
    MODE_INVADERS     = 56, // Space Invaders: Retro arcade cannon dodging via stereo balance & laser bass blasts
    MODE_FLAPPY       = 57, // Flappy Beat: Retro bird altitude reacting to RMS volume through FFT pipe obstacles
    MODE_PACMAN       = 58, // Pac-Beat: Waveform chomp mouth Pac-Man navigating dots & beat power-pellet frenzy
    MODE_DINO         = 59, // Dino Runner: Pixel T-Rex auto-jumping cacti on bass kicks across desert skyline
    MODE_XIAOZHI      = 60, // Xiaozhi AI Face: Cute OLED expressive robot eyes & live waveform talking mouth
    MODE_AI_BOT       = 61, // AI Robot Companion: Sci-fi antenna broadcast, stereo ear EQs & dancing head
    MODE_PLASMA_BALL  = 62, // Magic Plasma Ball: Core electrode & twisting electric arcs discharging to glass sphere
    MODE_ZIGZAG_STAGE = 63, // EDM Zigzag Stage: Multi-tier chevron stage LED arrays with chasing strobe pulses
    MODE_CAT          = 64, // Cyber Neko Cat: Stereo ear twitches, waveform talking mouth & wagging tail
    MODE_COUNT        = 65
};

// -----------------------------------------------------------------------
// API
// -----------------------------------------------------------------------

/** Initialize OLED (HW I2C) and show splash screen. */
void display_init();

/** Show an error message and stop updating (for fatal errors). */
void display_error(const char *msg);

/**
 * Render one audio frame using the current display mode.
 * Includes per-channel AGC and mode-label overlay on mode change.
 *
 * @param left   Left  channel samples (24-bit range, FRAME_SIZE elements)
 * @param right  Right channel samples (24-bit range, FRAME_SIZE elements)
 * @param n      Number of samples per channel (should equal FRAME_SIZE)
 */
void display_draw_waveform(const int32_t *left, const int32_t *right, size_t n);

/** Switch to a specific display mode. Triggers mode label for 1.5 s if show_label is true. */
void display_set_mode(DisplayMode m, bool show_label = true);

/** Advance to the next mode (wraps around). */
void display_next_mode(bool show_label = true);

/** Return the currently active display mode. */
DisplayMode display_get_mode();

/** Enable or disable auto-cycling of display modes. */
void display_set_auto_cycle(bool enable, uint32_t interval_ms = 20000);

/** Check if auto-cycling is enabled. */
bool display_get_auto_cycle();
