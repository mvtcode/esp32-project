#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Script test theo đúng quy trình của Arduino code gốc
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
    print("🧪 TEST: Theo quy trình Arduino code gốc\n")
    
    try:
        ser = serial.Serial(PORT, BAUDRATE, timeout=2)
        print(f"✅ Kết nối {PORT} thành công!\n")
        time.sleep(0.5)
        
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        # Bước 1: Kiểm tra số người dùng ban đầu
        print("=" * 70)
        print("BƯỚC 1: Kiểm tra số người dùng ban đầu")
        print("=" * 70)
        cmd = hex_to_bytes("F5090000000009F5")
        print(f"Gửi: {bytes_to_hex(cmd)}")
        ser.write(cmd)
        time.sleep(0.3)
        
        response = ser.read(8)
        print(f"Nhận: {bytes_to_hex(response)}")
        if len(response) >= 8:
            count = (response[2] << 8) | response[3]
            print(f"→ Số người dùng: {count}\n")
        
        # Bước 2: Thiết lập chế độ đăng ký (lệnh 3)
        print("=" * 70)
        print("BƯỚC 2: Thiết lập chế độ đăng ký vân tay")
        print("=" * 70)
        cmd = hex_to_bytes("F526000000000026F5")
        print(f"Gửi: {bytes_to_hex(cmd)}")
        ser.write(cmd)
        time.sleep(0.3)
        
        response = ser.read(8)
        print(f"Nhận: {bytes_to_hex(response)}")
        print()
        
        # Bước 3: Đăng ký vân tay với ID = 1 (lệnh 15 - 3CNR)
        print("=" * 70)
        print("BƯỚC 3: Đăng ký vân tay với ID = 1 (chế độ 3CNR)")
        print("=" * 70)
        cmd = hex_to_bytes("F5010001010101F5")
        print(f"Gửi: {bytes_to_hex(cmd)}")
        print("Vui lòng chuẩn bị chạm ngón tay...\n")
        ser.write(cmd)
        
        # Đọc tất cả phản hồi trong 25 giây
        print("Đang theo dõi phản hồi:")
        print("-" * 70)
        
        last_status = None
        for i in range(25):
            time.sleep(1)
            if ser.in_waiting > 0:
                response = ser.read(ser.in_waiting)
                
                # Phân tích từng gói 8 bytes
                for j in range(0, len(response), 8):
                    if j + 8 <= len(response):
                        packet = response[j:j+8]
                        if len(packet) == 8 and packet[0] == 0xF5 and packet[7] == 0xF5:
                            cmd_byte = packet[1]
                            id_val = (packet[2] << 8) | packet[3]
                            status = packet[4]
                            step = packet[5]
                            
                            print(f"[{i+1:2d}s] {bytes_to_hex(packet)}")
                            print(f"      → CMD=0x{cmd_byte:02X}, ID={id_val}, Status=0x{status:02X}, Step={step}")
                            
                            if status == 0x00:
                                print(f"      ✅ THÀNH CÔNG! User ID = {id_val}")
                                last_status = "success"
                            elif status == 0x01:
                                print(f"      ⏳ Chờ chạm ngón tay (bước {step})")
                            elif status == 0x02:
                                print(f"      🔄 Đang xử lý (bước {step})")
                            elif status == 0x11:
                                print(f"      ❌ Lỗi: Không khớp/Đã tồn tại")
                                last_status = "error"
                            elif status == 0x12:
                                print(f"      ❌ Lỗi: Không đọc được vân tay")
                                last_status = "error"
        
        print("-" * 70)
        
        # Bước 4: Kiểm tra lại số người dùng
        print("\n" + "=" * 70)
        print("BƯỚC 4: Kiểm tra lại số người dùng")
        print("=" * 70)
        
        time.sleep(2)
        
        cmd = hex_to_bytes("F5090000000009F5")
        print(f"Gửi: {bytes_to_hex(cmd)}")
        ser.write(cmd)
        time.sleep(0.3)
        
        response = ser.read(8)
        print(f"Nhận: {bytes_to_hex(response)}")
        if len(response) >= 8:
            count = (response[2] << 8) | response[3]
            print(f"→ Số người dùng: {count}")
            
            if count > 0:
                print("\n✅✅✅ THÀNH CÔNG! Vân tay đã được lưu vào bộ nhớ!")
            else:
                print("\n⚠️⚠️⚠️ CẢNH BÁO: Số người dùng vẫn là 0!")
                print("\nThử nghiệm thêm: Gửi lệnh 14 (Lấy toàn bộ dữ liệu người dùng)")
                cmd = hex_to_bytes("F52B000000002BF5")
                print(f"Gửi: {bytes_to_hex(cmd)}")
                ser.write(cmd)
                time.sleep(0.5)
                
                if ser.in_waiting > 0:
                    response = ser.read(ser.in_waiting)
                    print(f"Nhận ({len(response)} bytes): {bytes_to_hex(response)}")
        
        ser.close()
        print("\n✅ Hoàn thành test!")
        
    except Exception as e:
        print(f"❌ Lỗi: {e}")

if __name__ == "__main__":
    main()
