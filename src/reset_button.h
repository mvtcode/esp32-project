#ifndef RESET_BUTTON_H
#define RESET_BUTTON_H

#include <Arduino.h>

// Reset button configuration
#define RESET_BUTTON_PIN 0  // GPIO 0 - BOOT button (has internal pull-up)
#define LONG_PRESS_TIME 3000 // 3 seconds in milliseconds

// Reset button functions
void setupResetButton();
void checkResetButton(); // Call this in loop()

#endif // RESET_BUTTON_H
