#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>

#ifndef BUTTON_PIN
#define BUTTON_PIN 2  // Default fallback to GPIO2
#endif

void initButton();
void checkConfigButton();

#endif
