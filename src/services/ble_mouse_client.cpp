#include "ble_mouse_client.h"
#include "log.h"
#include "mouse_config.h"

static const char *TAG = "BleMouse";

static BleMouseClient *s_instance = nullptr;

// Callbacks xử lý sự kiện kết nối của NimBLE Client
class BleMouseClientCallbacks : public NimBLEClientCallbacks {
public:
    void onConnect(NimBLEClient *pClient) override {
        LOG_I(TAG, "🔗 Đã kết nối vật lý với chuột qua BLE (MAC: %s)", 
              pClient->getPeerAddress().toString().c_str());
    }

    void onDisconnect(NimBLEClient *pClient) override {
        LOG_W(TAG, "⚠️ Mất kết nối với chuột Bluetooth (MAC: %s)", 
              pClient->getPeerAddress().toString().c_str());
        if (s_instance) {
            s_instance->onDisconnected();
        }
    }

    bool onConnParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params) override {
        LOG_D(TAG, "Cập nhật tham số kết nối từ chuột (Interval: %d - %d, Latency: %d)",
              params->itvl_min, params->itvl_max, params->latency);
        return true; // Chấp thuận để chuột phản hồi nhanh và mượt mà
    }

    uint32_t onPassKeyRequest() override {
        LOG_D(TAG, "Chuột yêu cầu Passkey (Mặc định 000000)");
        return 0;
    }

    void onAuthenticationComplete(ble_gap_conn_desc *desc) override {
        if (desc && desc->sec_state.encrypted) {
            LOG_I(TAG, "🔒 [BẢO MẬT BLE] Kết nối đã được MÃ HÓA & BONDING thành công!");
        } else {
            LOG_W(TAG, "🔒 [BẢO MẬT BLE] Chưa được mã hóa, có thể chuột dùng kết nối tiêu chuẩn.");
        }
    }

    bool onConfirmPIN(uint32_t pin) override {
        LOG_D(TAG, "Xác nhận mã PIN ghép đôi: %u", pin);
        return true;
    }
};

static BleMouseClientCallbacks s_clientCallbacks;

BleMouseClient::BleMouseClient()
    : _targetMac(""),
      _pClient(nullptr),
      _isInitialized(false),
      _isConnected(false),
      _isConnecting(false),
      _lastReconnectAttempt(0),
      _batteryLevel(100),
      _lastButtons(0),
      _moveCallback(nullptr),
      _clickCallback(nullptr),
      _scrollCallback(nullptr) {
    s_instance = this;
}

BleMouseClient::~BleMouseClient() {
    end();
    s_instance = nullptr;
}

bool BleMouseClient::begin(const char *targetMac) {
    if (targetMac == nullptr || strlen(targetMac) == 0) {
        LOG_E(TAG, "Địa chỉ MAC mục tiêu không hợp lệ!");
        return false;
    }

    _targetMac = targetMac;

    if (!_isInitialized) {
        LOG_I(TAG, "Đang khởi tạo BLE Device cho Mouse Host Client...");
        NimBLEDevice::init("ESP32_MouseHost");
        NimBLEDevice::setPower(ESP_PWR_LVL_P9);

        // Thiết lập cơ chế Bảo mật và Ghép đôi (Bắt buộc cho chuẩn BLE HID Mouse / HOGP)
        NimBLEDevice::setSecurityAuth(true, true, true); // Bonding, MITM, Secure Connections
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
        NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
        NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);

        _isInitialized = true;
    }

    return connectToMouse();
}

void BleMouseClient::end() {
    if (_pClient) {
        if (_pClient->isConnected()) {
            _pClient->disconnect();
        }
        NimBLEDevice::deleteClient(_pClient);
        _pClient = nullptr;
    }
    _isConnected = false;
    _isConnecting = false;
    _isInitialized = false;
    _lastButtons = 0;
    LOG_I(TAG, "Đã dọn dẹp và giải phóng tài nguyên BleMouseClient.");
}

