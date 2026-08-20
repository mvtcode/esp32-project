#pragma once
#include <Arduino.h>

#define ENCODER_PIN_CLK  32  // TRA
#define ENCODER_PIN_DT   33  // TRB

/**
 * @brief Initialize EC11 rotary encoder on GPIO 32 & 33 with interrupts.
 */
void encoder_init();

/**
 * @brief Enable or disable rotary encoder hardware decoding.
 */
void encoder_set_enabled(bool enabled);
bool encoder_is_enabled();

/**
 * @brief Get the accumulated rotation delta (+steps or -steps) and reset to 0.
 * @return Signed change in volume/encoder position.
 */
int32_t encoder_get_delta();
