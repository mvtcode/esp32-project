#include <Arduino.h>
#include "esp_camera.h"
#include <TFT_eSPI.h>

// ============================================
// Camera Pins for ESP32-S3 (Default FREENOVE)
// ============================================
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     15
#define SIOD_GPIO_NUM     4
#define SIOC_GPIO_NUM     5

#define Y9_GPIO_NUM       16
#define Y8_GPIO_NUM       17
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       12
#define Y5_GPIO_NUM       10
#define Y4_GPIO_NUM       8
#define Y3_GPIO_NUM       9
#define Y2_GPIO_NUM       11

#define VSYNC_GPIO_NUM    6
#define HREF_GPIO_NUM     7
#define PCLK_GPIO_NUM     13

// ============================================
// Display Pins (Defined in platformio.ini)
// ============================================
// MOSI: GPIO 20
// CLK:  GPIO 19
// DC:   GPIO 47
// RST:  GPIO 21
// CS:   GPIO 45
// BL:   GPIO 38

// ============================================
// Global Objects
// ============================================
TFT_eSPI tft = TFT_eSPI();

// ============================================
// Camera Configuration
// ============================================
bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  
  // ⚠️ NO PSRAM - Use smaller frame size and DRAM
  config.frame_size = FRAMESIZE_QQVGA;    // 160x120 (fits in DRAM)
  config.pixel_format = PIXFORMAT_RGB565; // RGB565 for TFT
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_DRAM; // ✅ Use DRAM instead of PSRAM
  config.jpeg_quality = 12;
  config.fb_count = 1;                    // Single buffer to save RAM

  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("❌ Camera init failed with error 0x%x\n", err);
    return false;
  }

  // Get camera sensor
  sensor_t * s = esp_camera_sensor_get();
  if (s == NULL) {
    Serial.println("❌ Failed to get camera sensor");
    return false;
  }

  // Adjust camera settings for better image quality
  s->set_brightness(s, 0);     // -2 to 2
  s->set_contrast(s, 0);       // -2 to 2
  s->set_saturation(s, 0);     // -2 to 2
  s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect)
  s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
  s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
  s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled
  s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
  s->set_aec2(s, 0);           // 0 = disable , 1 = enable
  s->set_ae_level(s, 0);       // -2 to 2
  s->set_aec_value(s, 300);    // 0 to 1200
  s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
  s->set_agc_gain(s, 0);       // 0 to 30
  s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
  s->set_bpc(s, 0);            // 0 = disable , 1 = enable
  s->set_wpc(s, 1);            // 0 = disable , 1 = enable
  s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
  s->set_lenc(s, 1);           // 0 = disable , 1 = enable
  s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
  s->set_vflip(s, 0);          // 0 = disable , 1 = enable
  s->set_dcw(s, 1);            // 0 = disable , 1 = enable
  s->set_colorbar(s, 0);       // 0 = disable , 1 = enable

  Serial.println("✅ Camera initialized successfully");
  return true;
}

// ============================================
// Display Initialization
// ============================================
bool initDisplay() {
  tft.init();
  tft.setRotation(0); // Portrait mode (240x320)
  tft.fillScreen(TFT_BLACK);
  
  // Turn on backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  
  // Display startup message
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 100);
  tft.println("ESP32-S3");
  tft.setCursor(20, 130);
  tft.println("Camera");
  tft.setCursor(20, 160);
  tft.println("Starting...");
  
  Serial.println("✅ Display initialized successfully");
  return true;
}

// ============================================
// Setup
// ============================================
void setup() {
  Serial.begin(115200);
  delay(2000); // Longer delay for serial to stabilize
  
  Serial.println("\n\n=================================");
  Serial.println("ESP32-S3 Camera Display System");
  Serial.println("Camera OV5640 + TFT ST7789");
  Serial.println("=================================\n");
  
  Serial.printf("ESP32 Chip: %s\n", ESP.getChipModel());
  Serial.printf("Chip Revision: %d\n", ESP.getChipRevision());
  Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("PSRAM Size: %d bytes\n\n", ESP.getPsramSize());
  
  // Initialize Display
  Serial.println("🔧 Initializing Display...");
  if (!initDisplay()) {
    Serial.println("❌ Display initialization failed!");
    while(1) { delay(1000); }
  }
  
  delay(2000); // Show startup message
  
  // Initialize Camera
  Serial.println("🔧 Initializing Camera...");
  if (!initCamera()) {
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.setTextSize(2);
    tft.setCursor(10, 80);
    tft.println("Camera");
    tft.setCursor(10, 110);
    tft.println("Init");
    tft.setCursor(10, 140);
    tft.println("FAILED!");
    tft.setTextSize(1);
    tft.setCursor(10, 180);
    tft.println("Check:");
    tft.setCursor(10, 200);
    tft.println("- Camera pins");
    tft.setCursor(10, 220);
    tft.println("- Power supply");
    tft.setCursor(10, 240);
    tft.println("- Serial log");
    
    Serial.println("❌ Camera initialization failed!");
    Serial.println("⚠️  System will continue in display-only mode");
    Serial.println("    You can test the display without camera\n");
    
    // Display-only mode - show test pattern
    delay(3000);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, 100);
    tft.println("Display OK!");
    tft.setTextSize(1);
    tft.setCursor(20, 140);
    tft.println("Camera not detected");
    
    // Stay in loop showing display works
    while(1) {
      delay(1000);
    }
  }
  
  tft.fillScreen(TFT_BLACK);
  Serial.println("\n✅ System ready! Starting camera stream...\n");
}

// ============================================
// Main Loop
// ============================================
unsigned long lastFrameTime = 0;
unsigned long frameCount = 0;
float fps = 0;

void loop() {
  // Capture frame from camera
  camera_fb_t * fb = esp_camera_fb_get();
  
  if (!fb) {
    Serial.println("❌ Camera capture failed");
    delay(100);
    return;
  }
  
  // Calculate FPS
  frameCount++;
  unsigned long currentTime = millis();
  if (currentTime - lastFrameTime >= 1000) {
    fps = frameCount * 1000.0 / (currentTime - lastFrameTime);
    frameCount = 0;
    lastFrameTime = currentTime;
    Serial.printf("📊 FPS: %.2f | Frame size: %d bytes\n", fps, fb->len);
  }
  
  // Display image on TFT
  // The camera is configured for RGB565 format which is directly compatible with TFT
  if (fb->format == PIXFORMAT_RGB565) {
    // Camera frame is 160x120 (QQVGA), display is 240x320
    // We'll scale it up 2x to 320x240 and center it
    // For simplicity, we'll just center the 160x120 image without scaling
    
    int x_offset = (240 - 160) / 2;  // Center horizontally: (240-160)/2 = 40
    int y_offset = (320 - 120) / 2;  // Center vertically: (320-120)/2 = 100
    
    tft.setAddrWindow(x_offset, y_offset, 160, 120);
    tft.pushColors((uint16_t*)fb->buf, fb->len / 2);
    
    // Display FPS and resolution info on screen
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(5, 5);
    tft.printf("FPS: %.1f | 160x120", fps);
  } else {
    Serial.println("⚠️  Unexpected pixel format");
  }
  
  // Return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb);
  
  // Small delay to prevent watchdog issues
  delay(10);
}