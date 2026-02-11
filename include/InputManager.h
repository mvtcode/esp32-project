#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "Constants.h"
#include <Arduino.h>

class InputManager {
public:
  InputManager();
  void begin();
  InputAction update();

private:
  struct ButtonState {
    uint8_t pin;
    bool isPressed;
    unsigned long pressTime;
    bool longPressHandled;
  };

  ButtonState btnMode;
  ButtonState btnPlay;
  ButtonState btnPrevVolMinus;
  ButtonState btnNextVolPlus;
};

#endif // INPUT_MANAGER_H