bool BleMouseClient::connectToMouse() {
    if (_isConnecting) {
        return false;
    }

    if (_pClient && _pClient->isConnected()) {
        _isConnected = true;
        return true;
    }

    _isConnecting = true;
    LOG_I(TAG, "==================================================");
    LOG_I(TAG, "⏳ ĐANG KẾT NỐI TỚI CHUỘT MỤC TIÊU: %s ...", _targetMac.c_str());
    LOG_I(TAG, "👉 Hãy chắc chắn chuột đang bật nguồn và trong tầm phủ sóng");
    LOG_I(TAG, "==================================================");

    if (!_pClient) {
        _pClient = NimBLEDevice::createClient();
        if (!_pClient) {
            LOG_E(TAG, "Không thể tạo đối tượng NimBLEClient!");
            _isConnecting = false;
            return false;
        }
        _pClient->setClientCallbacks(&s_clientCallbacks, false);
        _pClient->setConnectionParams(6, 12, 0, 400); // 7.5ms - 15ms interval
        _pClient->setConnectTimeout(6);
    }

    NimBLEAddress targetAddr(_targetMac);
    bool ok = _pClient->connect(targetAddr, false);
    if (!ok) {
        LOG_W(TAG, "Chưa thể kết nối tới chuột [%s]. Sẽ tự động thử lại sau %d giây.", 
              _targetMac.c_str(), BLE_RECONNECT_INTERVAL_MS / 1000);
        _isConnected = false;
        _isConnecting = false;
        _lastReconnectAttempt = millis();
        return false;
    }

    LOG_I(TAG, "✅ KẾT NỐI BLE THÀNH CÔNG! Đang bảo mật & khám phá toàn bộ Services...");

    // 1. Kích hoạt bảo mật mã hóa kết nối
    _pClient->secureConnection();

    // Chờ 300ms để BLE GAP hoàn tất kết nối & mã hóa
    vTaskDelay(pdMS_TO_TICKS(300));

    // 2. Khám phá TOÀN BỘ danh sách Dịch vụ (Services) từ chuột
    std::vector<NimBLERemoteService *> *pServices = _pClient->getServices(true);
    if (!pServices || pServices->empty()) {
        LOG_E(TAG, "Không thể lấy danh sách Services từ chuột!");
        _isConnected = false;
        _isConnecting = false;
        _lastReconnectAttempt = millis();
        return false;
    }

    LOG_I(TAG, "==================================================");
    LOG_I(TAG, "🔍 TÌM THẤY %u DỊCH VỤ (SERVICES) TRÊN CHUỘT:", pServices->size());
    for (auto *pService : *pServices) {
        if (!pService) continue;
        LOG_I(TAG, "   🔹 Service: %s", pService->getUUID().toString().c_str());
    }
    LOG_I(TAG, "==================================================");

    // 3. Đọc thông tin thiết bị (Tên, Hãng, Model, Firmware)
    readDeviceInfo();

    // 4. Khám phá Battery Service (Đọc mức pin)
    setupBatteryService();

    // 5. Khám phá và đăng ký nhận dữ liệu chuột từ HID Service
    bool hidOk = setupHidNotifications();
    if (!hidOk) {
        LOG_W(TAG, "Cảnh báo: Không thể đăng ký nhận dữ liệu chuột!");
    }

    _isConnected = true;
    _isConnecting = false;
    _lastButtons = 0;
    LOG_I(TAG, "==================================================");
    LOG_I(TAG, "🎉 CHUỘT ĐÃ SẴN SÀNG! HÃY MOVE, CLICK HOẶC SCROLL");
    LOG_I(TAG, "==================================================");
    return true;
}

void BleMouseClient::readDeviceInfo() {
    if (!_pClient || !_pClient->isConnected()) return;

    LOG_I(TAG, "--------------------------------------------------");
    LOG_I(TAG, "📋 THÔNG TIN THIẾT BỊ CHUỘT:");

    // Tên thiết bị từ Generic Access (0x1800)
    NimBLERemoteService *pGenericService = _pClient->getService(NimBLEUUID((uint16_t)0x1800));
    if (pGenericService) {
        NimBLERemoteCharacteristic *pNameChar = pGenericService->getCharacteristic(NimBLEUUID((uint16_t)0x2A00));
        if (pNameChar && pNameChar->canRead()) {
            std::string name = pNameChar->readValue();
            LOG_I(TAG, "   👉 Tên thiết bị:  %s", name.c_str());
        }
    }

    // Thông tin từ Device Information Service (0x180A)
    NimBLERemoteService *pDevInfoService = _pClient->getService(NimBLEUUID((uint16_t)0x180A));
    if (pDevInfoService) {
        // Manufacturer Name (0x2A29)
        NimBLERemoteCharacteristic *pMfgChar = pDevInfoService->getCharacteristic(NimBLEUUID((uint16_t)0x2A29));
        if (pMfgChar && pMfgChar->canRead()) {
            std::string mfg = pMfgChar->readValue();
            LOG_I(TAG, "   👉 Hãng sản xuất: %s", mfg.c_str());
        }

        // Model Number (0x2A24)
        NimBLERemoteCharacteristic *pModelChar = pDevInfoService->getCharacteristic(NimBLEUUID((uint16_t)0x2A24));
        if (pModelChar && pModelChar->canRead()) {
            std::string model = pModelChar->readValue();
            LOG_I(TAG, "   👉 Model:         %s", model.c_str());
        }

        // Firmware Revision (0x2A26)
        NimBLERemoteCharacteristic *pFwChar = pDevInfoService->getCharacteristic(NimBLEUUID((uint16_t)0x2A26));
        if (pFwChar && pFwChar->canRead()) {
            std::string fw = pFwChar->readValue();
            LOG_I(TAG, "   👉 Firmware Rev:  %s", fw.c_str());
        }
    }
    LOG_I(TAG, "--------------------------------------------------");
}

