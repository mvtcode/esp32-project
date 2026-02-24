#include "wifi_manager.h"
#include "display.h"
#include "reset_button.h"
#include <Arduino.h>
#include <WiFi.h>

// Connect to WiFi using stored configuration with retry mechanism
void connectWiFi(const ConfigData& config) {
  static int retryCount = 0;
  static unsigned long lastRetryTime = 0;

  Serial.println("Connecting WiFi...");
  Serial.printf("SSID: %s\n", config.ssid);
  Serial.printf("Retry attempt: %d\n", retryCount);

  // Show "Waiting WiFi..." on LED display
  dma_display->clearScreen();
  dma_display->setTextSize(1);
  dma_display->setCursor(2, 4);
  dma_display->setTextColor(dma_display->color565(255, 165, 0)); // Orange color
  dma_display->print("Waiting");
  dma_display->setCursor(4, 20);
  dma_display->print("WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(config.ssid, config.password);

  // Calculate timeout based on retry count (exponential backoff)
  // First attempt: 10s, then 11s, 12s, 13s, etc. (max 30s)
  int timeoutSeconds = min(10 + retryCount, 30);
  int timeout = 0;
  int maxTimeout = timeoutSeconds * 2; // *2 because delay is 500ms

  Serial.printf("Timeout: %d seconds\n", timeoutSeconds);
  Serial.println("(Press BOOT button to enter config mode)");

  while (WiFi.status() != WL_CONNECTED && timeout < maxTimeout) {
    delay(500);
    Serial.print(".");
    timeout++;

    // CHECK RESET BUTTON during WiFi connection!
    checkResetButton();

    // Blink display every second to show activity
    if (timeout % 2 == 0) {
      dma_display->clearScreen();
    } else {
      dma_display->setCursor(2, 4);
      dma_display->setTextColor(dma_display->color565(255, 165, 0));
      dma_display->print("Waiting");
      dma_display->setCursor(4, 20);
      dma_display->print("WiFi...");
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    // Configure time with multiple NTP servers for better reliability
    // 1. vn.pool.ntp.org (Vietnam - lowest latency)
    // 2. pool.ntp.org (Global - backup)
    // 3. time.google.com (Google - reliable backup)
    configTime(7 * 3600, 0, "vn.pool.ntp.org", "pool.ntp.org", "time.google.com");

    // Reset retry count on successful connection
    retryCount = 0;

    // Show success message briefly
    dma_display->clearScreen();
    dma_display->setCursor(2, 4);
    dma_display->setTextColor(dma_display->color565(0, 255, 0)); // Green
    dma_display->print("WiFi OK!");
    delay(1000);
  } else {
    Serial.println("\nWiFi Failed!");
    retryCount++;
    
    // Limit retry attempts to prevent infinite loop
    if (retryCount > 10) {
      Serial.println("Too many retries! Hold BOOT button to reconfigure...");
      retryCount = 0; // Reset counter but keep trying
    }
    
    lastRetryTime = millis();

    // Show retry message
    dma_display->clearScreen();
    dma_display->setTextSize(1);
    dma_display->setCursor(2, 4);
    dma_display->setTextColor(dma_display->color565(255, 0, 0)); // Red
    dma_display->print("WiFi");
    dma_display->setCursor(2, 14);
    dma_display->print("Failed");
    dma_display->setCursor(2, 24);
    dma_display->printf("Retry:%d", retryCount);
    
    // Show hint to press BOOT button
    dma_display->setTextSize(1);
    dma_display->setCursor(2, 4);
    dma_display->setTextColor(dma_display->color565(255, 100, 0));
    dma_display->print("Press");
    dma_display->setCursor(2, 14);
    dma_display->print("BOOT");
    dma_display->setCursor(2, 24);
    dma_display->print("to cfg");
    
    // Wait and check button during delay
    for (int i = 0; i < 20; i++) { // 20 * 100ms = 2 seconds
      delay(100);
      checkResetButton();
    }

    // Disconnect before retry
    WiFi.disconnect();
    delay(500);
    
    // DON'T use recursive call - just return and let loop() handle retry
    // This prevents stack overflow and allows reset button to work
    return;
  }
}
