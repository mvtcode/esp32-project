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
#include "Verdana_Vietnamese10pt.h"
#include "Verdana_Vietnamese12pt.h"
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

// Helper function to calculate exact width of custom-mapped Vietnamese text
int16_t getCustomTextWidth(const String &str, const GFXfont *font) {
  if (!font) return str.length() * 6; // Default fallback
  int16_t totalWidth = 0;
  GFXglyph *glyphs = (GFXglyph *)pgm_read_ptr(&font->glyph);
  uint8_t first = pgm_read_byte(&font->first);
  uint8_t last = pgm_read_byte(&font->last);
  
  for (int i = 0; i < str.length(); i++) {
    uint8_t c = (uint8_t)str[i];
    if (c >= first && c <= last) {
      GFXglyph *glyph = &glyphs[c - first];
      totalWidth += pgm_read_byte(&glyph->xAdvance);
    }
  }
  return totalWidth;
}

// Helper function to convert WMO weather code to a short friendly Vietnamese text
String getWeatherDesc(int code) {
  switch (code) {
    case 0: return "Trời quang";
    case 1: return "Ít mây";
    case 2: return "Có mây";
    case 3: return "Nhiều mây";
    case 45:
    case 48: return "Sương mù";
    case 51:
    case 53:
    case 55: return "Mưa phùn";
    case 56:
    case 57: return "Mưa lạnh";
    case 61:
    case 63: return "Có mưa";
    case 65: return "Mưa to";
    case 66:
    case 67: return "Mưa băng";
    case 71:
    case 73:
    case 75: return "Tuyết rơi";
    case 77: return "Hạt tuyết";
    case 80:
    case 81:
    case 82: return "Mưa rào";
    case 85:
    case 86: return "Mưa tuyết";
    case 95: return "Dông bão";
    case 96:
    case 99: return "Mưa đá";
    default: return "Cập nhật";
  }
}


