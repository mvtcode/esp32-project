#include <Arduino.h>
#include <WiFi.h>
#include "log.h"
#include "pin_config.h"
#include "version.h"
#include "services/ota_service.h"

static const char *TAG = "Main";

static const char *WIFI_SSID = "HPSTAR";
static const char *WIFI_PASS = "0964335688";

static bool s_otaChecked = false;
static unsigned long s_lastOtaCheckTime = 0;
static bool s_hasNewVersion = false;
static OtaInfo s_cachedOtaInfo;

void checkOtaVersion() {
    if (OtaService::isUpdating()) {
        return;
    }

    LOG_I(TAG, "--------------------------------------------------");
    LOG_I(TAG, "Đang kiểm tra phiên bản mới từ: %s", OTA_MANIFEST_URL);
    OtaInfo info;
    if (OtaService::checkUpdate(info)) {
        s_cachedOtaInfo = info;
        LOG_I(TAG, "Phiên bản trên Server: %s (Hiện tại: %s)", info.version.c_str(), FIRMWARE_VERSION);
        LOG_I(TAG, "Nội dung cập nhật: %s", info.changelog.c_str());

        if (info.hasUpdate) {
            s_hasNewVersion = true;
            LOG_I(TAG, "==================================================");
            LOG_I(TAG, "🚀 PHÁT HIỆN PHIÊN BẢN MỚI: %s", info.version.c_str());
            LOG_I(TAG, "Link tải firmware: %s", info.firmwareUrl.c_str());
            LOG_I(TAG, "👉 HÃY GIỮ NÚT BOOT (IO0) TRONG 3 GIÂY ĐỂ BẮT ĐẦU NÂNG CẤP!");
            LOG_I(TAG, "==================================================");
        } else {
            s_hasNewVersion = false;
            LOG_I(TAG, "Thiết bị đang ở phiên bản mới nhất (%s).", FIRMWARE_VERSION);
        }
    } else {
        LOG_W(TAG, "Kiểm tra phiên bản thất bại: %s", OtaService::getErrorMessage());
    }
    LOG_I(TAG, "--------------------------------------------------");
}

void triggerOtaUpdate() {
    if (OtaService::isUpdating()) {
        LOG_W(TAG, "OTA đang trong tiến trình, bỏ qua yêu cầu.");
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        LOG_E(TAG, "Chưa kết nối WiFi! Không thể nạp OTA.");
        return;
    }

    if (!s_hasNewVersion) {
        LOG_I(TAG, "Đang kiểm tra lại máy chủ trước khi nạp...");
        checkOtaVersion();
    }

    if (!s_hasNewVersion) {
        LOG_W(TAG, "Không có phiên bản mới trên máy chủ để nạp.");
        return;
    }

    LOG_I(TAG, "==================================================");
    LOG_I(TAG, "🔥 ĐÃ GIỮ NÚT BOOT ĐỦ 3 GIÂY -> BẮT ĐẦU NẠP OTA!");
    LOG_I(TAG, "Phiên bản mục tiêu: %s", s_cachedOtaInfo.version.c_str());
    LOG_I(TAG, "==================================================");

    OtaService::startUpdate(
        s_cachedOtaInfo.firmwareUrl,
        s_cachedOtaInfo.clearNvs,
        s_cachedOtaInfo.changelog,
        s_cachedOtaInfo.releaseDate,
        [](int percent, size_t dl, size_t total) {
            static int lastPct = -1;
            if (percent % 10 == 0 && percent != lastPct) {
                lastPct = percent;
                LOG_I(TAG, "[OTA] Tiến trình nạp: %d%% (%u / %u KB)", percent, (uint32_t)(dl / 1024), (uint32_t)(total / 1024));
            }
        },
        [](OtaState state, const char *msg) {
            if (state == OtaState::SUCCESS) {
                LOG_I(TAG, "✅ [OTA] %s", msg);
            } else if (state == OtaState::ERROR) {
                LOG_E(TAG, "❌ [OTA] %s", msg);
            }
        }
    );
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  LOG_I(TAG, "=================================");
  LOG_I(TAG, "ESP32 Firmware: %s (%s)", FIRMWARE_VERSION, FIRMWARE_RELEASE_DATE);
  LOG_I(TAG, "OTA Manifest URL: %s", OTA_MANIFEST_URL);
  LOG_I(TAG, "=================================");

  OtaService::init();

  // Cấu hình LED và Nút BOOT (Rule 3)
  pinMode(PIN_LED_BUILTIN, OUTPUT);
  pinMode(PIN_BUTTON_BOOT, INPUT_PULLUP);

  // Kết nối WiFi (Rule 5)
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  LOG_I(TAG, "Đang kết nối WiFi: '%s' ...", WIFI_SSID);
}

void loop() {
  static unsigned long lastToggle = 0;
  static bool ledState = false;
  static unsigned long lastWifiCheck = 0;
  static unsigned long bootPressStart = 0;
  static bool bootTriggered = false;

  unsigned long currentMillis = millis();

  // 1. Nhấp nháy LED built-in (chớp nhanh 100ms khi đang OTA, 1000ms bình thường)
  unsigned long blinkInterval = OtaService::isUpdating() ? 100 : 1000;
  if (currentMillis - lastToggle >= blinkInterval) {
    lastToggle = currentMillis;
    ledState = !ledState;
    digitalWrite(PIN_LED_BUILTIN, ledState ? HIGH : LOW);
  }

  // 2. Xử lý giữ nút BOOT 3 giây để kích hoạt cập nhật OTA
  if (digitalRead(PIN_BUTTON_BOOT) == LOW) {
    if (bootPressStart == 0) {
      bootPressStart = currentMillis;
      bootTriggered = false;
      LOG_I(TAG, "Nút BOOT được nhấn! Giữ tiếp 3 giây để kích hoạt OTA...");
    } else if (!bootTriggered && (currentMillis - bootPressStart >= 3000)) {
      bootTriggered = true;
      triggerOtaUpdate();
    }
  } else {
    bootPressStart = 0;
    bootTriggered = false;
  }

  // 3. Quản lý kết nối WiFi & Kiểm tra thông tin phiên bản từ xa
  if (WiFi.status() == WL_CONNECTED) {
    if (!s_otaChecked) {
      s_otaChecked = true;
      s_lastOtaCheckTime = currentMillis;
      LOG_I(TAG, "WiFi đã kết nối! IP: %s (RSSI: %d dBm)", 
            WiFi.localIP().toString().c_str(), WiFi.RSSI());
      checkOtaVersion();
    } else if (currentMillis - s_lastOtaCheckTime >= 60000) {
      // Tự động kiểm tra lại sau mỗi 60 giây nếu chưa cập nhật
      s_lastOtaCheckTime = currentMillis;
      checkOtaVersion();
    }
  } else {
    // Reconnect định kỳ mỗi 12 giây nếu mất kết nối (Rule 5)
    if (currentMillis - lastWifiCheck >= 12000) {
      lastWifiCheck = currentMillis;
      if (s_otaChecked) {
        LOG_W(TAG, "Mất kết nối WiFi, đang kết nối lại...");
        s_otaChecked = false;
      }
      WiFi.reconnect();
    }
  }

  // Nhường CPU cho FreeRTOS IDLE task reset Watchdog Timer (Rule 8)
  vTaskDelay(pdMS_TO_TICKS(10));
}