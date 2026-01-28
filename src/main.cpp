#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <SPI.h>
#include <WiFi.h>
#include <time.h>
#include <esp_wifi.h>

// WiFi credentials
const char* ssid = "HPSTAR";
const char* password = "0964335688";

// NTP settings
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 3600;  // GMT+7
const int daylightOffset_sec = 0;

// Display pins
#define TFT_CS   1
#define TFT_DC   2
#define TFT_MOSI 3
#define TFT_SCLK 4

Adafruit_GC9A01A tft(TFT_CS, TFT_DC);

// Clock settings
#define CLOCK_CENTER_X 120
#define CLOCK_CENTER_Y 120
#define CLOCK_RADIUS 110
#define HOUR_HAND_LENGTH 50
#define MINUTE_HAND_LENGTH 70
#define SECOND_HAND_LENGTH 90

// Colors
#define COLOR_BACKGROUND 0x0000  // Black
#define COLOR_FACE       0xFFFF  // White
#define COLOR_HOUR_MARK  0xFFFF  // White
#define COLOR_MIN_MARK   0x7BEF  // Gray
#define COLOR_HOUR_HAND  0xFFFF  // White
#define COLOR_MIN_HAND   0x07FF  // Cyan
#define COLOR_SEC_HAND   0xF800  // Red
#define COLOR_CENTER     0xFFE0  // Yellow
#define COLOR_TEXT       0x07E0  // Green

// Previous hand positions for erasing
int prevSecondX = 0, prevSecondY = 0;
int prevMinuteX = 0, prevMinuteY = 0;
int prevHourX = 0, prevHourY = 0;
bool firstDraw = true;

void connectWiFi() {
  Serial.println("\n=== Connecting to WiFi ===");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password length: ");
  Serial.println(strlen(password));
  
  tft.fillScreen(COLOR_BACKGROUND);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(30, 100);
  tft.println("WiFi...");
  
  // Scan for networks first
  Serial.println("\nScanning WiFi networks...");
  int n = WiFi.scanNetworks();
  Serial.print("Found ");
  Serial.print(n);
  Serial.println(" networks:");
  
  bool foundSSID = false;
  int targetChannel = 0;
  for (int i = 0; i < n; i++) {
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" (");
    Serial.print(WiFi.RSSI(i));
    Serial.print(" dBm, Ch:");
    Serial.print(WiFi.channel(i));
    Serial.print(") ");
    Serial.println(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" : "Encrypted");
    
    if (WiFi.SSID(i) == ssid) {
      foundSSID = true;
      targetChannel = WiFi.channel(i);
      Serial.println("   ^^ TARGET NETWORK FOUND!");
      Serial.print("   Channel: ");
      Serial.println(targetChannel);
    }
  }
  
  if (!foundSSID) {
    Serial.println("\nWARNING: Target SSID not found in scan!");
    Serial.println("Make sure:");
    Serial.println("1. SSID is correct (case-sensitive)");
    Serial.println("2. Router is on 2.4GHz (ESP32 C3 doesn't support 5GHz)");
    Serial.println("3. Router is in range");
  }
  
  Serial.println("\nConfiguring WiFi...");
  
  // Disconnect any previous connection
  WiFi.disconnect(true);
  delay(1000);
  
  // Configure WiFi with advanced settings
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);  // Don't save WiFi config to flash
  WiFi.setAutoReconnect(true);
  
  // Set power save mode off for better stability
  esp_wifi_set_ps(WIFI_PS_NONE);
  
  Serial.println("Attempting connection...");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  int maxAttempts = 40;  // 20 seconds total
  
  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(500);
    
    if (attempts % 5 == 0) {
      Serial.println();
      Serial.print("Attempt ");
      Serial.print(attempts/5 + 1);
      Serial.print("/");
      Serial.print(maxAttempts/5);
      Serial.print(" - Status: ");
      
      switch(WiFi.status()) {
        case WL_IDLE_STATUS: Serial.print("IDLE"); break;
        case WL_NO_SSID_AVAIL: Serial.print("NO_SSID"); break;
        case WL_SCAN_COMPLETED: Serial.print("SCAN_DONE"); break;
        case WL_CONNECTED: Serial.print("CONNECTED"); break;
        case WL_CONNECT_FAILED: Serial.print("FAILED"); break;
        case WL_CONNECTION_LOST: Serial.print("LOST"); break;
        case WL_DISCONNECTED: Serial.print("DISCONNECTED"); break;
        default: Serial.print(WiFi.status()); break;
      }
      Serial.print(" ");
    } else {
      Serial.print(".");
    }
    
    attempts++;
    
    // Try to reconnect if disconnected
    if (attempts > 10 && WiFi.status() == WL_DISCONNECTED) {
      Serial.println("\nRetrying connection...");
      WiFi.disconnect();
      delay(500);
      WiFi.begin(ssid, password);
    }
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.print("Channel: ");
    Serial.println(WiFi.channel());
    
    tft.fillScreen(COLOR_BACKGROUND);
    tft.setCursor(20, 90);
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(2);
    tft.println("WiFi OK!");
    tft.setTextSize(1);
    tft.setCursor(10, 120);
    tft.print("IP: ");
    tft.println(WiFi.localIP());
    delay(2000);
  } else {
    Serial.println("\n✗ WiFi connection FAILED!");
    Serial.print("Final status: ");
    Serial.println(WiFi.status());
    Serial.println("\nPossible issues:");
    Serial.println("1. Router security: Try WPA2-PSK (not WPA3)");
    Serial.println("2. MAC filtering enabled on router");
    Serial.println("3. Router max clients reached");
    Serial.println("4. Router firewall blocking ESP32");
    
    tft.fillScreen(COLOR_BACKGROUND);
    tft.setCursor(20, 90);
    tft.setTextColor(COLOR_SEC_HAND);
    tft.setTextSize(2);
    tft.println("WiFi ERR");
    tft.setTextSize(1);
    tft.setCursor(10, 120);
    tft.print("Code: ");
    tft.println(WiFi.status());
    delay(3000);
  }
}

