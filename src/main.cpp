#include "config_manager.h"
#include "lunar_calendar.h"
#include "reset_button.h"
#include "web_server.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <FastLED.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <time.h>

// Configuration data loaded from NVS
ConfigData deviceConfig;
#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 1

MatrixPanel_I2S_DMA *dma_display = nullptr;
uint8_t globalHue = 0;

// Weather API URL (will be built dynamically from config)
String weatherApiUrl = "";
bool isConfigMode = false; // Flag to indicate if in AP config mode
float temperature = 0.0;
int humidity = 0;
unsigned long lastWeatherUpdate = 0;
const unsigned long weatherUpdateInterval =
    600000; // 10 minutes in milliseconds
bool hasWeatherData = false;

// FreeRTOS task handle and mutex for thread-safe weather updates
TaskHandle_t weatherTaskHandle = NULL;
SemaphoreHandle_t weatherMutex = NULL;

// Lunar calendar variables
int lunarDay = 0;
int lunarMonth = 0;
int lunarYear = 0;
int lastCalculatedDay = -1; // Cache: chỉ tính lại khi sang ngày mới

uint16_t hsvToRgb565(uint8_t hue, uint8_t sat, uint8_t val) {
  CRGB rgb;
  CHSV hsv(hue, sat, val);
  hsv2rgb_rainbow(hsv, rgb);
  return dma_display->color565(rgb.r, rgb.g, rgb.b);
}

// Bitmap cho chữ "Thứ" (15x8 pixels)
// T = 84, h = 68, ứ = u + dấu
const uint8_t thuBitmap[8] = {0b11111000, // T
                              0b00100000, // h
                              0b00100110, // ứ (u)
                              0b00100110, //
                              0b00100110, //
                              0b00100110, //
                              0b00100011, // dấu ứ
                              0b00000000};

// Hàm vẽ chữ "Thứ" tùy chỉnh
void drawThu(int16_t x, int16_t y, uint16_t color) {
  // Vẽ chữ "T"
  dma_display->drawLine(x, y, x + 4, y, color);         // Ngang trên
  dma_display->drawLine(x + 2, y, x + 2, y + 6, color); // Dọc

  // Vẽ chữ "h"
  dma_display->drawLine(x + 6, y, x + 6, y + 6, color); // Dọc trái
  dma_display->drawPixel(x + 7, y + 3, color);          // Cong
  dma_display->drawPixel(x + 8, y + 4, color);
  dma_display->drawPixel(x + 8, y + 5, color);
  dma_display->drawPixel(x + 8, y + 6, color);

  // Vẽ chữ "ư" (u + dấu)
  dma_display->drawLine(x + 10, y + 3, x + 10, y + 6, color); // Dọc trái
  dma_display->drawLine(x + 13, y + 3, x + 13, y + 6, color); // Dọc phải
  dma_display->drawPixel(x + 11, y + 6, color);               // Đáy
  dma_display->drawPixel(x + 12, y + 6, color);
  dma_display->drawPixel(x + 14, y + 4, color); // Dấu ngang

  // Vẽ dấu sắc (/)
  dma_display->drawPixel(x + 12, y + 1, color);
  dma_display->drawPixel(x + 13, y, color);
}

// Hàm vẽ chữ "ậ" (a + dấu mũ + dấu nặng) tùy chỉnh
// Dựa theo pattern 5x8 pixels từ hình mẫu
void drawACircumflexDotBelow(int16_t x, int16_t y, uint16_t color) {
  // Hàng 0: Dấu mũ (^) - đỉnh
  dma_display->drawPixel(x + 2, y, color);

  // Hàng 1: Dấu mũ - hai bên
  dma_display->drawPixel(x + 1, y + 1, color);
  dma_display->drawPixel(x + 3, y + 1, color);

  // Hàng 2: Trống (khoảng cách giữa dấu mũ và chữ a)

  // Hàng 3: Phần trên của chữ "a" - 3 pixel ngang
  dma_display->drawPixel(x + 1, y + 3, color);
  dma_display->drawPixel(x + 2, y + 3, color);
  dma_display->drawPixel(x + 3, y + 3, color);
  dma_display->drawPixel(x + 4, y + 3, color);

  // Hàng 4: Cạnh trái và phải của "a"
  dma_display->drawPixel(x, y + 4, color);
  dma_display->drawPixel(x + 4, y + 4, color);
  dma_display->drawPixel(x, y + 5, color);
  dma_display->drawPixel(x + 4, y + 5, color);

  // Hàng 5: Đáy của "a" - 3 pixel ngang ở giữa + cạnh phải
  dma_display->drawPixel(x + 1, y + 6, color);
  dma_display->drawPixel(x + 2, y + 6, color);
  dma_display->drawPixel(x + 3, y + 6, color);
  dma_display->drawPixel(x + 4, y + 6, color);
  dma_display->drawPixel(x + 5, y + 6, color);

  // Hàng 6: Trống (khoảng cách trước dấu nặng)

  // Hàng 7: Trống (khoảng cách trước dấu nặng)

  // Hàng 8: Dấu nặng (chấm dưới)
  dma_display->drawPixel(x + 2, y + 8, color);
}

