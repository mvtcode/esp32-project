#include "config_manager.h"
#include "display.h"
#include "indoor_sensor.h"
#include "lunar_calendar.h"
#include "reset_button.h"
#include "rtc_manager.h"
#include "weather.h"
#include "web_server.h"
#include "wifi_manager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

// Custom Vietnamese fonts and mapper
#include "Verdana_Vietnamese8pt.h"
#include "Verdana_Vietnamese10pt.h"
#include "Verdana_Bold14pt.h"
#include "vietnamese_helper.h"

// Configuration data loaded from NVS
ConfigData deviceConfig;

bool isConfigMode = false; // Flag to indicate if in AP config mode

// Lunar calendar variables
int lunarDay = 0;
int lunarMonth = 0;
int lunarYear = 0;
int lastCalculatedDay = -1; // Cache: chỉ tính lại khi sang ngày mới

// Helper function to get day of week string in Vietnamese
String getDayOfWeekStr(int wday) {
  switch (wday) {
    case 0: return "CN";
    case 1: return "T.Hai";
    case 2: return "T.Ba";
    case 3: return "T.Tư";
    case 4: return "T.Năm";
    case 5: return "T.Sáu";
    case 6: return "T.Bảy";
    default: return "";
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== ESP32 LED Matrix Clock 128x96 ===");

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

    // Display AP mode status on virtual screen
    virtual_display->fillScreen(virtual_display->color565(0, 0, 0));
    virtual_display->setTextSize(1);
    virtual_display->setTextColor(virtual_display->color565(255, 128, 0));
    virtual_display->setCursor(4, 20);
    virtual_display->print("CONFIG MODE");
    virtual_display->setCursor(4, 40);
    virtual_display->print("AP: Clock-2026");
    virtual_display->setCursor(4, 60);
    virtual_display->print("IP: 192.168.4.1");
    delay(1000);

    // Setup AP and web server
    setupAPMode();
    setupWebServer();
    setupCaptivePortal(); // Enable captive portal DNS redirect

    Serial.println("\nConnect to WiFi: Clock-2026");
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

    // Initialize RTC
    initRTC();
    
    // Initialize indoor sensor
    initIndoorSensor();
    
    // Start indoor sensor background task
    startIndoorSensorTask();
    
    // Initialize weather system
    if (WiFi.status() == WL_CONNECTED) {
      initWeather(weatherApiUrl);
      
      // Sync RTC from NTP
      Serial.println("Syncing time from NTP...");
      struct tm timeinfo;
      if (getLocalTime(&timeinfo, 5000)) { // 5 second timeout
        syncRTCFromNTP(timeinfo);
        Serial.println("RTC synced from NTP successfully");
      } else {
        Serial.println("NTP sync timeout, will retry later");
      }
    }
    
    // Apply initial brightness from configuration
    Serial.printf("\n>>> Applying Initial Brightness: %d%% <<<\n", deviceConfig.brightness);
    setDisplayBrightness(deviceConfig.brightness);
    Serial.println(">>> Brightness Applied Successfully <<<\n");
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

      virtual_display->fillScreen(virtual_display->color565(0, 0, 0));
      virtual_display->setFont(&Verdana_Vietnamese8pt);
      
      // Line 1: Header (orange)
      virtual_display->setTextColor(virtual_display->color565(255, 128, 0));
      virtual_display->setCursor(4, 22);
      virtual_display->print(utf8ToCustom("Cấu hình WiFi:"));

      // Line 2: AP name (blinking white/gray)
      virtual_display->setTextColor(blinkState ? virtual_display->color565(255, 255, 255) : virtual_display->color565(64, 64, 64));
      virtual_display->setCursor(4, 46);
      virtual_display->print(utf8ToCustom("WiFi: Clock-2026"));

      // Line 3: IP address (cyan)
      virtual_display->setTextColor(virtual_display->color565(0, 255, 255));
      virtual_display->setCursor(4, 70);
      virtual_display->print(utf8ToCustom("Web: 192.168.4.1"));
    }

    handleDNS(); // Process DNS requests for captive portal
    delay(50);
    return;
  }

  // Normal operation mode - check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastReconnectAttempt = 0;
    unsigned long now = millis();
    
    // Try to reconnect every 5 seconds
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      Serial.println("WiFi disconnected, attempting to reconnect...");
      connectWiFi(deviceConfig);
    }
    
    // Show premium "No WiFi" reconnection status screen
    static int dots = 0;
    static unsigned long lastDot = 0;
    if (millis() - lastDot > 500) {
      dots = (dots + 1) % 4;
      lastDot = millis();
    }

    virtual_display->fillScreen(virtual_display->color565(0, 0, 0));
    virtual_display->setFont(&Verdana_Vietnamese8pt);
    
    virtual_display->setTextColor(virtual_display->color565(255, 100, 0));
    virtual_display->setCursor(10, 36);
    virtual_display->print(utf8ToCustom("Mất kết nối WiFi"));
    
    virtual_display->setTextColor(virtual_display->color565(200, 200, 200));
    virtual_display->setCursor(10, 56);
    virtual_display->print(utf8ToCustom("Đang kết nối lại"));
    
    virtual_display->setCursor(10, 76);
    String dotStr = "";
    for (int d = 0; d < dots; d++) dotStr += ".";
    virtual_display->print(dotStr);
    
    delay(100);
    return;
  }
  
  // Get time from NTP or RTC fallback
  struct tm timeinfo;
  bool hasTime;
  static unsigned long lastRTCSync = 0;
  
  if (WiFi.status() == WL_CONNECTED) {
    hasTime = getLocalTime(&timeinfo, 100);
    
    if (hasTime) {
      // Sync RTC from NTP every hour
      if (millis() - lastRTCSync > 3600000) {
        syncRTCFromNTP(timeinfo);
        lastRTCSync = millis();
      }
    } else {
      hasTime = getRTCTime(timeinfo);
    }
  } else {
    hasTime = getRTCTime(timeinfo);
  }

  // Apply sleep mode logic if enabled
  if (deviceConfig.sleepEnabled && hasTime) {
    uint16_t currentMinute = timeinfo.tm_hour * 60 + timeinfo.tm_min;
    
    static unsigned long lastDebugPrint = 0;
    bool shouldPrint = (millis() - lastDebugPrint > 10000);
    
    bool inSleepPeriod = false;
    if (deviceConfig.sleepStartMinute <= deviceConfig.sleepEndMinute) {
      inSleepPeriod = (currentMinute >= deviceConfig.sleepStartMinute && 
                       currentMinute < deviceConfig.sleepEndMinute);
    } else {
      inSleepPeriod = (currentMinute >= deviceConfig.sleepStartMinute || 
                       currentMinute < deviceConfig.sleepEndMinute);
    }
    
    if (inSleepPeriod) {
      if (deviceConfig.sleepBrightness == 0) {
        virtual_display->fillScreen(0);
        delay(100);
        if (shouldPrint) {
          lastDebugPrint = millis();
        }
        return; // Skip rendering
      } else {
        setDisplayBrightness(deviceConfig.sleepBrightness);
      }
    } else {
      setDisplayBrightness(deviceConfig.brightness);
    }
    
    if (shouldPrint) {
      lastDebugPrint = millis();
    }
  } else if (hasTime) {
    setDisplayBrightness(deviceConfig.brightness);
  }

  // Non-blocking render timer (~28 FPS / 35ms tick)
  static unsigned long lastRender = 0;
  if (millis() - lastRender >= 35) {
    lastRender = millis();
    
    // Increment global hue for gradient transitions
    globalHue += 1;
    
    // Clear screen
    virtual_display->fillScreen(virtual_display->color565(0, 0, 0));

    if (hasTime) {
      // ==========================================
      // ROW 1: Time (HH:MM:SS) (Y: 0..31)
      // ==========================================
      char hStr[3], mStr[3], sStr[3];
      sprintf(hStr, "%02d", timeinfo.tm_hour);
      sprintf(mStr, "%02d", timeinfo.tm_min);
      sprintf(sStr, "%02d", timeinfo.tm_sec);

      virtual_display->setFont(&Verdana_Bold14pt);
      
      // Draw Hour (X: 16)
      virtual_display->setCursor(16, 22);
      virtual_display->setTextColor(hsvToRgb565(globalHue, 255, 255));
      virtual_display->print(hStr);

      // Draw Minute (X: 54)
      virtual_display->setCursor(54, 22);
      virtual_display->setTextColor(hsvToRgb565(globalHue + 40, 255, 255));
      virtual_display->print(mStr);

      // Draw Second (X: 92)
      virtual_display->setCursor(92, 22);
      virtual_display->setTextColor(hsvToRgb565(globalHue + 80, 255, 255));
      virtual_display->print(sStr);

      // Draw Blinking/Animating Colons (X: 46 and 84)
      unsigned long ms = millis() % 1000;
      float progress = ms / 1000.0;
      float offset = (progress < 0.5) ? (progress * 2.0) : ((1.0 - progress) * 2.0); // 0.0 -> 1.0 -> 0.0
      int maxMove = 4;
      
      int currentTopY = 9 + (int)(offset * maxMove);
      int currentBottomY = 17 - (int)(offset * maxMove);
      
      uint16_t colonColor = hsvToRgb565(globalHue + 20, 255, 255);
      virtual_display->fillRect(46, currentTopY, 2, 2, colonColor);
      virtual_display->fillRect(46, currentBottomY, 2, 2, colonColor);
      virtual_display->fillRect(84, currentTopY, 2, 2, colonColor);
      virtual_display->fillRect(84, currentBottomY, 2, 2, colonColor);

      // ==========================================
      // ROW 2: Calendar (Solar & Lunar) (Y: 32..47)
      // ==========================================
      virtual_display->setFont(&Verdana_Vietnamese8pt);

      // 1. Solar Date (Left Side)
      String solarStr = getDayOfWeekStr(timeinfo.tm_wday) + ", " + String(timeinfo.tm_mday) + "/" + String(timeinfo.tm_mon + 1);
      virtual_display->setTextColor(hsvToRgb565(globalHue + 10, 255, 255));
      virtual_display->setCursor(2, 43);
      virtual_display->print(utf8ToCustom(solarStr));

      // 2. Lunar Date (Right Side)
      if (lastCalculatedDay != timeinfo.tm_mday) {
        solarToLunar(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
                     timeinfo.tm_mday, lunarDay, lunarMonth, lunarYear);
        lastCalculatedDay = timeinfo.tm_mday;
      }
      char lunarStr[16];
      sprintf(lunarStr, "AL:%02d/%02d", lunarDay, lunarMonth);
      virtual_display->setTextColor(hsvToRgb565(globalHue + 40, 255, 255));
      virtual_display->setCursor(74, 43);
      virtual_display->print(utf8ToCustom(lunarStr));

      // ==========================================
      // ROW 3: Sensors & Weather (Y: 48..63)
      // ==========================================
      float tempOutdoor = 0.0;
      int humOutdoor = 0;
      bool showWeather = getWeatherData(tempOutdoor, humOutdoor);
      
      float tempIndoor = 0.0;
      int humIndoor = 0;
      bool showIndoor = getIndoorData(tempIndoor, humIndoor);

      // 1. Indoor Data (Left Side)
      uint16_t indoorColor = hsvToRgb565(globalHue + 60, 255, 255);
      drawIndoorIcon(2, 50, indoorColor);

      char inTempStr[16];
      if (showIndoor) {
        sprintf(inTempStr, "%.1f", tempIndoor);
      } else {
        sprintf(inTempStr, "--.-");
      }
      virtual_display->setTextColor(indoorColor);
      virtual_display->setCursor(11, 59);
      virtual_display->print(utf8ToCustom(inTempStr));
      
      int curX = virtual_display->getCursorX();
      virtual_display->drawCircle(curX + 1, 51, 1, indoorColor); // Degree circle
      
      virtual_display->setCursor(curX + 4, 59);
      char inHumStr[16];
      if (showIndoor) {
        sprintf(inHumStr, "C %d%%", humIndoor);
      } else {
        sprintf(inHumStr, "C --%%");
      }
      virtual_display->print(utf8ToCustom(inHumStr));

      // 2. Outdoor Data (Right Side)
      uint16_t outdoorColor = hsvToRgb565(globalHue + 100, 255, 255);
      drawOutdoorIcon(72, 50, outdoorColor);

      char outTempStr[16];
      if (showWeather) {
        sprintf(outTempStr, "%.1f", tempOutdoor);
      } else {
        sprintf(outTempStr, "--.-");
      }
      virtual_display->setTextColor(outdoorColor);
      virtual_display->setCursor(81, 59);
      virtual_display->print(utf8ToCustom(outTempStr));
      
      int curX2 = virtual_display->getCursorX();
      virtual_display->drawCircle(curX2 + 1, 51, 1, outdoorColor); // Degree circle
      
      virtual_display->setCursor(curX2 + 4, 59);
      char outHumStr[16];
      if (showWeather) {
        sprintf(outHumStr, "C %d%%", humOutdoor);
      } else {
        sprintf(outHumStr, "C --%%");
      }
      virtual_display->print(utf8ToCustom(outHumStr));

      // ==========================================
      // ROW 4: Marquee Scrolling Text (Y: 64..95)
      // ==========================================
      static float scrollX = 128.0;
      scrollX -= 0.8; // Adjust speed (pixels per frame)

      static String cachedRawMarquee = "";
      static String cachedCustomMsg = "";
      static uint16_t cachedTextWidth = 0;

      String rawMarquee = String(deviceConfig.marqueeText);
      if (rawMarquee != cachedRawMarquee) {
        cachedRawMarquee = rawMarquee;
        cachedCustomMsg = utf8ToCustom(rawMarquee);
        
        // Measure width using the correct font
        virtual_display->setFont(&Verdana_Vietnamese10pt);
        int16_t bx, by;
        uint16_t bw, bh;
        virtual_display->getTextBounds(cachedCustomMsg, 0, 84, &bx, &by, &bw, &bh);
        cachedTextWidth = bw;
      }

      if (scrollX < -cachedTextWidth) {
        scrollX = 128.0; // Reset scroll position to right edge
      }

      virtual_display->setFont(&Verdana_Vietnamese10pt);
      virtual_display->setTextColor(hsvToRgb565(globalHue + 120, 255, 255));
      virtual_display->setCursor((int)scrollX, 84);
      virtual_display->print(cachedCustomMsg);

    } else {
      // Offline / waiting for time sync
      virtual_display->setFont(&Verdana_Vietnamese8pt);
      virtual_display->setTextColor(virtual_display->color565(255, 255, 0));
      virtual_display->setCursor(10, 48);
      virtual_display->print(utf8ToCustom("Đang đồng bộ thời gian..."));
    }
  }
}