void syncTime() {
  Serial.println("Syncing time from NTP...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.println("Time synced successfully!");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  } else {
    Serial.println("Failed to sync time");
  }
}

void drawClockFace() {
  tft.fillScreen(COLOR_BACKGROUND);
  
  // Draw outer circle
  tft.drawCircle(CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_RADIUS, COLOR_FACE);
  tft.drawCircle(CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_RADIUS - 1, COLOR_FACE);
  
  // Draw hour marks and numbers
  for (int i = 0; i < 12; i++) {
    float angle = (i * 30 - 90) * PI / 180.0;
    
    // Hour tick marks
    int x1 = CLOCK_CENTER_X + (CLOCK_RADIUS - 10) * cos(angle);
    int y1 = CLOCK_CENTER_Y + (CLOCK_RADIUS - 10) * sin(angle);
    int x2 = CLOCK_CENTER_X + (CLOCK_RADIUS - 20) * cos(angle);
    int y2 = CLOCK_CENTER_Y + (CLOCK_RADIUS - 20) * sin(angle);
    
    tft.drawLine(x1, y1, x2, y2, COLOR_HOUR_MARK);
    tft.drawLine(x1 + 1, y1, x2 + 1, y2, COLOR_HOUR_MARK);
    
    // Draw numbers for 12, 3, 6, 9
    if (i == 0 || i == 3 || i == 6 || i == 9) {
      int num = (i == 0) ? 12 : i;
      int numX = CLOCK_CENTER_X + (CLOCK_RADIUS - 35) * cos(angle);
      int numY = CLOCK_CENTER_Y + (CLOCK_RADIUS - 35) * sin(angle);
      
      tft.setTextSize(2);
      tft.setTextColor(COLOR_FACE);
      
      // Center the text
      if (num == 12) {
        tft.setCursor(numX - 12, numY - 8);
      } else if (num == 3) {
        tft.setCursor(numX - 6, numY - 8);
      } else if (num == 6) {
        tft.setCursor(numX - 6, numY - 8);
      } else if (num == 9) {
        tft.setCursor(numX - 6, numY - 8);
      }
      tft.print(num);
    }
  }
  
  // Draw minute marks
  for (int i = 0; i < 60; i++) {
    if (i % 5 != 0) {  // Skip hour positions
      float angle = (i * 6 - 90) * PI / 180.0;
      int x1 = CLOCK_CENTER_X + (CLOCK_RADIUS - 5) * cos(angle);
      int y1 = CLOCK_CENTER_Y + (CLOCK_RADIUS - 5) * sin(angle);
      int x2 = CLOCK_CENTER_X + (CLOCK_RADIUS - 10) * cos(angle);
      int y2 = CLOCK_CENTER_Y + (CLOCK_RADIUS - 10) * sin(angle);
      
      tft.drawLine(x1, y1, x2, y2, COLOR_MIN_MARK);
    }
  }
  
  // Draw center dot
  tft.fillCircle(CLOCK_CENTER_X, CLOCK_CENTER_Y, 5, COLOR_CENTER);
  tft.drawCircle(CLOCK_CENTER_X, CLOCK_CENTER_Y, 6, COLOR_FACE);
}

