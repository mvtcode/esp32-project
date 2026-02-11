#include "InputManager.h"

InputManager::InputManager()
    : btnMode{BTN_MODE, false, 0, false}, btnPlay{BTN_PLAY, false, 0, false},
      btnPrevVolMinus{BTN_PREV_VOL_MINUS, false, 0, false},
      btnNextVolPlus{BTN_NEXT_VOL_PLUS, false, 0, false} {}

void InputManager::begin() {
  pinMode(BTN_MODE, INPUT_PULLUP);
  pinMode(BTN_PLAY, INPUT_PULLUP);
  pinMode(BTN_PREV_VOL_MINUS, INPUT); // Requires external pull-up
  pinMode(BTN_NEXT_VOL_PLUS, INPUT);  // Requires external pull-up
}

InputAction InputManager::update() {
  InputAction action = ACTION_NONE;
  unsigned long now = millis();

  // Read Buttons
  bool modeState = digitalRead(BTN_MODE) == LOW;
  bool playState = digitalRead(BTN_PLAY) == LOW;
  bool volMinusState = digitalRead(BTN_PREV_VOL_MINUS) == LOW;
  bool volPlusState = digitalRead(BTN_NEXT_VOL_PLUS) == LOW;

  // --- MODE ---
  if (modeState && !btnMode.isPressed) {
    btnMode.isPressed = true;
    btnMode.pressTime = now;
    btnMode.longPressHandled = false;
  } else if (modeState && btnMode.isPressed &&
             (now - btnMode.pressTime > 2000) && !btnMode.longPressHandled) {
    btnMode.longPressHandled = true;
    action = ACTION_DEEP_SLEEP;
  } else if (!modeState && btnMode.isPressed) {
    if (!btnMode.longPressHandled) {
      action = ACTION_NEXT_MODE;
    }
    btnMode.isPressed = false;
  }

  // --- PLAY ---
  // Note: Play toggles on PRESS in original code
  if (playState && !btnPlay.isPressed) {
    btnPlay.isPressed = true;
    action = ACTION_TOGGLE_PLAY;
  } else if (!playState) {
    btnPlay.isPressed = false;
  }

  // --- Vol- / PREV ---
  if (volMinusState && !btnPrevVolMinus.isPressed) {
    btnPrevVolMinus.isPressed = true;
    btnPrevVolMinus.pressTime = now;
    btnPrevVolMinus.longPressHandled = false;
  } else if (volMinusState && btnPrevVolMinus.isPressed &&
             (now - btnPrevVolMinus.pressTime > 600) &&
             !btnPrevVolMinus.longPressHandled) {
    btnPrevVolMinus.longPressHandled = true;
    action = ACTION_PREV_TRACK;
  } else if (!volMinusState && btnPrevVolMinus.isPressed) {
    if (!btnPrevVolMinus.longPressHandled) {
      action = ACTION_VOL_DOWN;
    }
    btnPrevVolMinus.isPressed = false;
  }

  // --- Vol+ / NEXT ---
  if (volPlusState && !btnNextVolPlus.isPressed) {
    btnNextVolPlus.isPressed = true;
    btnNextVolPlus.pressTime = now;
    btnNextVolPlus.longPressHandled = false;
  } else if (volPlusState && btnNextVolPlus.isPressed &&
             (now - btnNextVolPlus.pressTime > 600) &&
             !btnNextVolPlus.longPressHandled) {
    btnNextVolPlus.longPressHandled = true;
    action = ACTION_NEXT_TRACK;
  } else if (!volPlusState && btnNextVolPlus.isPressed) {
    if (!btnNextVolPlus.longPressHandled) {
      action = ACTION_VOL_UP;
    }
    btnNextVolPlus.isPressed = false;
  }

  return action;
}
