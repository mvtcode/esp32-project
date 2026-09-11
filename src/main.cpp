#include <Arduino.h>
#include "log.h"
#include "pin_config.h"
#include "modules/i2c_scanner.h"
#include "modules/display_touch_test.h"
#include "modules/sd_card_test.h"
#include "modules/audio_test.h"
#include "services/ble_hid_service.h"
#include "services/serial_cli.h"

static const char *TAG = "Main";

// Con trỏ đối tượng các module kiểm thử (áp dụng RAII & quản lý vòng đời bộ nhớ)
static I2cScanner *s_i2cScanner = nullptr;
static DisplayTouchTest *s_displayTouch = nullptr;
static SdCardTest *s_sdTest = nullptr;
static AudioTest *s_audioTest = nullptr;
static BleHidService *s_bleHid = nullptr;
static SerialCli *s_serialCli = nullptr;

// Lưu thông tin thẻ nhớ SD để hiển thị
static String s_sdCapStr = "";
static String s_sdDetailStr = "";
static bool s_sdOk = false;

// Biến trạng thái cảm ứng và âm thanh
static bool s_wasTouched = false;
static int32_t s_lastX = -1;
static int32_t s_lastY = -1;
static int32_t s_startX = -1;
static int32_t s_startY = -1;
static int32_t s_lastRawX = 0;
static int32_t s_lastRawY = 0;
static unsigned long s_touchDownTime = 0;
static unsigned long s_lastTouchUpdate = 0;
static unsigned long s_lastSdCheck = 0;
static bool s_lastBleConnected = false;

static void updateSdStatus(const SdCardStatus& status) {
  s_sdOk = status.mounted;
  if (s_sdOk) {
    float capGB = (float)status.totalBytes / (1024.0f * 1024.0f * 1024.0f);
    uint32_t capMB = (uint32_t)(status.totalBytes / (1024 * 1024));
    if (capGB >= 1.0f) {
      char b[64];
      snprintf(b, sizeof(b), "%.2f GB (%u MB)", capGB, capMB);
      s_sdCapStr = b;
    } else {
      s_sdCapStr = String(capMB) + " MB";
    }
    s_sdDetailStr = "Loai: " + status.cardType + " | Doc/Ghi & Magic Bytes: THANH CONG";
  } else {
    s_sdCapStr = "CHUA NHAN THE NHO MICRO SD";
    s_sdDetailStr = status.message + " (Cam the vao khe de thu lai)";
  }
}

void setup() {
  Serial.begin(115200);

  // Đợi cổng USB CDC kết nối nếu đang cắm qua USB trực tiếp
  delay(1500);

  LOG_I(TAG, "==================================================");
  LOG_I(TAG, "ESP32-S3 SMART BLE HID CONTROLLER - PHASE 1");
  LOG_I(TAG, "Thiet ke cho: %s", BOARD_NAME);
  LOG_I(TAG, "==================================================");

  // 1. Kiểm tra Thông Số Bộ Nhớ (Flash & PSRAM)
  uint32_t flashSize = ESP.getFlashChipSize() / (1024 * 1024);
  uint32_t psramSize = ESP.getPsramSize() / (1024 * 1024);
  uint32_t freePsram = ESP.getFreePsram() / 1024;
  uint32_t freeHeap = ESP.getFreeHeap() / 1024;

  LOG_I(TAG, "--- THONG SO HE THONG ---");
  LOG_I(TAG, "Flash Chip Size: %u MB", flashSize);
  LOG_I(TAG, "PSRAM Total: %u MB (Con trong: %u KB)", psramSize, freePsram);
  LOG_I(TAG, "Internal Heap Free: %u KB", freeHeap);

  // 2. Chạy SD Card Diagnostic
  s_sdTest = new SdCardTest();
  SdCardStatus sdStatus = s_sdTest->runDiagnostic();
  updateSdStatus(sdStatus);

  // 3. Khởi tạo Audio (NS4168 qua I2S: BCLK=42, LRCK=2, DOUT=41)
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

  // 6. Khởi tạo Màn hình và Cảm ứng qua PSRAM Canvas DMA
  s_displayTouch = new DisplayTouchTest();
  if (s_displayTouch->begin()) {
    s_displayTouch->showColorTest();
    s_displayTouch->showScreen(s_sdCapStr, s_sdDetailStr, s_sdOk,
                               -1, -1, 0, 0, false,
                               s_bleHid->isConnected(), s_bleHid->getConnectedDeviceName());
  } else {
    LOG_E(TAG, "Khong the khoi tao man hinh LCD!");
  }

  // Phát âm thanh giai điệu khởi động
  s_audioTest->playChime();

  LOG_I(TAG, "He thong Phase 1 da san sang! Go 'help' tren Serial Monitor de xem danh sach lenh.");
}

