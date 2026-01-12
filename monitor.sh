#!/bin/bash

# Script mở Serial Monitor
# Tự động tìm cổng serial phù hợp

echo "🔍 Đang tìm cổng ESP32..."

# Tìm cổng usbserial hoặc SLAB_USBtoUART
PORT=$(ls /dev/cu.* 2>/dev/null | grep -E "(usbserial|SLAB_USBtoUART|wchusbserial)" | head -n 1)

if [ -z "$PORT" ]; then
    echo "❌ Không tìm thấy ESP32. Vui lòng kiểm tra:"
    echo "   1. ESP32 đã được cắm vào máy chưa?"
    echo "   2. Driver USB-to-Serial đã được cài đặt chưa?"
    echo ""
    echo "📋 Danh sách tất cả các cổng:"
    pio device list
    exit 1
fi

echo "✅ Tìm thấy ESP32 tại: $PORT"
echo "📡 Đang mở Serial Monitor..."
echo "   (Nhấn Ctrl+C để thoát)"
echo ""

pio device monitor --port "$PORT"
