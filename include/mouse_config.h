#pragma once

/**
 * mouse_config.h - Cấu hình tập trung cho tính năng Bluetooth Mouse trên ESP32
 */

// Chế độ ứng dụng:
// 1 = Phase 1: Quét (Scan) tìm thiết bị BLE và địa chỉ MAC
// 2 = Phase 2: Kết nối trực tiếp đến chuột theo MAC và đọc tọa độ/nút bấm
#define BLE_MODE_SCAN   1
#define BLE_MODE_MOUSE  2

// Đã chuyển sang Chế độ Phase 2: Kết nối chuột
#define BLE_APP_MODE    BLE_MODE_MOUSE

// Địa chỉ MAC của chuột Bluetooth UGREEN BLE Mouse quét được từ Phase 1
#define TARGET_MOUSE_MAC "d7:54:3f:17:16:12"

// Thời gian mỗi lần quét BLE trong Phase 1 (giây)
#define BLE_SCAN_DURATION_SEC 10

// Khoảng thời gian tự động thử kết nối lại nếu chuột mất kết nối trong Phase 2 (ms)
#define BLE_RECONNECT_INTERVAL_MS 5000
