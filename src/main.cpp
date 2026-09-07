#include <Arduino.h>
#include "log.h"
#include "pin_config.h"
#include "modules/i2c_scanner.h"
#include "modules/display_touch_test.h"
#include "modules/sd_card_test.h"
#include "modules/audio_test.h"

static const char *TAG = "Main";

// Con trỏ đối tượng các module kiểm thử (áp dụng RAII & quản lý vòng đời bộ nhớ)
static I2cScanner *s_i2cScanner = nullptr;
static DisplayTouchTest *s_displayTouch = nullptr;
static SdCardTest *s_sdTest = nullptr;
static AudioTest *s_audioTest = nullptr;

// Lưu thông tin thẻ nhớ SD để hiển thị
static String s_sdCapStr = "";
static String s_sdDetailStr = "";
static bool s_sdOk = false;

// Biến trạng thái cảm ứng và âm thanh
static bool s_wasTouched = false;
static int32_t s_lastX = -1;
static int32_t s_lastY = -1;
static int32_t s_lastRawX = 0;
static int32_t s_lastRawY = 0;
static unsigned long s_lastTouchUpdate = 0;
static unsigned long s_lastSdCheck = 0;

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
  LOG_I(TAG, "CHUONG TRINH KIEM THU PHAN CUNG ESP32-S3 KIT 3.5\"");
  LOG_I(TAG, "Thiet ke cho: %s", BOARD_NAME);
  LOG_I(TAG, "==================================================");

  // 1. Kiem tra Thong So Bo Nho (Flash & PSRAM)
  uint32_t flashSize = ESP.getFlashChipSize() / (1024 * 1024);
  uint32_t psramSize = ESP.getPsramSize() / (1024 * 1024);
  uint32_t freePsram = ESP.getFreePsram() / 1024;
  uint32_t freeHeap = ESP.getFreeHeap() / 1024;

  LOG_I(TAG, "--- THONG SO HE THONG ---");
  LOG_I(TAG, "Flash Chip Size: %u MB", flashSize);
  LOG_I(TAG, "PSRAM Total: %u MB (Con trong: %u KB)", psramSize, freePsram);
  LOG_I(TAG, "Internal Heap Free: %u KB", freeHeap);

  // 2. Chay SD Card Diagnostic (Yeu cau 3: Dung luong the SD o giua man hinh)
  s_sdTest = new SdCardTest();
  SdCardStatus sdStatus = s_sdTest->runDiagnostic();
  updateSdStatus(sdStatus);

  // 3. Khoi tao Audio Test (NS4168 qua I2S: BCLK=42, LRCK=2, DOUT=41)
  s_audioTest = new AudioTest();
  s_audioTest->begin();

  // 4. Khoi tao Man hinh va Cam ung qua PSRAM Canvas DMA
  s_displayTouch = new DisplayTouchTest();
  if (s_displayTouch->begin()) {
    // Chay bai test 5 mau co ban
    s_displayTouch->showColorTest();

    // Hien thi giao dien Dashboard trung tam (SD card + Touch position)
    s_displayTouch->showScreen(s_sdCapStr, s_sdDetailStr, s_sdOk, -1, -1, 0, 0, false);
  } else {
    LOG_E(TAG, "Khong the khoi tao man hinh LCD!");
  }

  // Phat am thanh giai điệu khởi động
  s_audioTest->playChime();

  LOG_I(TAG, "He thong da san sang nhan tuong tac!");
}

void loop() {
  if (s_displayTouch != nullptr) {
    TouchPoint pt = s_displayTouch->getTouch();

    if (pt.touched) {
      s_lastX = pt.x;
      s_lastY = pt.y;
      s_lastRawX = pt.rawX;
      s_lastRawY = pt.rawY;

      if (!s_wasTouched) {
        // Sự kiện mới chạm vào màn hình (Touch DOWN)
        s_wasTouched = true;
        s_lastTouchUpdate = millis();

        LOG_I(TAG, "-> TOUCH DOWN tai: Screen(X=%d, Y=%d) | Raw(X=%d, Y=%d)",
              pt.x, pt.y, pt.rawX, pt.rawY);

        // Phát âm thanh bip phản hồi xúc giác
        if (s_audioTest != nullptr) {
          s_audioTest->playTone(2200, 30, 0.4f);
        }

        // Cập nhật ngay tọa độ lên giữa màn hình
        s_displayTouch->showScreen(s_sdCapStr, s_sdDetailStr, s_sdOk,
                                   pt.x, pt.y, pt.rawX, pt.rawY, true);
      } else {
        // Đang giữ/di chuyển ngón tay (Throttle update ~ 30ms / ~33 FPS)
        unsigned long now = millis();
        if (now - s_lastTouchUpdate >= 30) {
          s_lastTouchUpdate = now;
          s_displayTouch->showScreen(s_sdCapStr, s_sdDetailStr, s_sdOk,
                                     pt.x, pt.y, pt.rawX, pt.rawY, true);
        }
      }
    } else {
      if (s_wasTouched) {
        // Sự kiện nhấc tay khỏi màn hình (Touch UP)
        s_wasTouched = false;
        LOG_I(TAG, "-> TOUCH UP (Giu toa do cuoi: X=%d, Y=%d)", s_lastX, s_lastY);

        // Cập nhật lại trạng thái [ CHO CHAM ] nhưng vẫn giữ tọa độ chạm cuối cùng
        s_displayTouch->showScreen(s_sdCapStr, s_sdDetailStr, s_sdOk,
                                   s_lastX, s_lastY, s_lastRawX, s_lastRawY, false);
      }
    }
  }

  // Tự phục hồi: Kiểm tra lại thẻ SD nếu ban đầu chưa cắm thẻ (chu kỳ 5 giây/lần - Rule 10)
  if (!s_sdOk && s_sdTest != nullptr && s_displayTouch != nullptr) {
    unsigned long now = millis();
    if (now - s_lastSdCheck >= 5000) {
      s_lastSdCheck = now;
      SdCardStatus newStatus = s_sdTest->runDiagnostic();
      if (newStatus.mounted) {
        LOG_I(TAG, "Phat hien the nho SD vua duoc cam vao!");
        updateSdStatus(newStatus);
        s_displayTouch->showScreen(s_sdCapStr, s_sdDetailStr, s_sdOk,
                                   s_lastX, s_lastY, s_lastRawX, s_lastRawY, s_wasTouched);
      }
    }
  }

  // Nhường thời gian cho FreeRTOS IDLE task để tránh kích hoạt Watchdog Timer (Rule 4 & Rule 8)
  vTaskDelay(pdMS_TO_TICKS(10));
}