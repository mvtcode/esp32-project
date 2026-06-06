#include "led_indicator.h"

#ifdef STATUS_LED_PIN
#include <Ticker.h>
static Ticker ledTicker;

static void tickLED() {
  int state = digitalRead(STATUS_LED_PIN);
  digitalWrite(STATUS_LED_PIN, !state);
}

void initLed() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  // Default to LED OFF (active low)
  digitalWrite(STATUS_LED_PIN, HIGH);
}

void setLedMode(LedMode mode) {
  ledTicker.detach();
  if (mode == LED_MODE_OFF) {
    digitalWrite(STATUS_LED_PIN, HIGH); // Off
  } else if (mode == LED_MODE_ON) {
    digitalWrite(STATUS_LED_PIN, LOW);  // On
  } else if (mode == LED_MODE_SLOW) {
    ledTicker.attach(0.5, tickLED);
  } else if (mode == LED_MODE_FAST) {
    ledTicker.attach(0.1, tickLED);
  }
}

bool isLedActive() {
  return ledTicker.active();
}
#else
// Dummy implementations for Phase 2
void initLed() {}
void setLedMode(LedMode mode) {}
bool isLedActive() { return false; }
#endif
