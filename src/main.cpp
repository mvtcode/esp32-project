#include <Arduino.h>
#include <WiFi.h>
#include "log.h"
#include "pin_config.h"
#include "version.h"
#include "mouse_config.h"
#include "services/ble_scanner.h"
#include "services/ble_mouse_client.h"

static const char *TAG = "Main";

#if BLE_APP_MODE == BLE_MODE_SCAN
// Đối tượng quản lý quét BLE (Phase 1)
static BleScanner s_bleScanner;
#elif BLE_APP_MODE == BLE_MODE_MOUSE
// Đối tượng quản lý kết nối chuột BLE (Phase 2)
static BleMouseClient s_bleMouseClient;
#endif

void setup() {
    Serial.begin(115200);
    delay(1000);

    LOG_I(TAG, "==================================================");
    LOG_I(TAG, "ESP32 Firmware: %s (%s)", FIRMWARE_VERSION, FIRMWARE_RELEASE_DATE);
    LOG_I(TAG, "DỰ ÁN: KẾT NỐI CHUỘT BLUETOOTH (ESP32 BLE HID HOST)");
    LOG_I(TAG, "==================================================");

    // 1. TẮT HOÀN TOÀN WIFI ĐỂ ƯU TIÊN BLUETOOTH & TIẾT KIỆM RAM (Theo yêu cầu)
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    LOG_I(TAG, "⚡ WiFi đã được tắt hoàn toàn để tối ưu tài nguyên cho Bluetooth.");

    // 2. Cấu hình GPIO (LED & Nút BOOT)
    pinMode(PIN_LED_BUILTIN, OUTPUT);
    pinMode(PIN_BUTTON_BOOT, INPUT_PULLUP);

    // 3. Khởi tạo theo chế độ cấu hình trong mouse_config.h
#if BLE_APP_MODE == BLE_MODE_SCAN
    LOG_I(TAG, "🚀 Đang khởi động Chế độ: PHASE 1 - BLE SCANNER...");
    if (s_bleScanner.begin()) {
        s_bleScanner.startScan(BLE_SCAN_DURATION_SEC);
        LOG_I(TAG, "💡 Gợi ý: Bấm nút BOOT (IO0) bất kỳ lúc nào để quét lại!");
    } else {
        LOG_E(TAG, "Khởi tạo BLE Scanner thất bại!");
    }
#elif BLE_APP_MODE == BLE_MODE_MOUSE
    LOG_I(TAG, "🚀 Đang khởi động Chế độ: PHASE 2 - BLE MOUSE CLIENT...");
    LOG_I(TAG, "🎯 MAC Chuột mục tiêu: %s", TARGET_MOUSE_MAC);
    if (!s_bleMouseClient.begin(TARGET_MOUSE_MAC)) {
        LOG_W(TAG, "Kết nối lần đầu chưa thành công, module sẽ tự động thử kết nối lại trong loop().");
    }
#endif
}

void loop() {
    static unsigned long lastToggle = 0;
    static bool ledState = false;
    static bool lastBootState = HIGH;
    static unsigned long lastDebounceTime = 0;

    unsigned long currentMillis = millis();

#if BLE_APP_MODE == BLE_MODE_SCAN
    // 1. Nhấp nháy LED khi quét (250ms khi quét, 1000ms khi nghỉ)
    bool isScanning = s_bleScanner.isScanning();
    unsigned long blinkInterval = isScanning ? 250 : 1000;
    if (currentMillis - lastToggle >= blinkInterval) {
        lastToggle = currentMillis;
        ledState = !ledState;
        digitalWrite(PIN_LED_BUILTIN, ledState ? HIGH : LOW);
    }

    // 2. Cập nhật tiến trình quét BLE
    s_bleScanner.update();

    // 3. Xử lý nút BOOT (IO0) để quét lại khi người dùng bấm
    bool currentBootState = digitalRead(PIN_BUTTON_BOOT);
    if (currentBootState != lastBootState) {
        lastDebounceTime = currentMillis;
    }
    if ((currentMillis - lastDebounceTime) > 50) {
        static bool s_buttonHandled = false;
        if (currentBootState == LOW && !s_buttonHandled) {
            s_buttonHandled = true;
            if (!s_bleScanner.isScanning()) {
                LOG_I(TAG, "🔘 Nút BOOT được nhấn! Đang bắt đầu đợt quét BLE mới...");
                s_bleScanner.startScan(BLE_SCAN_DURATION_SEC);
            } else {
                LOG_W(TAG, "🔘 Đang quét, vui lòng chờ...");
            }
        } else if (currentBootState == HIGH) {
            s_buttonHandled = false;
        }
    }
    lastBootState = currentBootState;

#elif BLE_APP_MODE == BLE_MODE_MOUSE
    // 1. Hiển thị trạng thái LED: Sáng liên tục khi đã kết nối chuột, nhấp nháy nhanh khi đang tìm
    bool isConnected = s_bleMouseClient.isConnected();
    if (isConnected) {
        digitalWrite(PIN_LED_BUILTIN, HIGH); // Sáng liên tục báo hiệu chuột online
    } else {
        if (currentMillis - lastToggle >= 300) {
            lastToggle = currentMillis;
            ledState = !ledState;
            digitalWrite(PIN_LED_BUILTIN, ledState ? HIGH : LOW);
        }
    }

    // 2. Cập nhật trạng thái và tự động kết nối lại nếu chuột sleep/tắt nguồn
    s_bleMouseClient.update();

    // 3. Xử lý nút BOOT: Bấm để ép kết nối lại ngay lập tức
    bool currentBootState = digitalRead(PIN_BUTTON_BOOT);
    if (currentBootState != lastBootState) {
        lastDebounceTime = currentMillis;
    }
    if ((currentMillis - lastDebounceTime) > 50) {
        static bool s_buttonHandled = false;
        if (currentBootState == LOW && !s_buttonHandled) {
            s_buttonHandled = true;
            LOG_I(TAG, "🔘 Nút BOOT được nhấn! Yêu cầu kết nối lại chuột ngay lập tức...");
            s_bleMouseClient.connectToMouse();
        } else if (currentBootState == HIGH) {
            s_buttonHandled = false;
        }
    }
    lastBootState = currentBootState;
#endif

    // Nhường CPU cho FreeRTOS IDLE task reset Watchdog Timer (Rule 8)
    vTaskDelay(pdMS_TO_TICKS(10));
}