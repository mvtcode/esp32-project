#include "reset_button.h"
#include "config_manager.h"

volatile bool buttonPressed = false;
unsigned long buttonPressTime = 0;
bool wasPressed = false;

void IRAM_ATTR resetButtonISR() {
  buttonPressed = true;
}

void setupResetButton() {
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RESET_BUTTON_PIN), resetButtonISR, FALLING);
}

void checkResetButton() {
  // Read current button state
  bool currentlyPressed = (digitalRead(RESET_BUTTON_PIN) == LOW);
  
  if (currentlyPressed && !wasPressed) {
    // Button just pressed - trigger reset immediately with debouncing
    unsigned long now = millis();
    
    // Debounce: ignore if last press was less than 1 second ago
    static unsigned long lastResetTime = 0;
    if (now - lastResetTime < 1000) {
      Serial.println("Reset button debounced - ignoring");
      return;
    }
    
    wasPressed = true;
    lastResetTime = now;
    
    Serial.println("=== RESET BUTTON PRESSED ===");
    Serial.println("=== FACTORY RESET TRIGGERED ===");
    Serial.println("Clearing configuration...");
    clearConfig();
    Serial.println("Restarting in AP mode...");
    delay(1000);
    ESP.restart();
    
  } else if (!currentlyPressed && wasPressed) {
    // Button released
    wasPressed = false;
  }
}
