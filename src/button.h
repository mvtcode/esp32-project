#pragma once
#include <Arduino.h>

#define BTN_GPIO        0       // BOOT button — active LOW, built-in pull-up
#define BTN_DEBOUNCE_MS 200     // Debounce window in milliseconds (200ms prevents rapid crash on fast switching)

/**
 * @brief Initialize the button GPIO with internal pull-up.
 * @param gpio  GPIO number (default: BTN_GPIO = 0, the BOOT button)
 */
void button_init(uint8_t gpio = BTN_GPIO);

/**
 * @brief Update the internal button state. Call this frequently (e.g., in loop()).
 */
void button_update();

/**
 * @brief Check if the button was short pressed.
 * @return true if short pressed, false otherwise.
 */
bool button_pressed();

/**
 * @brief Check if the button was long pressed (held > 1s).
 * @return true if long pressed, false otherwise.
 */
bool button_long_pressed();
