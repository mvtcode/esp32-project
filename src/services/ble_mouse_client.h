#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <string>
#include <functional>

/**
 * @brief Enum định nghĩa các nút chuột
 */
enum class MouseButton {
    LEFT = 0,
    RIGHT = 1,
    MIDDLE = 2
};

/**
 * @brief Enum định nghĩa hành động nút chuột
 */
enum class ButtonAction {
    RELEASE = 0,
    PRESS = 1
};

/**
 * @brief Cấu trúc lưu trữ dữ liệu chuột thời gian thực
 */
struct MouseData {
    bool leftButton;
    bool rightButton;
    bool middleButton;
    int8_t deltaX;
    int8_t deltaY;
    int8_t wheel;
    int8_t pan;
    uint8_t rawButtons;
};

// Định nghĩa các kiểu Callback Event
typedef std::function<void(int8_t dx, int8_t dy)> MouseMoveCallback;
typedef std::function<void(MouseButton button, ButtonAction action)> MouseClickCallback;
typedef std::function<void(int8_t wheel)> MouseScrollCallback;

/**
 * @brief Module quản lý kết nối chuột BLE, đọc thông tin và parse HID report
 */
class BleMouseClient {
public:
    BleMouseClient();
    ~BleMouseClient();

    // Khởi tạo client với địa chỉ MAC mục tiêu
    bool begin(const char *targetMac);

    // Dừng và giải phóng tài nguyên kết nối
    void end();

    // Vòng lặp cập nhật trạng thái kết nối và tự phục hồi (non-blocking)
    void update();

    // Kiểm tra trạng thái đã kết nối
    bool isConnected() const;

    // Thực hiện kết nối tới chuột
    bool connectToMouse();

    // Đăng ký Event Callbacks (nếu muốn xử lý logic thêm bên ngoài)
    void onMove(MouseMoveCallback cb) { _moveCallback = cb; }
    void onClick(MouseClickCallback cb) { _clickCallback = cb; }
    void onScroll(MouseScrollCallback cb) { _scrollCallback = cb; }

    // Callback xử lý sự kiện ngắt kết nối
    void onDisconnected();

    // Callback xử lý khi nhận gói tin HID report từ chuột
    void handleReportNotify(NimBLERemoteCharacteristic *pChar, uint8_t *pData, size_t length, bool isNotify);

    // Callback xử lý khi nhận thông báo mức pin
    void handleBatteryNotify(NimBLERemoteCharacteristic *pChar, uint8_t *pData, size_t length, bool isNotify);

private:
    std::string _targetMac;
    NimBLEClient *_pClient;
    bool _isInitialized;
    bool _isConnected;
    bool _isConnecting;
    unsigned long _lastReconnectAttempt;
    uint8_t _batteryLevel;
    uint8_t _lastButtons;

    // Callbacks lưu trữ
    MouseMoveCallback _moveCallback;
    MouseClickCallback _clickCallback;
    MouseScrollCallback _scrollCallback;

    // Khám phá và đọc thông tin thiết bị
    void readDeviceInfo();

    // Đăng ký dịch vụ HID và Report Notification
    bool setupHidNotifications();

    // Đọc và đăng ký Battery Service
    void setupBatteryService();
};