void BleMouseClient::setupBatteryService() {
    if (!_pClient || !_pClient->isConnected()) return;

    NimBLERemoteService *pBatteryService = _pClient->getService(NimBLEUUID((uint16_t)0x180F));
    if (!pBatteryService) {
        LOG_D(TAG, "Chuột không hỗ trợ Battery Service (0x180F).");
        return;
    }

    NimBLERemoteCharacteristic *pBatChar = pBatteryService->getCharacteristic(NimBLEUUID((uint16_t)0x2A19));
    if (pBatChar) {
        if (pBatChar->canRead()) {
            _batteryLevel = pBatChar->readValue<uint8_t>();
            LOG_I(TAG, "🔋 [PIN CHUỘT] Mức pin hiện tại: %u%%", _batteryLevel);
        }

        if (pBatChar->canNotify()) {
            pBatChar->subscribe(true, [this](NimBLERemoteCharacteristic *pChar, uint8_t *pData, size_t len, bool isNotify) {
                handleBatteryNotify(pChar, pData, len, isNotify);
            });
        }
    }
}

bool BleMouseClient::setupHidNotifications() {
    if (!_pClient || !_pClient->isConnected()) return false;

    bool subscribedAny = false;

    // 1. Kiểm tra Dịch vụ HID chuẩn 0x1812
    NimBLERemoteService *pHidService = _pClient->getService(NimBLEUUID((uint16_t)0x1812));
    if (pHidService) {
        LOG_I(TAG, "==================================================");
        LOG_I(TAG, "🎯 TÌM THẤY DỊCH VỤ HID (0x1812)!");

        // A. Đặt Protocol Mode sang Report Mode (0x01)
        NimBLERemoteCharacteristic *pProtoChar = pHidService->getCharacteristic(NimBLEUUID((uint16_t)0x2A4E));
        if (pProtoChar) {
            LOG_I(TAG, "   👉 Tìm thấy Protocol Mode (0x2A4E)");
            if (pProtoChar->canWrite()) {
                uint8_t mode = 0x01;
                pProtoChar->writeValue(&mode, 1, false);
                LOG_I(TAG, "      Đã gửi thiết lập Report Protocol Mode (0x01)");
            }
        }

        // B. Truy vấn Characteristic HID Report (0x2A4D)
        LOG_I(TAG, "   Đang tìm Characteristic HID Report (0x2A4D)...");
        NimBLERemoteCharacteristic *pReportChar = pHidService->getCharacteristic(NimBLEUUID((uint16_t)0x2A4D));
        if (pReportChar) {
            LOG_I(TAG, "   👉 Tìm thấy HID Report (0x2A4D) [Notify: %d, Read: %d, Write: %d]",
                  pReportChar->canNotify(), pReportChar->canRead(), pReportChar->canWrite());
        }

        // C. Truy vấn Characteristic Boot Mouse Report (0x2A33)
        LOG_I(TAG, "   Đang tìm Characteristic Boot Mouse Report (0x2A33)...");
        NimBLERemoteCharacteristic *pBootChar = pHidService->getCharacteristic(NimBLEUUID((uint16_t)0x2A33));
        if (pBootChar) {
            LOG_I(TAG, "   👉 Tìm thấy Boot Mouse Report (0x2A33) [Notify: %d, Read: %d, Write: %d]",
                  pBootChar->canNotify(), pBootChar->canRead(), pBootChar->canWrite());
        }

        // D. Lấy danh sách toàn bộ Characteristic đã được nạp trong HID Service
        auto pChars = pHidService->getCharacteristics(false);
        if (pChars && !pChars->empty()) {
            LOG_I(TAG, "   📋 Tổng số Characteristic đã nạp trong HID: %u", pChars->size());
            for (auto *pChar : *pChars) {
                if (!pChar) continue;
                NimBLEUUID cUuid = pChar->getUUID();
                LOG_I(TAG, "      🔹 Char [%s] - Notify: %d, Read: %d, Write: %d",
                      cUuid.toString().c_str(), pChar->canNotify(), pChar->canRead(), pChar->canWrite());

                if (pChar->canNotify()) {
                    bool ok = pChar->subscribe(true, [this](NimBLERemoteCharacteristic *c, uint8_t *data, size_t len, bool isNotify) {
                        handleReportNotify(c, data, len, isNotify);
                    }, false);
                    if (!ok) {
                        ok = pChar->subscribe(true, [this](NimBLERemoteCharacteristic *c, uint8_t *data, size_t len, bool isNotify) {
                            handleReportNotify(c, data, len, isNotify);
                        }, true);
                    }

                    if (ok) {
                        LOG_I(TAG, "      🎯 ✅ ĐÃ SUBSCRIBE THÀNH CÔNG NHẬN DỮ LIỆU TỪ [%s]!", cUuid.toString().c_str());
                        subscribedAny = true;
                    } else {
                        LOG_E(TAG, "      ❌ Lỗi khi Subscribe [%s]!", cUuid.toString().c_str());
                    }
                }
            }
        }
        LOG_I(TAG, "==================================================");
    } else {
        LOG_E(TAG, "Không tìm thấy HID Service (0x1812) trên chuột!");
    }

    // 2. Nếu chưa subscribe được trên 0x1812, kiểm tra thêm Vendor Service 0xFE59
    if (!subscribedAny) {
        NimBLERemoteService *pVendorService = _pClient->getService(NimBLEUUID((uint16_t)0xFE59));
        if (pVendorService) {
            LOG_I(TAG, "ℹ️ Đang kiểm tra Vendor Service (0xFE59)...");
            auto pChars = pVendorService->getCharacteristics(false);
            if (pChars) {
                for (auto *pChar : *pChars) {
                    if (pChar && pChar->canNotify()) {
                        bool ok = pChar->subscribe(true, [this](NimBLERemoteCharacteristic *c, uint8_t *data, size_t len, bool isNotify) {
                            handleReportNotify(c, data, len, isNotify);
                        }, false);
                        if (ok) {
                            LOG_I(TAG, "🎯 ✅ Đã Subscribe thành công trên Vendor Char [%s]!", pChar->getUUID().toString().c_str());
                            subscribedAny = true;
                        }
                    }
                }
            }
        }
    }

    return subscribedAny;
}

