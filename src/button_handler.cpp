#include "button_handler.h"
#include "led_indicator.h"
#include "debug.h"
#include <LittleFS.h>
#include <WiFiManager.h>

static unsigned long buttonPressStart = 0;
static bool buttonIsPressed = false;

void initButton() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void checkConfigButton() {
  bool pressed = (digitalRead(BUTTON_PIN) == LOW);
  
  if (pressed) {
    if (!buttonIsPressed) {
      buttonPressStart = millis();
      buttonIsPressed = true;
      DEBUG_PRINTLN("[Button] Press detected...");
    } else {
      unsigned long duration = millis() - buttonPressStart;
      if (duration > 3000) {
        DEBUG_PRINTLN("[Button] Held for 3 seconds! Resetting configuration...");
        
        // Fast flashing LED to notify user (only in Phase 1)
#ifdef STATUS_LED_PIN
        for (int i = 0; i < 15; i++) {
          digitalWrite(STATUS_LED_PIN, LOW); // ON
          delay(50);
          digitalWrite(STATUS_LED_PIN, HIGH); // OFF
          delay(50);
        }
#else
        delay(1500);
#endif
        
        WiFiManager wm;
        wm.resetSettings();
        LittleFS.remove("/config.json");
        
        DEBUG_PRINTLN("[System] Reset successful. Restarting...");
        delay(500);
        ESP.restart();
      }
    }
  } else {
    if (buttonIsPressed) {
      buttonIsPressed = false;
      DEBUG_PRINTLN("[Button] Released.");
    }
  }
}
