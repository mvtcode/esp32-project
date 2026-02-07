#include "config_manager.h"
#include "display.h"
#include "lunar_calendar.h"
#include "reset_button.h"
#include "weather.h"
#include "web_server.h"
#include "wifi_manager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

// Configuration data loaded from NVS
ConfigData deviceConfig;

bool isConfigMode = false; // Flag to indicate if in AP config mode

// Lunar calendar variables
int lunarDay = 0;
int lunarMonth = 0;
int lunarYear = 0;
int lastCalculatedDay = -1; // Cache: chỉ tính lại khi sang ngày mới

// Functions moved to display.cpp, wifi_manager.cpp, and weather.cpp

// Lunar calendar implementation moved to lunar_calendar.cpp
// WiFi connection moved to wifi_manager.cpp
// Weather fetching moved to weather.cpp

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== ESP32 LED Matrix Clock v2.0 ===");

  // 1. Initialize LED Matrix first
  initDisplay();

  // 2. Load configuration from NVS
  loadConfig(deviceConfig);

  // 3. Setup reset button
  setupResetButton();

  // 4. Check if configuration is valid
  if (!deviceConfig.isValid) {
    // No valid configuration - enter AP mode
    Serial.println("\n>>> NO CONFIGURATION FOUND <<<");
    Serial.println(">>> ENTERING AP CONFIGURATION MODE <<<\n");

    isConfigMode = true;

    // Display CONFIG MODE on LED matrix
    dma_display->clearScreen();
    dma_display->setTextSize(1);
    dma_display->setCursor(2, 4);
    dma_display->setTextColor(dma_display->color565(255, 128, 0));
    dma_display->print("CONFIG");
    dma_display->setCursor(2, 14);
    dma_display->print("MODE");
    dma_display->setCursor(2, 24);
    delay(1000);

    // Setup AP and web server
    setupAPMode();
    setupWebServer();
    setupCaptivePortal(); // Enable captive portal DNS redirect

    Serial.println("\nConnect to WiFi: ESP32-Clock-Config");
    Serial.println("Then visit: http://192.168.4.1");
    Serial.println("\nWaiting for configuration...\n");
  } else {
    // Valid configuration exists - normal operation
    Serial.println("\n>>> CONFIGURATION LOADED <<<");
    Serial.println(">>> STARTING NORMAL MODE <<<\n");

    isConfigMode = false;

    // Build weather API URL from stored coordinates
    String weatherApiUrl = "https://api.open-meteo.com/v1/forecast?latitude=";
    weatherApiUrl += String(deviceConfig.latitude, 4);
    weatherApiUrl += "&longitude=";
    weatherApiUrl += String(deviceConfig.longitude, 4);
    weatherApiUrl += "&current=temperature_2m,relative_humidity_2m";
    Serial.println("Weather API: " + weatherApiUrl);

    // Connect to WiFi
    connectWiFi(deviceConfig);

    // Initialize weather system
    if (WiFi.status() == WL_CONNECTED) {
      initWeather(weatherApiUrl);
    }
  }
}