// Connect to WiFi using stored configuration
// Connect to WiFi using stored configuration with retry mechanism
void connectWiFi() {
  static int retryCount = 0;
  static unsigned long lastRetryTime = 0;

  Serial.println("Connecting WiFi...");
  Serial.printf("SSID: %s\n", deviceConfig.ssid);
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
  WiFi.begin(deviceConfig.ssid, deviceConfig.password);

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
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    configTime(7 * 3600, 0, "pool.ntp.org");

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

// Lunar calendar implementation moved to lunar_calendar.cpp

// Hàm lấy dữ liệu thời tiết từ Open-Meteo API (thread-safe)
void fetchWeatherData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping weather update");
    return;
  }

  if (weatherApiUrl.length() == 0) {
    Serial.println("Weather API URL not configured");
    return;
  }

  Serial.println("[Weather Task] Fetching weather data...");
  HTTPClient http;
  http.begin(weatherApiUrl.c_str());
  http.setTimeout(5000); // 5 second timeout
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("[Weather Task] API Response received");

    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      // Lock mutex before updating shared variables
      if (xSemaphoreTake(weatherMutex, portMAX_DELAY) == pdTRUE) {
        temperature = doc["current"]["temperature_2m"];
        humidity = doc["current"]["relative_humidity_2m"];
        hasWeatherData = true;
        xSemaphoreGive(weatherMutex);
        Serial.printf("[Weather Task] Updated: %.1f°C, %d%%\n", temperature,
                      humidity);
      }
    } else {
      Serial.print("[Weather Task] JSON parse error: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.printf("[Weather Task] HTTP GET failed, error: %d\n", httpCode);
  }

  http.end();
  lastWeatherUpdate = millis();
}

// FreeRTOS task for background weather updates
void weatherUpdateTask(void *parameter) {
  Serial.println("[Weather Task] Started on Core " + String(xPortGetCoreID()));

  // Initial fetch after 5 seconds
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  fetchWeatherData();

  // Periodic updates every 10 minutes
  while (true) {
    vTaskDelay(weatherUpdateInterval / portTICK_PERIOD_MS);
    fetchWeatherData();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== ESP32 LED Matrix Clock v2.0 ===");

  // 1. Initialize LED Matrix first
  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);
  mxconfig.gpio.r1 = 25;
  mxconfig.gpio.g1 = 26;
  mxconfig.gpio.b1 = 27;
  mxconfig.gpio.r2 = 14;
  mxconfig.gpio.g2 = 12;
  mxconfig.gpio.b2 = 13;
  mxconfig.gpio.a = 23;
  mxconfig.gpio.b = 19;
  mxconfig.gpio.c = 5;
  mxconfig.gpio.d = 17;
  mxconfig.gpio.e = 18;
  mxconfig.gpio.clk = 16;
  mxconfig.gpio.lat = 4;
  mxconfig.gpio.oe = 15;

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  if (!dma_display->begin()) {
    Serial.println("Failed to initialize DMA Display!");
  }
  dma_display->setBrightness8(100);
  dma_display->clearScreen();

  dma_display->setCursor(2, 8);
  dma_display->setTextColor(dma_display->color565(255, 0, 0));
  dma_display->print("BOOTING...");
  dma_display->setCursor(6, 20);
  dma_display->print("v 2.0.1");
  delay(500);

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
    weatherApiUrl = "https://api.open-meteo.com/v1/forecast?latitude=";
    weatherApiUrl += String(deviceConfig.latitude, 4);
    weatherApiUrl += "&longitude=";
    weatherApiUrl += String(deviceConfig.longitude, 4);
    weatherApiUrl += "&current=temperature_2m,relative_humidity_2m";
    Serial.println("Weather API: " + weatherApiUrl);

    // Connect to WiFi
    connectWiFi();

    // Create mutex for thread-safe weather data access
    weatherMutex = xSemaphoreCreateMutex();
    if (weatherMutex == NULL) {
      Serial.println("Failed to create weather mutex!");
    }

    // Create weather update task on Core 0 (main loop runs on Core 1)
    if (WiFi.status() == WL_CONNECTED) {
      xTaskCreatePinnedToCore(weatherUpdateTask, // Task function
                              "WeatherTask",     // Task name
                              8192,              // Stack size (bytes)
                              NULL,              // Task parameter
                              1, // Priority (lower than default)
                              &weatherTaskHandle, // Task handle
                              0                   // Core 0
      );
      Serial.println("Weather task created successfully");
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
      connectWiFi();
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
    bool showWeather = false;
    float tempDisplay = 0.0;
    int humDisplay = 0;

    if (xSemaphoreTake(weatherMutex, 10 / portTICK_PERIOD_MS) == pdTRUE) {
      showWeather = hasWeatherData;
      tempDisplay = temperature;
      humDisplay = humidity;
      xSemaphoreGive(weatherMutex);
    }

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