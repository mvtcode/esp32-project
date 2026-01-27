#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Script debug đơn giản để test đăng ký vân tay
"""

import serial
import time

PORT = "COM8"
BAUDRATE = 115200

def hex_to_bytes(hex_string):
    return bytes.fromhex(hex_string)

def bytes_to_hex(data):
    return ' '.join([f'{b:02X}' for b in data])

def main():
    print("🔍 DEBUG: Test đăng ký vân tay TZM1026\n")
    
    try:
        ser = serial.Serial(PORT, BAUDRATE, timeout=2)
        print(f"✅ Kết nối {PORT} thành công!\n")
        time.sleep(0.5)
        
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        # Test 1: Lấy số người dùng ban đầu
        print("=" * 60)
        print("TEST 1: Lấy số người dùng ban đầu")
        print("=" * 60)
        cmd = hex_to_bytes("F5090000000009F5")
        print(f"Gửi: {bytes_to_hex(cmd)}")
        ser.write(cmd)
        time.sleep(0.3)
        
        response = ser.read(8)
        print(f"Nhận: {bytes_to_hex(response)}")
        if len(response) >= 8:
            count = (response[2] << 8) | response[3]
            print(f"Số người dùng: {count}\n")
        
        # Test 2: Đăng ký vân tay (chế độ NCNR - tự động gán ID)
        print("=" * 60)
        print("TEST 2: Đăng ký vân tay (NCNR)")
        print("=" * 60)
        cmd = hex_to_bytes("F5010000010000F5")
        print(f"Gửi: {bytes_to_hex(cmd)}")
        print("Vui lòng chuẩn bị chạm ngón tay...\n")
        ser.write(cmd)
        
        # Đọc tất cả phản hồi trong 20 giây
        print("Đang theo dõi phản hồi từ module:")
        print("-" * 60)
        
        for i in range(20):
            time.sleep(1)
            if ser.in_waiting > 0:
                response = ser.read(ser.in_waiting)
                print(f"[{i+1}s] Nhận {len(response)} bytes: {bytes_to_hex(response)}")
                
                # Phân tích từng gói 8 bytes
                for j in range(0, len(response), 8):
                    if j + 8 <= len(response):
                        packet = response[j:j+8]
                        if len(packet) == 8 and packet[0] == 0xF5 and packet[7] == 0xF5:
                            cmd_byte = packet[1]
                            status = packet[4]
                            step = packet[5]
                            print(f"      → Gói hợp lệ: CMD=0x{cmd_byte:02X}, Status=0x{status:02X}, Step={step}")
                            
                            if status == 0x00:
                                print(f"      ✅ THÀNH CÔNG!")
                            elif status == 0x01:
                                print(f"      ⏳ Chờ chạm ngón tay (bước {step})")
                            elif status == 0x02:
                                print(f"      🔄 Đang xử lý (bước {step})")
        
        print("-" * 60)
        
        # Test 3: Kiểm tra lại số người dùng
        print("\n" + "=" * 60)
        print("TEST 3: Kiểm tra lại số người dùng sau khi đăng ký")
        print("=" * 60)
        
        # Đợi 2 giây để module lưu dữ liệu
        time.sleep(2)
        
        cmd = hex_to_bytes("F5090000000009F5")
        print(f"Gửi: {bytes_to_hex(cmd)}")
        ser.write(cmd)
        time.sleep(0.3)
        
        response = ser.read(8)
        print(f"Nhận: {bytes_to_hex(response)}")
        if len(response) >= 8:
            count = (response[2] << 8) | response[3]
            print(f"Số người dùng: {count}")
            
            if count > 0:
                print("✅ Đăng ký thành công và đã lưu vào bộ nhớ!")
            else:
                print("⚠️  Cảnh báo: Số người dùng vẫn là 0!")
                print("    → Module có thể chưa lưu vào bộ nhớ flash")
                print("    → Hoặc cần lệnh xác nhận/commit riêng")
        
        ser.close()
        print("\n✅ Hoàn thành test!")
        
    except Exception as e:
        print(f"❌ Lỗi: {e}")

if __name__ == "__main__":
    main()