void loop() {
  // Check reset button in all modes
  checkResetButton();

  // If in config mode, just blink indicator and wait
  if (isConfigMode) {
    static unsigned long lastBlink = 0;
    static bool blinkState = false;

    if (millis() - lastBlink > 500) {
      blinkState = !blinkState;
      lastBlink = millis();

      dma_display->clearScreen();
      dma_display->setTextSize(1);

      // Dòng 1: "Conf wifi:" - cố định, không nháy
      dma_display->setCursor(2, 4);
      dma_display->setTextColor(
          dma_display->color565(255, 128, 0)); // Màu cam cố định
      dma_display->print("Conf wifi:");

      // Dòng 2: "Clock-2026" - nháy
      dma_display->setCursor(2, 14);
      dma_display->setTextColor(
          blinkState ? dma_display->color565(255, 255, 255)
                     : dma_display->color565(64, 64, 64)); // Màu trắng nháy
      dma_display->print("Clock-2026");

      // Dòng 3: Hiển thị IP address với dấu chấm chỉ 1 pixel
      uint16_t ipColor = dma_display->color565(0, 255, 255);
      int currentX = 5;

      // "192"
      dma_display->setCursor(currentX, 24);
      dma_display->setTextColor(ipColor);
      dma_display->print("192");
      currentX += 18; // 3 chữ số * 6 pixels = 18px

      // Dấu chấm (1 pixel)
      dma_display->drawPixel(currentX, 30, ipColor);
      currentX += 2; // 1px cho chấm + 1px khoảng cách

      // "168"
      dma_display->setCursor(currentX, 24);
      dma_display->setTextColor(ipColor);
      dma_display->print("168");
      currentX += 18; // 3 chữ số * 6 pixels = 18px

      // Dấu chấm (1 pixel)
      dma_display->drawPixel(currentX, 30, ipColor);
      currentX += 2;

      // "4"
      dma_display->setCursor(currentX, 24);
      dma_display->setTextColor(ipColor);
      dma_display->print("4");
      currentX += 6; // 1 chữ số * 6 pixels = 6px

      // Dấu chấm (1 pixel)
      dma_display->drawPixel(currentX, 30, ipColor);
      currentX += 2;

      // "1"
      dma_display->setCursor(currentX, 24);
      dma_display->setTextColor(ipColor);
      dma_display->print("1");
    }

    handleDNS(); // Process DNS requests for captive portal
    delay(50);
    return;
  }

  // Normal operation mode - but check WiFi first
  // If WiFi is disconnected, try to reconnect
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastReconnectAttempt = 0;
    unsigned long now = millis();
    
    // Try to reconnect every 5 seconds
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      Serial.println("WiFi disconnected, attempting to reconnect...");
      connectWiFi(deviceConfig);
    }
    
    // Show "No WiFi" message while waiting
    dma_display->clearScreen();
    dma_display->setTextSize(1);
    dma_display->setCursor(2, 4);
    dma_display->setTextColor(dma_display->color565(255, 100, 0));
    dma_display->print("No WiFi");
    dma_display->setCursor(2, 14);
    dma_display->print("Press");
    dma_display->setCursor(2, 24);
    dma_display->print("BOOT");
    
    delay(100);
    return; // Skip rest of loop until WiFi is connected
  }
  
  struct tm timeinfo;
  bool hasTime = getLocalTime(&timeinfo);

  dma_display->clearScreen();

  if (hasTime) {
    // Phần vẽ đồng hồ (Giữ nguyên logic cũ của bạn)
    char hStr[3], mStr[3], sStr[3];
    sprintf(hStr, "%02d", timeinfo.tm_hour);
    sprintf(mStr, "%02d", timeinfo.tm_min);
    sprintf(sStr, "%02d", timeinfo.tm_sec);

    dma_display->setTextSize(2);
    // Vẽ Giờ
    dma_display->setCursor(1, 2);
    dma_display->setTextColor(hsvToRgb565(globalHue, 255, 255));
    dma_display->print(hStr);
    // Dấu :
    // dma_display->setCursor(21, 2);
    // dma_display->print(":");
    // Dấu : với hiệu ứng chạy vào nhau
    // Tính toán vị trí dựa trên millisecond trong giây (0-999ms)
    unsigned long ms = millis() % 1000; // Lặp lại mỗi giây
    float progress = ms / 1000.0;       // 0.0 -> 1.0

    // Tạo hiệu ứng đi xuống rồi lên (ping-pong)
    float offset;
    if (progress < 0.5) {
      // Nửa đầu: chạy vào nhau (0 -> 1)
      offset = progress * 2.0; // 0.0 -> 1.0
    } else {
      // Nửa sau: chạy ra (1 -> 0)
      offset = (1.0 - progress) * 2.0; // 1.0 -> 0.0
    }

    // Vị trí gốc của dấu : khi size=2 là tại (21, 2)
    // Với textSize(2), mỗi pixel có kích thước 2x2
    int baseX = 25;      // Tâm của dấu :
    int topDotY = 4;     // Vị trí gốc của chấm trên
    int bottomDotY = 12; // Vị trí gốc của chấm dưới
    int maxMove = 5;     // Khoảng cách tối đa di chuyển (pixels)

    // Tính vị trí hiện tại
    int currentTopY = topDotY + (int)(offset * maxMove);
    int currentBottomY = bottomDotY - (int)(offset * maxMove);

    // Vẽ hai chấm với kích thước 2x2 (giống textSize(2))
    uint16_t colonColor = hsvToRgb565(globalHue + 20, 255, 255);
    dma_display->fillRect(baseX, currentTopY, 2, 2, colonColor);
    dma_display->fillRect(baseX, currentBottomY, 2, 2, colonColor);
    // Vẽ Phút
    dma_display->setCursor(29, 2);
    dma_display->setTextColor(hsvToRgb565(globalHue + 40, 255, 255));
    dma_display->print(mStr);
    // Vẽ Giây (với gradient cho từng chữ số)
    dma_display->setTextSize(1);
    // Chữ số giây thứ nhất
    dma_display->setCursor(52, 9);
    dma_display->setTextColor(hsvToRgb565(globalHue + 80, 255, 255));
    dma_display->print(sStr[0]);
    // Chữ số giây thứ hai
    dma_display->setCursor(58, 9);
    dma_display->setTextColor(hsvToRgb565(globalHue + 100, 255, 255));
    dma_display->print(sStr[1]);

    // Vẽ dòng 2: Luân phiên giữa Thứ/Ngày và Nhiệt độ/Độ ẩm mỗi 5 giây
    dma_display->setTextSize(1);

    // Tính toán chế độ hiển thị dựa trên thời gian (5 giây mỗi chế độ, 3 modes)
    unsigned long currentTime = millis() / 5000; // Chia cho 5000ms = 5 giây
    int displayMode =
        currentTime % 3; // 0 = ngày/tháng, 1 = thời tiết, 2 = âm lịch

    // Thread-safe check for weather data availability
    float tempDisplay = 0.0;
    int humDisplay = 0;
    bool showWeather = getWeatherData(tempDisplay, humDisplay);

    if (displayMode == 1 && showWeather) {
      // Hiển thị Nhiệt độ và Độ ẩm với nhãn T và H
      // Format: "T 23.1°C  H 79%" với gradient colors

      // Tách phần nguyên và phần thập phân của nhiệt độ (using thread-safe
      // copies)
      int tempInt = (int)tempDisplay;
      int tempDec =
          (int)((tempDisplay - tempInt) * 10); // Lấy 1 chữ số thập phân

      char tempIntStr[5], tempDecStr[2], humStr[8];
      sprintf(tempIntStr, "%d", tempInt);
      sprintf(tempDecStr, "%d", tempDec);
      sprintf(humStr, "%d%%", humDisplay);

      // Bắt đầu từ vị trí cố định
      int currentX = 1; // Bắt đầu từ pixel 1

      // Vẽ "T" với gradient
      // dma_display->setCursor(currentX, 23);
      // dma_display->setTextColor(hsvToRgb565(globalHue + currentX * 2, 255,
      // 255)); dma_display->print("T");
      currentX += 4;

      // Vẽ phần nguyên (23) - từng chữ số với gradient
      currentX -= 1;
      for (int i = 0; i < strlen(tempIntStr); i++) {
        dma_display->setCursor(currentX, 23);
        dma_display->setTextColor(
            hsvToRgb565(globalHue + currentX * 2, 255, 255));
        dma_display->print(tempIntStr[i]);
        currentX += 6;
      }

      // Vẽ dấu chấm (.) - chỉ 1 pixel
      dma_display->drawPixel(currentX, 29,
                             hsvToRgb565(globalHue + currentX * 2, 255, 255));
      currentX += 2;

      // Vẽ phần thập phân (1)
      dma_display->setCursor(currentX, 23);
      dma_display->setTextColor(
          hsvToRgb565(globalHue + currentX * 2, 255, 255));
      dma_display->print(tempDecStr);
      currentX += strlen(tempDecStr) * 6;

      // Vẽ ký tự độ (°) - vòng tròn nhỏ
      dma_display->drawCircle(currentX + 1, 24, 1,
                              hsvToRgb565(globalHue + currentX * 2, 255, 255));
      currentX += 3;

      // Vẽ "C"
      dma_display->setCursor(currentX, 23);
      dma_display->setTextColor(
          hsvToRgb565(globalHue + currentX * 2, 255, 255));
      dma_display->print("C");
      currentX += 6 + 6; // C (6px) + space (6px)

      // Vẽ "H"
      currentX = 40;
      // dma_display->setCursor(currentX, 23);
      // dma_display->setTextColor(hsvToRgb565(globalHue + currentX * 2, 255,
      // 255)); dma_display->print("H"); currentX += 6; // H (6px)

      // Vẽ độ ẩm - từng ký tự với gradient
      for (int i = 0; i < strlen(humStr); i++) {
        dma_display->setCursor(currentX, 23);
        dma_display->setTextColor(
            hsvToRgb565(globalHue + currentX * 2, 255, 255));
        dma_display->print(humStr[i]);
        currentX += 6;
      }
    } else if (displayMode == 2) {
      // Mode 2: Hiển thị Âm lịch
      // Chỉ tính lại khi sang ngày mới (cache)
      if (lastCalculatedDay != timeinfo.tm_mday) {
        solarToLunar(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
                     timeinfo.tm_mday, lunarDay, lunarMonth, lunarYear);
        lastCalculatedDay = timeinfo.tm_mday;
      }

      char lunarStr[16];
      sprintf(lunarStr, "AL%02d/%02d/%02d", lunarDay, lunarMonth,
              lunarYear % 100);

      // Hiển thị với gradient
      int currentX = 2;

      for (int i = 0; i < strlen(lunarStr); i++) {
        dma_display->setCursor(currentX, 23);
        dma_display->setTextColor(
            hsvToRgb565(globalHue + currentX * 2, 255, 255));
        dma_display->print(lunarStr[i]);
        currentX += 6;
      }
    } else {
      // Hiển thị Thứ và Ngày/Tháng - từng ký tự với gradient theo vị trí X

      if (timeinfo.tm_wday == 0) {
        // Chủ nhật - hiển thị "CNhật" với gradient từng ký tự
        dma_display->setCursor(2, 23);
        dma_display->setTextColor(hsvToRgb565(globalHue + 2 * 2, 255, 255));
        dma_display->print("C");

        dma_display->setCursor(8, 23);
        dma_display->setTextColor(hsvToRgb565(globalHue + 8 * 2, 255, 255));
        dma_display->print("N");

        dma_display->setCursor(14, 23);
        dma_display->setTextColor(hsvToRgb565(globalHue + 14 * 2, 255, 255));
        dma_display->print("h");

        // Vẽ chữ "ậ" tùy chỉnh
        drawACircumflexDotBelow(20, 23,
                                hsvToRgb565(globalHue + 20 * 2, 255, 255));

        dma_display->setCursor(26, 23);
        dma_display->setTextColor(hsvToRgb565(globalHue + 26 * 2, 255, 255));
        dma_display->print("t");
      } else {
        // Thứ 2-7 - vẽ "Thứ" + số với gradient theo vị trí
        // Vẽ "Thứ" tùy chỉnh - màu dựa trên vị trí x=2
        drawThu(2, 23, hsvToRgb565(globalHue + 2 * 2, 255, 255));

        dma_display->setCursor(18, 23);
        dma_display->setTextColor(hsvToRgb565(globalHue + 18 * 2, 255, 255));
        dma_display->print(timeinfo.tm_wday + 1); // 1->2, 2->3, ..., 6->7

        // Dấu phẩy sau thứ - vị trí x=24
        dma_display->setCursor(24, 23);
        dma_display->setTextColor(hsvToRgb565(globalHue + 24 * 2, 255, 255));
        dma_display->print(",");
      }

      // Hiển thị ngày/tháng DD/MM ở vị trí cố định với gradient từng ký tự
      char dateStr[12];
      sprintf(dateStr, "%02d/%02d", timeinfo.tm_mday, timeinfo.tm_mon + 1);
      int currentX = 34; // Vị trí cố định
      for (int i = 0; i < strlen(dateStr); i++) {
        dma_display->setCursor(currentX, 23);
        dma_display->setTextColor(
            hsvToRgb565(globalHue + currentX * 2, 255, 255));
        dma_display->print(dateStr[i]);
        currentX += 6;
      }
    }
  } else {
    // No time available yet - WiFi connection is being handled by connectWiFi()
    // Just wait a bit before next iteration
    delay(100);
  }

  // Weather updates now handled by background task - no blocking calls here!

  globalHue += 1;
  delay(100);
}