void BleMouseClient::handleReportNotify(NimBLERemoteCharacteristic *pChar, uint8_t *pData, size_t length, bool isNotify) {
    if (!pData || length == 0) return;

    // Chuẩn bị chuỗi HEX hiển thị dữ liệu thô
    char hexBuffer[64] = {0};
    for (size_t i = 0; i < length && i < 16; i++) {
        snprintf(hexBuffer + strlen(hexBuffer), sizeof(hexBuffer) - strlen(hexBuffer), "%02X ", pData[i]);
    }

    uint8_t buttons = 0;
    int8_t deltaX = 0;
    int8_t deltaY = 0;
    int8_t wheel = 0;

    // Phân tích định dạng gói tin HID chuột (chuẩn HOGP) với mọi độ dài từ 1 byte trở lên
    if (length == 1) {
        // [Buttons]
        buttons = pData[0];
    } else if (length == 2) {
        // [ReportID, Buttons] hoặc [Buttons, 0]
        if (pData[0] == 0x01 || pData[0] == 0x02 || pData[0] == 0x03) {
            buttons = pData[1];
        } else {
            buttons = pData[0];
        }
    } else if (length == 3) {
        // [Buttons, dX, dY]
        buttons = pData[0];
        deltaX = (int8_t)pData[1];
        deltaY = (int8_t)pData[2];
    } else if (length == 4) {
        // [Buttons, dX, dY, Wheel]
        buttons = pData[0];
        deltaX = (int8_t)pData[1];
        deltaY = (int8_t)pData[2];
        wheel = (int8_t)pData[3];
    } else if (length >= 5) {
        // Có Report ID ở byte đầu tiên (Report ID = 1, 2, 3...)
        if (pData[0] == 0x01 || pData[0] == 0x02 || pData[0] == 0x03) {
            buttons = pData[1];
            deltaX = (int8_t)pData[2];
            deltaY = (int8_t)pData[3];
            wheel = (int8_t)pData[4];
        } else {
            buttons = pData[0];
            deltaX = (int8_t)pData[1];
            deltaY = (int8_t)pData[2];
            wheel = (int8_t)pData[3];
        }
    }

    // 1. PHÁT HIỆN SỰ KIỆN CLICK (CLICK EVENT: PRESS & RELEASE)
    uint8_t changedButtons = buttons ^ _lastButtons;
    if (changedButtons != 0) {
        // Nút Trái (Bit 0)
        if (changedButtons & 0x01) {
            bool isPress = (buttons & 0x01) != 0;
            LOG_I(TAG, "🔔 [EVENT: CLICK] Chuột TRÁI -> %s [RAW: %s]", 
                  isPress ? "NHẤN (PRESSED) ⬇️" : "NHẢ (RELEASED) ⬆️", hexBuffer);
            if (_clickCallback) _clickCallback(MouseButton::LEFT, isPress ? ButtonAction::PRESS : ButtonAction::RELEASE);
        }
        // Nút Phải (Bit 1)
        if (changedButtons & 0x02) {
            bool isPress = (buttons & 0x02) != 0;
            LOG_I(TAG, "🔔 [EVENT: CLICK] Chuột PHẢI -> %s [RAW: %s]", 
                  isPress ? "NHẤN (PRESSED) ⬇️" : "NHẢ (RELEASED) ⬆️", hexBuffer);
            if (_clickCallback) _clickCallback(MouseButton::RIGHT, isPress ? ButtonAction::PRESS : ButtonAction::RELEASE);
        }
        // Nút Giữa / Con lăn (Bit 2)
        if (changedButtons & 0x04) {
            bool isPress = (buttons & 0x04) != 0;
            LOG_I(TAG, "🔔 [EVENT: CLICK] Chuột GIỮA -> %s [RAW: %s]", 
                  isPress ? "NHẤN (PRESSED) ⬇️" : "NHẢ (RELEASED) ⬆️", hexBuffer);
            if (_clickCallback) _clickCallback(MouseButton::MIDDLE, isPress ? ButtonAction::PRESS : ButtonAction::RELEASE);
        }
        _lastButtons = buttons;
    } else if (deltaX == 0 && deltaY == 0 && wheel == 0) {
        // Gói tin nhận được khi chuột đứng yên (phục vụ kiểm tra dữ liệu)
        LOG_I(TAG, "📍 [ĐỨNG YÊN] Gói tin nhận được: Len=%d | Nút: 0x%02X | RAW: [ %s]", 
              length, buttons, hexBuffer);
    }

    // 2. PHÁT HIỆN SỰ KIỆN DI CHUYỂN (MOVE EVENT)
    if (deltaX != 0 || deltaY != 0) {
        LOG_I(TAG, "🚀 [EVENT: MOVE] dX: %+4d | dY: %+4d", deltaX, deltaY);
        if (_moveCallback) _moveCallback(deltaX, deltaY);
    }

    // 3. PHÁT HIỆN SỰ KIỆN CUỘN (SCROLL EVENT)
    if (wheel != 0) {
        LOG_I(TAG, "🎡 [EVENT: SCROLL] %s (Bước: %+d)", 
              wheel > 0 ? "CUỘN LÊN ⬆️" : "CUỘN XUỐNG ⬇️", wheel);
        if (_scrollCallback) _scrollCallback(wheel);
    }
}

void BleMouseClient::handleBatteryNotify(NimBLERemoteCharacteristic *pChar, uint8_t *pData, size_t length, bool isNotify) {
    if (pData && length > 0) {
        _batteryLevel = pData[0];
        LOG_I(TAG, "🔋 [EVENT: BATTERY] Cập nhật mức pin mới: %u%%", _batteryLevel);
    }
}

void BleMouseClient::onDisconnected() {
    _isConnected = false;
    _isConnecting = false;
    _lastButtons = 0;
    _lastReconnectAttempt = millis();
}

void BleMouseClient::update() {
    // Cơ chế tự phục hồi kết nối (Self-Healing / Auto-reconnect - Rule 10)
    if (!_isConnected && !_isConnecting) {
        unsigned long currentMillis = millis();
        if (currentMillis - _lastReconnectAttempt >= BLE_RECONNECT_INTERVAL_MS) {
            _lastReconnectAttempt = currentMillis;
            LOG_I(TAG, "Đang thử kết nối lại với chuột [%s]...", _targetMac.c_str());
            connectToMouse();
        }
    }
}

bool BleMouseClient::isConnected() const {
    return _isConnected;
}