// Helper function to draw status icons at the top-right of the screen
void drawStatusIcons() {
  int baseX = 104;
  int baseY = 1;

  // 2. WiFi RSSI Signal Icon (X: 103..111, Y: 1..6)
  uint16_t wifiColor = virtual_display->color565(60, 60, 60); // dark gray default
  int rssi = 0;
  int numBars = 0;
  bool wifiConnected = (WiFi.status() == WL_CONNECTED);
  if (wifiConnected) {
    rssi = WiFi.RSSI();
    if (rssi >= -55) {
      numBars = 5;
      wifiColor = virtual_display->color565(0, 255, 0); // Green
    } else if (rssi >= -65) {
      numBars = 4;
      wifiColor = virtual_display->color565(128, 255, 0); // Light green
    } else if (rssi >= -75) {
      numBars = 3;
      wifiColor = virtual_display->color565(255, 255, 0); // Yellow
    } else if (rssi >= -85) {
      numBars = 2;
      wifiColor = virtual_display->color565(255, 128, 0); // Orange
    } else {
      numBars = 1;
      wifiColor = virtual_display->color565(255, 0, 0); // Red
    }
  }

  // Draw 5 bars
  for (int b = 1; b <= 5; b++) {
    uint16_t color = (b <= numBars) ? wifiColor : virtual_display->color565(40, 40, 40);
    int barHeight = b;
    int barX = 103 + (b - 1) * 2;
    int barY = baseY + 5 - barHeight;
    virtual_display->drawFastVLine(barX, barY, barHeight, color);
  }

  // 3. Time Sync Icon (X: 115..119, Y: 1..6)
  uint16_t timeColor = (WiFi.status() == WL_CONNECTED) ? virtual_display->color565(0, 255, 0) : virtual_display->color565(255, 128, 0);
  // Outer circle (diameter 5, radius 2 at center X=117, Y=baseY+2)
  virtual_display->drawCircle(117, baseY + 2, 2, timeColor);
  // Center pixel and hand
  virtual_display->drawPixel(117, baseY + 2, timeColor);
  virtual_display->drawPixel(117, baseY + 1, timeColor);

  // 4. Weather Sync Icon (X: 122..126, Y: 1..6)
  uint16_t weatherColor = hasWeatherData ? virtual_display->color565(0, 255, 0) : virtual_display->color565(255, 0, 0);
  // Draw a tiny sun (cross with center) at X=124, Y=baseY+2
  virtual_display->drawPixel(124, baseY + 2, weatherColor); // center
  virtual_display->drawPixel(124, baseY + 1, weatherColor); // top
  virtual_display->drawPixel(124, baseY + 3, weatherColor); // bottom
  virtual_display->drawPixel(123, baseY + 2, weatherColor); // left
  virtual_display->drawPixel(125, baseY + 2, weatherColor); // right
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
    virtual_display->print("AP: " DEFAULT_AP_SSID);
    virtual_display->setCursor(4, 60);
    virtual_display->print("IP: 192.168.4.1");
    dma_display->flipDMABuffer(); // Swap buffer to show the screen
    delay(1000);

    // Setup AP and web server
    setupAPMode();
    setupWebServer();
    setupCaptivePortal(); // Enable captive portal DNS redirect

    Serial.println("\nConnect to WiFi: " DEFAULT_AP_SSID);
    Serial.println("Then visit: http://192.168.4.1");
    Serial.println("\nWaiting for configuration...\n");
  } else {
    // Valid configuration exists - normal operation
    Serial.println("\n>>> CONFIGURATION LOADED <<<");
    Serial.println(">>> STARTING NORMAL MODE <<<\n");

    isConfigMode = false;

    // Build weather API URL from stored coordinates (using HTTP to save RAM/CPU and prevent SSL out of memory errors)
    String weatherApiUrl = "http://api.open-meteo.com/v1/forecast?latitude=";
    weatherApiUrl += String(deviceConfig.latitude, 4);
    weatherApiUrl += "&longitude=";
    weatherApiUrl += String(deviceConfig.longitude, 4);
    weatherApiUrl += "&current=temperature_2m,relative_humidity_2m,weather_code,uv_index";
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
      
      // Sync RTC from NTP on first boot (update only if drifted >= 5s)
      Serial.println("Syncing time from NTP...");
      struct tm timeinfo;
      if (getLocalTime(&timeinfo, 5000)) { // 5 second timeout
        syncRTCFromNTPIfDrifted(timeinfo, 5);
        Serial.println("Initial NTP sync complete");
      } else {
        Serial.println("NTP sync timeout, RTC will be used as fallback");
      }
    } else {
      // No WiFi at startup — RTC will be used, initWeather skipped
      Serial.println("No WiFi at startup. RTC will be used for timekeeping.");
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
      virtual_display->setFont(&Verdana_Vietnamese10pt);
      
      // Line 1: Header (orange)
      virtual_display->setTextColor(virtual_display->color565(255, 128, 0));
      virtual_display->setCursor(4, 22);
      virtual_display->print(utf8ToCustom("Cấu hình WiFi:"));

      // Line 2: AP name (blinking white/gray)
      virtual_display->setTextColor(blinkState ? virtual_display->color565(255, 255, 255) : virtual_display->color565(64, 64, 64));
      virtual_display->setCursor(4, 46);
      virtual_display->print(utf8ToCustom("WiFi: " DEFAULT_AP_SSID));

      // Line 3: IP address (cyan)
      virtual_display->setTextColor(virtual_display->color565(0, 255, 255));
      virtual_display->setCursor(4, 70);
      virtual_display->print(utf8ToCustom("Web: 192.168.4.1"));
      
      dma_display->flipDMABuffer(); // Swap buffer to display the updated frame
    }

    handleDNS(); // Process DNS requests for captive portal
    delay(50);
    return;
  }

  // Normal operation mode — background WiFi reconnect (non-blocking)
  // Attempt reconnect every 30 seconds when disconnected, without blocking the display
  static unsigned long lastReconnectAttempt = 0;
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 30000) { // 30 second interval
      lastReconnectAttempt = now;
      Serial.println("WiFi disconnected, attempting background reconnect...");
      WiFi.disconnect();
      WiFi.mode(WIFI_STA);
      WiFi.begin(deviceConfig.ssid, deviceConfig.password);
      // Non-blocking: result will be checked on next loop iteration
    }
  }
  
  // Get time: prefer NTP (system clock) when WiFi is up, fall back to RTC
  struct tm timeinfo;
  bool hasTime;
  static unsigned long lastNTPSync = 0;
  const unsigned long NTP_SYNC_INTERVAL_MS = 60UL * 60UL * 1000UL; // 60 minutes
  
  if (WiFi.status() == WL_CONNECTED) {
    hasTime = getLocalTime(&timeinfo, 100);
    
    if (hasTime) {
      // Every 60 minutes: compare NTP with RTC and update RTC if drifted >= 5 seconds
      if (millis() - lastNTPSync > NTP_SYNC_INTERVAL_MS) {
        lastNTPSync = millis();
        Serial.println("[NTP Sync] 60-minute periodic check...");
        syncRTCFromNTPIfDrifted(timeinfo, 5);
      }
    } else {
      // NTP not ready yet — use RTC as fallback
      hasTime = getRTCTime(timeinfo);
    }
  } else {
    // No WiFi — use RTC
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
        dma_display->flipDMABuffer(); // Swap buffer to show blank screen
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
      // Draw status icons (WiFi, Time Sync, Weather Sync) at the very top
      drawStatusIcons();

      // ==========================================
      // ROW 1: Time (HH:MM:SS) (Y: 0..31)
      // ==========================================
      char hStr[3], mStr[3], sStr[3];
      sprintf(hStr, "%02d", timeinfo.tm_hour);
      sprintf(mStr, "%02d", timeinfo.tm_min);
      sprintf(sStr, "%02d", timeinfo.tm_sec);

      virtual_display->setFont(&ClockFont24px);
      
      // Draw Hour Digit 1 (X: 3, Y: 29) - shifted down 4px
      virtual_display->setCursor(3, 29);
      virtual_display->setTextColor(hsvToRgb565(globalHue, 255, 255));
      virtual_display->print(hStr[0]);
      
      // Draw Hour Digit 2 (X: 20, Y: 29) - shifted down 4px
      virtual_display->setCursor(20, 29);
      virtual_display->setTextColor(hsvToRgb565(globalHue + 15, 255, 255));
      virtual_display->print(hStr[1]);

      // Draw Blinking/Animating Colon 1 (X: 41) - shifted down 4px
      unsigned long ms = millis() % 1000;
      float progress = ms / 1000.0;
      float offset = (progress < 0.5) ? (progress * 2.0) : ((1.0 - progress) * 2.0); // 0.0 -> 1.0 -> 0.0
      int maxMove = 6;
      int currentTopY = 14 + (int)(offset * maxMove);
      int currentBottomY = 26 - (int)(offset * maxMove);
      uint16_t colonColor = hsvToRgb565(globalHue + 30, 255, 255);
      virtual_display->fillRect(41, currentTopY, 2, 2, colonColor);
      virtual_display->fillRect(41, currentBottomY, 2, 2, colonColor);

      // Draw Minute Digit 1 (X: 47, Y: 29) - shifted down 4px
      virtual_display->setCursor(47, 29);
      virtual_display->setTextColor(hsvToRgb565(globalHue + 45, 255, 255));
      virtual_display->print(mStr[0]);
      
      // Draw Minute Digit 2 (X: 64, Y: 29) - shifted down 4px
      virtual_display->setCursor(64, 29);
      virtual_display->setTextColor(hsvToRgb565(globalHue + 60, 255, 255));
      virtual_display->print(mStr[1]);

      // Draw Blinking/Animating Colon 2 (X: 85) - shifted down 4px
      uint16_t colonColor2 = hsvToRgb565(globalHue + 75, 255, 255);
      virtual_display->fillRect(85, currentTopY, 2, 2, colonColor2);
      virtual_display->fillRect(85, currentBottomY, 2, 2, colonColor2);

      // Draw Second Digit 1 (X: 91, Y: 29) - shifted down 4px
      virtual_display->setCursor(91, 29);
      virtual_display->setTextColor(hsvToRgb565(globalHue + 90, 255, 255));
      virtual_display->print(sStr[0]);
      
      // Draw Second Digit 2 (X: 108, Y: 29) - shifted down 4px
      virtual_display->setCursor(108, 29);
      virtual_display->setTextColor(hsvToRgb565(globalHue + 105, 255, 255));
      virtual_display->print(sStr[1]);

      // Read weather data early to display weather description in Row 2
      float tempOutdoor = 0.0;
      int humOutdoor = 0;
      int weatherCode = 0;
      float uvOutdoor = 0.0;
      bool showWeather = getWeatherData(tempOutdoor, humOutdoor, weatherCode, uvOutdoor);

      // ==========================================
      // ROW 2: Calendar (Solar & Lunar) (Y: 32..47)
      // ==========================================
      virtual_display->setFont(&Verdana_Vietnamese10pt);

      // 1. Solar Date (Left Side)
      String solarStr = getDayOfWeekStr(timeinfo.tm_wday) + ", " + String(timeinfo.tm_mday) + "/" + String(timeinfo.tm_mon + 1);
      virtual_display->setTextColor(hsvToRgb565(globalHue + 10, 255, 255));
      virtual_display->setCursor(2, 43);
      virtual_display->print(utf8ToCustom(solarStr));

      // 2. Lunar Date (Right Side) - Static
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

      // 3. Weather Description (Left Side, Y=56) & UV Index (Right Side, Y=56) - Static
      uint16_t outdoorColor = hsvToRgb565(globalHue + 100, 255, 255);
      if (showWeather) {
        // Weather Description (Left aligned at X=2)
        String wDesc = getWeatherDesc(weatherCode);
        virtual_display->setTextColor(outdoorColor);
        virtual_display->setCursor(2, 56);
        virtual_display->print(utf8ToCustom(wDesc));

        // UV Index (Right aligned at X=74)
        char uvStr[16];
        if (uvOutdoor >= 0.1) {
          sprintf(uvStr, "UV:%.1f", uvOutdoor);
        } else {
          sprintf(uvStr, "UV:0");
        }

        uint16_t uvColor = hsvToRgb565(globalHue + 80, 255, 255);
        virtual_display->setTextColor(uvColor);
        virtual_display->setCursor(89, 56);
        virtual_display->print(utf8ToCustom(uvStr));
      }

      // ==========================================
      // ROW 3: Sensors & Weather (Y: 58..73) - Shifted down 10px
      // ==========================================
      float tempIndoor = 0.0;
      int humIndoor = 0;
      bool showIndoor = getIndoorData(tempIndoor, humIndoor);

      // 1. Indoor Data (Left Side)
      uint16_t indoorColor = hsvToRgb565(globalHue + 60, 255, 255);
      drawIndoorIcon(2, 62, indoorColor);

      char inTempStr[16];
      if (showIndoor) {
        sprintf(inTempStr, "%.1f", tempIndoor);
      } else {
        sprintf(inTempStr, "--.-");
      }
      virtual_display->setTextColor(indoorColor);
      virtual_display->setCursor(11, 69);
      virtual_display->print(utf8ToCustom(inTempStr));
      
      int curX = virtual_display->getCursorX();
      virtual_display->drawCircle(curX + 1, 61, 1, indoorColor); // Degree circle
      
      virtual_display->setCursor(curX + 1, 69);
      char inHumStr[16];
      if (showIndoor) {
        sprintf(inHumStr, " %d%%", humIndoor); // Removed 'C' to save space
      } else {
        sprintf(inHumStr, " --%%");
      }
      virtual_display->print(utf8ToCustom(inHumStr));

      // 2. Outdoor Data (Right Side) - Shifted left and down 10px
      drawOutdoorIcon(66, 62, outdoorColor);

      char outTempStr[16];
      if (showWeather) {
        sprintf(outTempStr, "%.1f", tempOutdoor);
      } else {
        sprintf(outTempStr, "--.-");
      }
      virtual_display->setTextColor(outdoorColor);
      virtual_display->setCursor(75, 69);
      virtual_display->print(utf8ToCustom(outTempStr));
      
      int curX2 = virtual_display->getCursorX();
      virtual_display->drawCircle(curX2 + 1, 61, 1, outdoorColor); // Degree circle
      
      virtual_display->setCursor(curX2 + 1, 69);
      char outHumStr[16];
      if (showWeather) {
        sprintf(outHumStr, " %d%%", humOutdoor); // Removed 'C' to save space
      } else {
        sprintf(outHumStr, " --%%");
      }
      virtual_display->print(utf8ToCustom(outHumStr));

      // ==========================================
      // ROW 4: Marquee Scrolling Text (Y: 74..95) - Shifted down to Y=93
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
        
        // Measure width using custom function that handles char > 127 correctly
        cachedTextWidth = getCustomTextWidth(cachedCustomMsg, &Verdana_Bold18pt);
      }

      if (scrollX < -cachedTextWidth) {
        scrollX = 128.0; // Reset scroll position to right edge
      }

      virtual_display->setTextWrap(false); // CRITICAL: Disable wrapping to prevent multiple lines
      virtual_display->setFont(&Verdana_Bold18pt);
      virtual_display->setTextColor(hsvToRgb565(globalHue + 120, 255, 255));
      virtual_display->setCursor((int)scrollX, 88);
      virtual_display->print(cachedCustomMsg);

    } else {
      // Offline / waiting for time sync
      virtual_display->setFont(&Verdana_Vietnamese10pt);
      virtual_display->setTextColor(virtual_display->color565(255, 255, 0));
      virtual_display->setCursor(10, 20);
      virtual_display->print(utf8ToCustom("Đang đồng bộ"));
      virtual_display->setCursor(30, 48);
      virtual_display->print(utf8ToCustom("thời gian..."));
    }
    
    dma_display->flipDMABuffer(); // Swap buffer to display the finished frame
  }
}