void loop() {
  // 1. Cập nhật và xử lý lệnh từ Serial CLI (non-blocking)
  if (s_serialCli != nullptr) {
    s_serialCli->update();
  }

  // 2. Kiểm tra thay đổi trạng thái kết nối BLE để cập nhật màn hình
  bool bleConnected = (s_bleHid != nullptr && s_bleHid->isConnected());
  if (bleConnected != s_lastBleConnected) {
    s_lastBleConnected = bleConnected;
    if (s_displayTouch != nullptr) {
      s_displayTouch->showScreen(s_sdCapStr, s_sdDetailStr, s_sdOk,
                                 s_lastX, s_lastY, s_lastRawX, s_lastRawY, s_wasTouched,
                                 bleConnected, s_bleHid->getConnectedDeviceName());
    }
  }

  // 3. Xử lý cảm ứng và Trackpad Chuột thời gian thực
  if (s_displayTouch != nullptr) {
    TouchPoint pt = s_displayTouch->getTouch();

    if (pt.touched) {
      int32_t curX = pt.x;
      int32_t curY = pt.y;
      s_lastRawX = pt.rawX;
      s_lastRawY = pt.rawY;

      if (!s_wasTouched) {
        // Sự kiện chạm xuống (Touch DOWN)
        s_wasTouched = true;
        s_startX = curX;
        s_startY = curY;
        s_lastX = curX;
        s_lastY = curY;
        s_touchDownTime = millis();
        s_lastTouchUpdate = s_touchDownTime;

        LOG_I(TAG, "-> TOUCH DOWN: X=%d, Y=%d (Raw: X=%d, Y=%d)",
              curX, curY, pt.rawX, pt.rawY);

        // Âm thanh phản hồi xúc giác
        if (s_audioTest != nullptr) {
          s_audioTest->playTone(2200, 25, 0.35f);
        }

        s_displayTouch->showScreen(s_sdCapStr, s_sdDetailStr, s_sdOk,
                                   curX, curY, pt.rawX, pt.rawY, true,
                                   bleConnected, s_bleHid->getConnectedDeviceName());
      } else {
        // Đang giữ và kéo ngón tay (Drag / Move)
        unsigned long now = millis();
        if (now - s_lastTouchUpdate >= 25) { // ~40Hz update rate
          s_lastTouchUpdate = now;

          // Tính toán delta di chuyển chuột nếu đã kết nối BLE
          if (bleConnected && s_bleHid != nullptr) {
            int32_t dx = curX - s_lastX;
            int32_t dy = curY - s_lastY;

            // Giới hạn delta trong phạm vi int8 (-127 đến +127)
            if (dx > 127) dx = 127;
            if (dx < -127) dx = -127;
            if (dy > 127) dy = 127;
            if (dy < -127) dy = -127;

            if (dx != 0 || dy != 0) {
              s_bleHid->mouseMove((int8_t)dx, (int8_t)dy, 0);
            }
          }

          s_lastX = curX;
          s_lastY = curY;

          s_displayTouch->showScreen(s_sdCapStr, s_sdDetailStr, s_sdOk,
                                     curX, curY, pt.rawX, pt.rawY, true,
                                     bleConnected, s_bleHid->getConnectedDeviceName());
        }
      }
    } else {
      if (s_wasTouched) {
        // Sự kiện nhấc tay (Touch UP)
        unsigned long pressDuration = millis() - s_touchDownTime;
        int32_t totalDistX = abs(s_lastX - s_startX);
        int32_t totalDistY = abs(s_lastY - s_startY);

        // Nhận diện cú gõ nhanh (Single Tap) thành Left Click nếu di chuyển < 12px và thời gian < 300ms
        if (bleConnected && s_bleHid != nullptr && pressDuration < 300 && totalDistX < 12 && totalDistY < 12) {
          LOG_I(TAG, "-> PHAT HIEN CU GO (TAP) -> GUI LEFT CLICK");
          s_bleHid->mouseClick(MOUSE_BUTTON_LEFT);
        }

        s_wasTouched = false;
        LOG_I(TAG, "-> TOUCH UP (Toa do cuoi: X=%d, Y=%d, thoi gian giu=%lu ms)",
              s_lastX, s_lastY, pressDuration);

        s_displayTouch->showScreen(s_sdCapStr, s_sdDetailStr, s_sdOk,
                                   s_lastX, s_lastY, s_lastRawX, s_lastRawY, false,
                                   bleConnected, s_bleHid->getConnectedDeviceName());
      }
    }
  }

  // 4. Tự phục hồi: Kiểm tra lại thẻ SD nếu chưa cắm ban đầu (chu kỳ 5s - Rule 10)
  if (!s_sdOk && s_sdTest != nullptr && s_displayTouch != nullptr) {
    unsigned long now = millis();
    if (now - s_lastSdCheck >= 5000) {
      s_lastSdCheck = now;
      SdCardStatus newStatus = s_sdTest->runDiagnostic();
      if (newStatus.mounted) {
        LOG_I(TAG, "Phat hien the nho SD vua duoc cam vao!");
        updateSdStatus(newStatus);
        s_displayTouch->showScreen(s_sdCapStr, s_sdDetailStr, s_sdOk,
                                   s_lastX, s_lastY, s_lastRawX, s_lastRawY, s_wasTouched,
                                   bleConnected, s_bleHid->getConnectedDeviceName());
      }
    }
  }

  // Nhường CPU cho IDLE task để tránh Watchdog Timer (Rule 4 & Rule 8)
  vTaskDelay(pdMS_TO_TICKS(5));
}