#pragma once
#include <Arduino.h>

#define BTN_GPIO        0       // BOOT button — active LOW, built-in pull-up
#define BTN_DEBOUNCE_MS 50      // Debounce window in milliseconds

/**
 * @brief Initialize the button GPIO with internal pull-up.
 * @param gpio  GPIO number (default: BTN_GPIO = 0, the BOOT button)
 */
void button_init(uint8_t gpio = BTN_GPIO);

/**
 * @brief Check if the button was pressed since the last call.
 *
 * Returns true exactly once per physical button press (falling edge detect).
 * Must be called frequently (e.g. in loop()) to catch presses.
 *
 * @return true on new press event, false otherwise.
 */
bool button_pressed();
