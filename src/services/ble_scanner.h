#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>

/**
 * @brief Module quét thiết bị BLE xung quanh, nhận diện chuột và bàn phím HID
 */
class BleScanner {
public:
    BleScanner();
    ~BleScanner();

    // Khởi tạo BLE Scanner
    bool begin();

    // Dừng và giải phóng tài nguyên scanner
    void end();

    // Bắt đầu quét thiết bị trong khoảng thời gian durationSec
    bool startScan(uint32_t durationSec = 10);

    // Dừng quét khẩn cấp
    void stopScan();

    // Hàm cập nhật trạng thái quét trong vòng lặp chính (non-blocking)
    void update();

    // Kiểm tra xem scanner có đang quét hay không
    bool isScanning() const;

private:
    NimBLEScan *_pBLEScan;
    bool _isInitialized;
    bool _isScanning;
    unsigned long _scanStartTime;
    uint32_t _scanDurationSec;
};
