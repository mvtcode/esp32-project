#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include "config_manager.h"
#include "wifi_controller.h"
#include "ntp_manager.h"
#include "button_handler.h"
#include "debug.h"

// Define LED matrix hardware type and pins
// Standard Chinese 4-in-1 modules are FC16_HW
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

#define DATA_PIN 1  // GPIO1 (TX) -> DIN
#define CLK_PIN  3  // GPIO3 (RX) -> CLK
#define CS_PIN   0  // GPIO0       -> CS

// Software SPI Parola object
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// Scroll display control
static char scrollMessage[40] = "";
static bool isScrolling = false;

// Display message updater (called by WiFi controller / main)
void updateDisplayMessage(const char* msg) {
  P.setTextAlignment(PA_CENTER);
  P.print(msg);
}

void startScrollMessage(const char* msg) {
  strlcpy(scrollMessage, msg, sizeof(scrollMessage));
  P.displayText(scrollMessage, PA_LEFT, 50, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  isScrolling = true;
}

void setup() {
  // 1. Release TX/RX pins back to GPIO for LED Matrix control
  Serial.end();
  
  // 2. Initialize Parola Display
  P.begin();
  P.setIntensity(3); // Default boot brightness
  updateDisplayMessage("BOOT");
  delay(1000);

  // 3. Initialize components
  initButton();
  initConfig();
  
  if (loadConfig()) {
    // Apply loaded brightness setting (0-15)
    P.setIntensity(systemConfig.brightness);
  }
  
  // 4. Initialize WiFi
  initWiFi();
  
  // Show connection success and scroll local IP
  char ipBuf[40];
  snprintf(ipBuf, sizeof(ipBuf), "IP: %s", WiFi.localIP().toString().c_str());
  startScrollMessage(ipBuf);
  
  // Wait for IP scroll to finish
  while (isScrolling) {
    if (P.displayAnimate()) {
      isScrolling = false;
    }
    delay(15);
  }
  
  // 5. Initialize NTP
  updateDisplayMessage("SYNC");
  delay(1500); // Allow network stack to settle before NTP query
  initNTP();
  
  // Wait for NTP sync (up to 20 seconds)
  unsigned long startNTPWait = millis();
  while (!isTimeSynced() && (millis() - startNTPWait < 20000)) {
    delay(500);
  }
  
  if (isTimeSynced()) {
    updateDisplayMessage("OK");
    delay(1000);
  } else {
    updateDisplayMessage("FAIL");
    delay(1000);
  }
  
  P.setTextAlignment(PA_CENTER);
}

static unsigned long lastClockUpdate = 0;
static bool colonBlink = false;
static char clockText[10] = "";

void updateClockDisplay() {
  time_t now = time(nullptr);
  struct tm* timeInfo = localtime(&now);
  int hour = timeInfo->tm_hour;
  
  // Convert 12h format if configured
  if (!systemConfig.is24h) {
    if (hour > 12) hour -= 12;
    else if (hour == 0) hour = 12;
  }
  
  if (colonBlink) {
    snprintf(clockText, sizeof(clockText), "%02d:%02d", hour, timeInfo->tm_min);
  } else {
    snprintf(clockText, sizeof(clockText), "%02d %02d", hour, timeInfo->tm_min);
  }
  
  P.print(clockText);
}

void loop() {
  // Check button for factory reset (GPIO2)
  checkConfigButton();
  
  // Check WiFi and auto-reconnect
  handleWiFiConnection();
  
  // Handle text scrolling or clock display
  if (isScrolling) {
    if (P.displayAnimate()) {
      isScrolling = false;
      P.setTextAlignment(PA_CENTER);
    }
  } else {
    // Normal clock operation
    if (millis() - lastClockUpdate >= 1000) {
      lastClockUpdate = millis();
      colonBlink = !colonBlink;
      if (isTimeSynced()) {
        updateClockDisplay();
      } else {
        updateDisplayMessage("SYNC");
      }
    }
  }
  
  // Small delay to keep the ESP8266 background tasks running stably
  delay(10);
}
