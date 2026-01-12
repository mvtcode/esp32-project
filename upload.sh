#!/bin/bash

# Script tự động upload code lên ESP32
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
echo ""
echo "📤 Đang upload..."

pio run --target upload --upload-port "$PORT"

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Upload thành công!"
    echo ""
    read -p "Bạn có muốn mở Serial Monitor không? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        pio device monitor --port "$PORT"
    fi
else
    echo ""
    echo "❌ Upload thất bại!"
    exit 1
fi
