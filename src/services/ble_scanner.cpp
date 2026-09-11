#include "ble_scanner.h"
#include "log.h"

static const char *TAG = "BleScanner";

// Callback nhận kết quả khi phát hiện thiết bị BLE
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice *advertisedDevice) override {
        if (!advertisedDevice) return;

        std::string name = advertisedDevice->getName();
        std::string mac = advertisedDevice->getAddress().toString();
        int rssi = advertisedDevice->getRSSI();
        bool isHid = false;

        // Kiểm tra xem thiết bị có phát Service UUID 0x1812 (Human Interface Device) hay không
        if (advertisedDevice->haveServiceUUID()) {
            if (advertisedDevice->isAdvertisingService(NimBLEUUID((uint16_t)0x1812))) {
                isHid = true;
            }
        }

        // Kiểm tra Appearance (0x03C2 = Mouse, 0x03C0 = Generic HID, 0x03C1 = Keyboard)
        if (advertisedDevice->haveAppearance()) {
            uint16_t app = advertisedDevice->getAppearance();
            if (app == 0x03C2 || app == 0x03C0 || app == 0x03C1) {
                isHid = true;
            }
        }

        const char *displayName = name.empty() ? "(Chưa đặt tên / Unknown)" : name.c_str();

        if (isHid) {
            LOG_I(TAG, "🎯 [TÌM THẤY THIẾT BỊ HID / CHUỘT]");
            LOG_I(TAG, "   👉 MAC:  %s", mac.c_str());
            LOG_I(TAG, "   👉 Tên:  %s", displayName);
            LOG_I(TAG, "   👉 RSSI: %d dBm", rssi);
            LOG_I(TAG, "--------------------------------------------------");
        } else {
            LOG_D(TAG, "Tìm thấy: MAC=[%s], RSSI=%d, Name='%s'", mac.c_str(), rssi, displayName);
        }
    }
};

static AdvertisedDeviceCallbacks s_advertisedCallbacks;

// Callback khi kết thúc thời gian quét
static void scanCompleteCallback(NimBLEScanResults results) {
    LOG_I(TAG, "==================================================");
    LOG_I(TAG, "✅ HOÀN THÀNH ĐỢT QUÉT BLE! (Tìm thấy %u thiết bị)", results.getCount());
    LOG_I(TAG, "Nếu thấy chuột của bạn xuất hiện ở trên, hãy copy địa chỉ MAC");
    LOG_I(TAG, "và dán vào file 'include/mouse_config.h' cho Phase 2.");
    LOG_I(TAG, "💡 Bấm nút BOOT (IO0) để quét lại bất kỳ lúc nào.");
    LOG_I(TAG, "==================================================");
}

BleScanner::BleScanner()
    : _pBLEScan(nullptr),
      _isInitialized(false),
      _isScanning(false),
      _scanStartTime(0),
      _scanDurationSec(10) {
}

BleScanner::~BleScanner() {
    end();
}

bool BleScanner::begin() {
    if (_isInitialized) {
        return true;
    }

    LOG_I(TAG, "Đang khởi tạo BLE Device cho Scanner...");
    NimBLEDevice::init("ESP32_Scanner");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); // Công suất phát tối đa

    _pBLEScan = NimBLEDevice::getScan();
    if (!_pBLEScan) {
        LOG_E(TAG, "Không thể khởi tạo NimBLEScan!");
        return false;
    }

    _pBLEScan->setAdvertisedDeviceCallbacks(&s_advertisedCallbacks, false);
    _pBLEScan->setActiveScan(true);      // Active scan để yêu cầu response gói tin chứa tên
    _pBLEScan->setInterval(100);         // Quét liên tục (chu kỳ 100ms)
    _pBLEScan->setWindow(99);            // Cửa sổ nhận 99ms

    _isInitialized = true;
    LOG_I(TAG, "BLE Scanner đã sẵn sàng.");
    return true;
}

void BleScanner::end() {
    if (!_isInitialized) return;

    stopScan();
    _pBLEScan = nullptr;
    NimBLEDevice::deinit(true);
    _isInitialized = false;
    LOG_I(TAG, "BLE Scanner đã giải phóng tài nguyên.");
}

bool BleScanner::startScan(uint32_t durationSec) {
    if (!_isInitialized) {
        if (!begin()) return false;
    }

    if (isScanning()) {
        LOG_W(TAG, "Đang có tiến trình quét đang chạy...");
        return false;
    }

    _scanDurationSec = durationSec;
    _scanStartTime = millis();
    _isScanning = true;

    LOG_I(TAG, "==================================================");
    LOG_I(TAG, "🔍 BẮT ĐẦU QUÉT THIẾT BỊ BLE (Thời gian: %u giây)...", _scanDurationSec);
    LOG_I(TAG, "👉 Hãy bật chuột Bluetooth ở chế độ Pairing (Đèn nháy nhanh)");
    LOG_I(TAG, "==================================================");

    // Chạy scan async (non-blocking scan với scanCompleteCallback)
    bool started = _pBLEScan->start(_scanDurationSec, scanCompleteCallback, false);
    if (!started) {
        LOG_E(TAG, "Không thể bắt đầu quét BLE!");
        _isScanning = false;
        return false;
    }

    return true;
}

void BleScanner::stopScan() {
    if (_pBLEScan && isScanning()) {
        _pBLEScan->stop();
        _pBLEScan->clearResults();
        _isScanning = false;
        LOG_I(TAG, "Đã dừng quét BLE.");
    }
}

void BleScanner::update() {
    if (_isScanning) {
        if (!_pBLEScan || !_pBLEScan->isScanning()) {
            _isScanning = false;
            if (_pBLEScan) {
                _pBLEScan->clearResults();
            }
        }
    }
}

bool BleScanner::isScanning() const {
    if (_pBLEScan) {
        return _pBLEScan->isScanning();
    }
    return _isScanning;
}