void drawHand(int length, float angle, uint16_t color, int thickness) {
  float radian = (angle - 90) * PI / 180.0;
  int x = CLOCK_CENTER_X + length * cos(radian);
  int y = CLOCK_CENTER_Y + length * sin(radian);
  
  if (thickness == 1) {
    tft.drawLine(CLOCK_CENTER_X, CLOCK_CENTER_Y, x, y, color);
  } else {
    // Draw thick line
    for (int i = -thickness/2; i <= thickness/2; i++) {
      tft.drawLine(CLOCK_CENTER_X + i, CLOCK_CENTER_Y, x + i, y, color);
      tft.drawLine(CLOCK_CENTER_X, CLOCK_CENTER_Y + i, x, y + i, color);
    }
  }
}

void updateClock() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to get time");
    return;
  }
  
  int hours = timeinfo.tm_hour % 12;
  int minutes = timeinfo.tm_min;
  int seconds = timeinfo.tm_sec;
  
  // Calculate angles
  float secondAngle = seconds * 6;
  float minuteAngle = minutes * 6 + seconds * 0.1;
  float hourAngle = hours * 30 + minutes * 0.5;
  
  // Erase previous hands (draw in black)
  if (!firstDraw) {
    drawHand(SECOND_HAND_LENGTH, prevSecondX, COLOR_BACKGROUND, 1);
    drawHand(MINUTE_HAND_LENGTH, prevMinuteX, COLOR_BACKGROUND, 3);
    drawHand(HOUR_HAND_LENGTH, prevHourX, COLOR_BACKGROUND, 4);
  }
  
  // Draw new hands
  drawHand(HOUR_HAND_LENGTH, hourAngle, COLOR_HOUR_HAND, 4);
  drawHand(MINUTE_HAND_LENGTH, minuteAngle, COLOR_MIN_HAND, 3);
  drawHand(SECOND_HAND_LENGTH, secondAngle, COLOR_SEC_HAND, 1);
  
  // Redraw center dot
  tft.fillCircle(CLOCK_CENTER_X, CLOCK_CENTER_Y, 5, COLOR_CENTER);
  tft.drawCircle(CLOCK_CENTER_X, CLOCK_CENTER_Y, 6, COLOR_FACE);
  
  // Save current positions
  prevSecondX = secondAngle;
  prevMinuteX = minuteAngle;
  prevHourX = hourAngle;
  firstDraw = false;
  
  // Display digital time at bottom
  tft.fillRect(0, 210, 240, 30, COLOR_BACKGROUND);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(45, 215);
  char timeStr[10];
  sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, minutes, seconds);
  tft.print(timeStr);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n================================");
  Serial.println("ESP32 C3 Analog Clock");
  Serial.println("GC9A01 Display");
  Serial.println("================================\n");
  
  // Initialize SPI and display
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.begin();
  tft.setRotation(0);
  
  // Connect to WiFi
  connectWiFi();
  
  // Sync time
  if (WiFi.status() == WL_CONNECTED) {
    syncTime();
    delay(1000);
  }
  
  // Draw clock face
  drawClockFace();
  
  Serial.println("Clock started!");
}

void loop() {
  static unsigned long lastUpdate = 0;
  static unsigned long lastSync = 0;
  
  // Update clock every second
  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();
    updateClock();
  }
  
  // Resync time every hour
  if (millis() - lastSync >= 3600000) {
    lastSync = millis();
    if (WiFi.status() == WL_CONNECTED) {
      syncTime();
    }
  }
  
  delay(10);
}