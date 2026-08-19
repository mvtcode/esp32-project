#pragma once
#include <Arduino.h>

enum BtnId {
    BTN_PUSH = 0,   // GPIO 4  (Encoder Push - Switch MIC/BT)
    BTN_BACK,       // GPIO 13 (Button Back - Play/Pause)
    BTN_PLUS,       // GPIO 14 (Button Plus/CON - Next Effect / Auto-cycle)
    BTN_BOOT,       // GPIO 0  (Board BOOT - Short: WiFi reset, Long >3s: BT pair)
    BTN_COUNT
};

// GPIO pin mapping
#define PIN_BTN_PUSH   4
#define PIN_BTN_BACK   13
#define PIN_BTN_PLUS   14
#define PIN_BTN_BOOT   0

#define BTN_DEBOUNCE_MS 50     // Debounce window in milliseconds

/**
 * @brief Initialize all button GPIOs with internal pull-up.
 */
void buttons_init();

/**
 * @brief Update the internal state of all buttons. Call in loop().
 */
void buttons_update();

/**
 * @brief Check if a specific button was short pressed.
 */
bool button_pressed(BtnId btn);

/**
 * @brief Check if a specific button was long pressed.
 */
bool button_long_pressed(BtnId btn);

/**
 * @brief Check if a button is currently held down.
 */
bool button_is_down(BtnId btn);

