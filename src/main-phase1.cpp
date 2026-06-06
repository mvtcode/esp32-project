#include <Arduino.h>
#include "led_indicator.h"
#include "config_manager.h"
#include "wifi_controller.h"
#include "ntp_manager.h"
#include "button_handler.h"
#include "debug.h"

void setup() {
  DEBUG_BEGIN(115200);
  delay(1000);
  DEBUG_PRINTLN("\n\n=========================================");
  DEBUG_PRINTLN("      ESP Clock - Phase 1 Start (Modular)");
  DEBUG_PRINTLN("=========================================");

  // Initialize modular units
  initLed();
  initButton();
  initConfig();
  loadConfig();
  
  // Start WiFi (blocks if config portal is needed)
  initWiFi();
  
  // Start NTP
  delay(1500); // Allow network stack to settle before NTP query
  initNTP();
  
  // Wait for NTP sync (non-blocking after timeout)
  DEBUG_PRINTLN("[NTP] Waiting for sync...");
  unsigned long startNTPWait = millis();
  while (!isTimeSynced() && (millis() - startNTPWait < 20000)) {
    delay(500);
    DEBUG_PRINT(".");
  }
  DEBUG_PRINTLN();

  if (isTimeSynced()) {
    DEBUG_PRINTLN("[NTP] Synchronized successfully.");
    printLocalTime();
  } else {
    DEBUG_PRINTLN("[NTP] Timeout synchronization. Will retry in background.");
  }
  
  // Solid ON LED to show normal running status
  setLedMode(LED_MODE_ON);
}

unsigned long lastTimePrint = 0;

void loop() {
  // Check button press for reset config
  checkConfigButton();
  
  // Check WiFi status and handle background reconnection
  handleWiFiConnection();

  // Print local time every 2 seconds
  if (millis() - lastTimePrint > 2000) {
    lastTimePrint = millis();
    if (isTimeSynced()) {
      printLocalTime();
    } else {
      DEBUG_PRINTLN("[Time] Time not synchronized yet.");
    }
  }
  
  // Small delay to prevent watchdog reset issues
  delay(10);
}
