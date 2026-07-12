#include "wifi_manager.h"
#include "display.h"
#include "reset_button.h"
#include "vietnamese_helper.h"
#include <Arduino.h>
#include <WiFi.h>

// Connect to WiFi using stored configuration with retry mechanism
void connectWiFi(const ConfigData& config) {
  static int retryCount = 0;
  static unsigned long lastRetryTime = 0;

  Serial.println("Connecting WiFi...");
  Serial.printf("SSID: %s\n", config.ssid);
  Serial.printf("Retry attempt: %d\n", retryCount);

  char ssidLine[64];
  snprintf(ssidLine, sizeof(ssidLine), "WiFi: %s", config.ssid);

  // Show "Waiting WiFi..." on LED display
  virtual_display->fillScreen(0);
  virtual_display->setFont(&Verdana_Vietnamese10pt);
  virtual_display->setTextColor(virtual_display->color565(255, 165, 0)); // Orange color
  virtual_display->setCursor(6, 28);
  virtual_display->print(utf8ToCustom("Đang kết nối..."));
  virtual_display->setTextColor(virtual_display->color565(0, 255, 255)); // Cyan color for SSID
  virtual_display->setCursor(6, 50);
  virtual_display->print(utf8ToCustom(ssidLine));
  virtual_display->setTextColor(virtual_display->color565(200, 200, 200));
  virtual_display->setCursor(6, 72);
  virtual_display->print(utf8ToCustom("Vui lòng đợi..."));
  dma_display->flipDMABuffer(); // Swap buffer to display the connection screen

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
      virtual_display->fillScreen(0);
      dma_display->flipDMABuffer();
    } else {
      virtual_display->fillScreen(0);
      virtual_display->setFont(&Verdana_Vietnamese10pt);
      virtual_display->setTextColor(virtual_display->color565(255, 165, 0)); // Orange
      virtual_display->setCursor(6, 28);
      virtual_display->print(utf8ToCustom("Đang kết nối..."));
      virtual_display->setTextColor(virtual_display->color565(0, 255, 255)); // Cyan
      virtual_display->setCursor(6, 50);
      virtual_display->print(utf8ToCustom(ssidLine));
      virtual_display->setTextColor(virtual_display->color565(200, 200, 200));
      virtual_display->setCursor(6, 72);
      virtual_display->print(utf8ToCustom("Vui lòng đợi..."));
      dma_display->flipDMABuffer();
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

    // Show success message with IP address
    showIPAddress(WiFi.localIP());
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
    virtual_display->fillScreen(0);
    virtual_display->setFont(&Verdana_Vietnamese10pt);
    virtual_display->setTextColor(virtual_display->color565(255, 0, 0)); // Red
    virtual_display->setCursor(10, 30);
    virtual_display->print(utf8ToCustom("Kết nối Thất Bại"));
    
    virtual_display->setTextColor(virtual_display->color565(255, 100, 0));
    virtual_display->setCursor(10, 50);
    char retryMsg[32];
    sprintf(retryMsg, "Thử lại lần %d...", retryCount);
    virtual_display->print(utf8ToCustom(retryMsg));

    virtual_display->setTextColor(virtual_display->color565(200, 200, 200));
    virtual_display->setCursor(10, 70);
    virtual_display->print(utf8ToCustom("Ấn BOOT để cấu hình"));
    dma_display->flipDMABuffer();
    
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

// Hiển thị IP address của đồng hồ lên màn hình matrix
void showIPAddress(IPAddress ip) {
  String ipStr = ip.toString();
  int ipWidth = 0;
  for (int i = 0; i < ipStr.length(); i++) {
    if (ipStr[i] == '.') {
      ipWidth += 4;
    } else {
      ipWidth += 8;
    }
  }
  int ipX = (128 - ipWidth) / 2;
  if (ipX < 0) ipX = 0;

  unsigned long displayStart = millis();
  while (millis() - displayStart < 3000) {
    virtual_display->fillScreen(0);
    virtual_display->setFont(&Verdana_Vietnamese10pt);
    
    // Line 1: WiFi Kết Nối OK!
    virtual_display->setTextColor(virtual_display->color565(0, 255, 0)); // Green
    virtual_display->setCursor(12, 35);
    virtual_display->print(utf8ToCustom("WiFi Kết Nối OK!"));
    
    // Line 2: IP Address
    virtual_display->setTextColor(virtual_display->color565(0, 255, 255)); // Cyan
    virtual_display->setCursor(ipX, 65);
    virtual_display->print(ipStr);
    
    dma_display->flipDMABuffer();
    
    checkResetButton();
    delay(50);
  }
}
