#include <Arduino.h>
#include "log.h"
#include "pin_config.h"
#include "modules/sd_card_test.h"
#include "modules/audio_test.h"
#include "services/ble_hid_service.h"
#include "services/serial_cli.h"
#include "ui/lvgl_hal.h"
#include "ui/ui_manager.h"

static const char *TAG = "Main";

// Con trỏ đối tượng các module hệ thống (áp dụng RAII & quản lý vòng đời bộ nhớ)
static SdCardTest *s_sdTest = nullptr;
static AudioTest *s_audioTest = nullptr;
static BleHidService *s_bleHid = nullptr;
static SerialCli *s_serialCli = nullptr;
static LvglHal *s_lvglHal = nullptr;
static UiManager *s_uiMgr = nullptr;

// Trạng thái hệ thống
static bool s_lastBleConnected = false;
static unsigned long s_lastBleCheck = 0;

void setup() {
  Serial.begin(115200);

  // Đợi cổng USB CDC kết nối nếu đang cắm qua USB trực tiếp
  delay(1500);

  LOG_I(TAG, "==================================================");
  LOG_I(TAG, "ESP32-S3 SMART BLE HID CONTROLLER - PHASE 2 (LVGL)");
  LOG_I(TAG, "Thiet ke cho: %s", BOARD_NAME);
  LOG_I(TAG, "==================================================");

  // 1. Kiểm tra thông số bộ nhớ (Flash & PSRAM)
  uint32_t flashSize = ESP.getFlashChipSize() / (1024 * 1024);
  uint32_t psramSize = ESP.getPsramSize() / (1024 * 1024);
  uint32_t freePsram = ESP.getFreePsram() / 1024;
  uint32_t freeHeap = ESP.getFreeHeap() / 1024;

  LOG_I(TAG, "--- THONG SO HE THONG ---");
  LOG_I(TAG, "Flash Chip Size: %u MB", flashSize);
  LOG_I(TAG, "PSRAM Total: %u MB (Con trong: %u KB)", psramSize, freePsram);
  LOG_I(TAG, "Internal Heap Free: %u KB", freeHeap);

  // 2. Kiểm tra thẻ nhớ MicroSD
  s_sdTest = new SdCardTest();
  s_sdTest->runDiagnostic();

  // 3. Khởi tạo Audio NS4168 I2S
  s_audioTest = new AudioTest();
  s_audioTest->begin();

  // 4. Khởi tạo BLE HID Composite Service (Core 0)
  s_bleHid = new BleHidService();
  if (!s_bleHid->begin("ESP32-S3 TouchPad")) {
    LOG_E(TAG, "Khoi tao BLE HID Service that bai!");
  }

  // 5. Khởi tạo Serial CLI Test Service
  s_serialCli = new SerialCli(s_bleHid);
  s_serialCli->begin();

  // 6. Khởi tạo LVGL Hardware Abstraction Layer (Portrait 320x480)
  s_lvglHal = new LvglHal();
  if (!s_lvglHal->begin(0 /* Portrait */)) {
    LOG_E(TAG, "Khoi tao LVGL HAL that bai!");
  }

  // 7. Khởi tạo UI Manager (5 Tabs + Vietnamese Montserrat Fonts)
  s_uiMgr = new UiManager(s_bleHid);
  if (!s_uiMgr->begin()) {
    LOG_E(TAG, "Khoi tao UI Manager that bai!");
  }

  // Cập nhật trạng thái BLE ban đầu
  s_uiMgr->setBleConnected(s_bleHid->isConnected(), s_bleHid->getConnectedDeviceName());

  // Âm thanh khởi động
  s_audioTest->playChime();

  LOG_I(TAG, "He thong Phase 2 khoi dong hoan tat! Giao dien Portrait 320x480 da san sang.");
}

void loop() {
  // 1. Cập nhật Serial CLI (non-blocking)
  if (s_serialCli != nullptr) {
    s_serialCli->update();
  }

  // 2. Cập nhật khung hình LVGL và cảm ứng (Core 1)
  if (s_lvglHal != nullptr) {
    s_lvglHal->update();
  }

  // 3. Kiểm tra thay đổi trạng thái kết nối BLE để cập nhật UI
  unsigned long now = millis();
  if (now - s_lastBleCheck >= 100) {
    s_lastBleCheck = now;
    if (s_bleHid != nullptr && s_uiMgr != nullptr) {
      bool connected = s_bleHid->isConnected();
      if (connected != s_lastBleConnected) {
        s_lastBleConnected = connected;
        s_uiMgr->setBleConnected(connected, s_bleHid->getConnectedDeviceName());
      }
    }
  }

  // Nhường CPU cho IDLE task để tránh Watchdog Timer (Rule 4 & Rule 8)
  vTaskDelay(pdMS_TO_TICKS(4));
}