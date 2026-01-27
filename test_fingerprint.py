#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Script test module vân tay TZM1026 (Biosec TA0702)
Kết nối qua cổng COM để test các lệnh trước khi tích hợp vào GUI
"""

import serial
import time
import sys

# Cấu hình cổng COM
PORT = "COM8"
BAUDRATE = 115200
TIMEOUT = 2

# Danh sách lệnh HEX đầy đủ (21 lệnh) - ĐÃ SỬA LỖI CHECKSUM
COMMANDS = {
    0: {"hex": "F5090000000009F5", "desc": "Lấy số lượng người dùng"},
    1: {"hex": "F5600000000060F5", "desc": "Lấy ID (mã định danh) duy nhất"},
    2: {"hex": "F52D000001002CF5", "desc": "Thiết lập mức độ so khớp (độ nghiêm ngặt)"},
    3: {"hex": "F526000000000026F5", "desc": "Thiết lập chế độ đăng ký vân tay"},
    4: {"hex": "F528000200002AF5", "desc": "Thiết lập cấp phân quyền người dùng"},
    5: {"hex": "F505000000000005F5", "desc": "Xóa toàn bộ dữ liệu người dùng"},
    6: {"hex": "F52D000000002DF5", "desc": "Cho phép hoặc cấm đăng ký người dùng mới"},
    7: {"hex": "F5280002000028F5", "desc": "Thiết lập lại mức độ so khớp"},
    8: {"hex": "F53F000000003FF5", "desc": "Thiết lập chế độ đăng ký"},
    9: {"hex": "F53F000100003EF5", "desc": "Thiết lập mức xác thực cùng cấp"},
    10: {"hex": "F53F000200003DF5", "desc": "Thiết lập mức xác thực cùng cấp (mức 2)"},
    11: {"hex": "F53F000300003CF5", "desc": "Thiết lập tần suất thu thập vân tay"},
    12: {"hex": "F53F00040001003AF5", "desc": "Thiết lập cấp độ cho người dùng chỉ định"},
    13: {"hex": "F53F000500003AF5", "desc": "Thiết lập chức năng điều khiển bằng nút bấm"},
    14: {"hex": "F52B000000002BF5", "desc": "Lấy toàn bộ dữ liệu người dùng"},
    15: {"hex": "F5010001010101F5", "desc": "Đăng ký bằng nhiều lần nhấn (chế độ 3CNR)"},
    16: {"hex": "F5010000010000F5", "desc": "Đăng ký bằng nhiều lần nhấn (chế độ NCNR)"},
    17: {"hex": "F50B0001000000AF5", "desc": "So khớp vân tay 1:1 (verify với ID cụ thể)"},
    18: {"hex": "F50C000000000CF5", "desc": "So khớp vân tay 1:N (tìm trong toàn bộ database)"},
    19: {"hex": "F524000000000024F5", "desc": "Lấy ảnh vân tay (fingerprint image)"},
    20: {"hex": "F5B8000000000B8F5", "desc": "Hủy / ngắt thao tác hiện tại"},
}

# Mã trạng thái phản hồi
STATUS_CODES = {
    0x00: "Thành công",
    0x01: "Đang chờ chạm vân tay",
    0x02: "Đang xử lý ảnh vân tay",
    0x11: "Lỗi: Không khớp / Đã tồn tại",
    0x12: "Lỗi: Không đọc được (chất lượng kém)",
    0x13: "Lỗi: Timeout (hết thời gian chờ)",
    0x14: "Lỗi: Đầy bộ nhớ",
}


def hex_to_bytes(hex_string):
    """Chuyển chuỗi HEX thành bytes"""
    return bytes.fromhex(hex_string)


def bytes_to_hex(data):
    """Chuyển bytes thành chuỗi HEX để hiển thị"""
    return ' '.join([f'{b:02X}' for b in data])


def parse_response(response):
    """Phân tích và giải mã phản hồi từ module"""
    if len(response) < 8:
        return "⚠️  Phản hồi không đầy đủ (< 8 bytes)"
    
    if response[0] != 0xF5 or response[-1] != 0xF5:
        return "⚠️  Gói tin không hợp lệ (thiếu header/footer 0xF5)"
    
    cmd = response[1]
    data1 = response[2]
    data2 = response[3]
    data3 = response[4]
    data4 = response[5]
    checksum = response[6]
    
    # Kiểm tra checksum
    calculated_sum = cmd ^ data1 ^ data2 ^ data3 ^ data4
    if calculated_sum != checksum:
        return f"⚠️  Checksum không khớp! Tính được: {calculated_sum:02X}, Nhận được: {checksum:02X}"
    
    result = []
    
    # Giải mã theo từng loại lệnh
    if cmd == 0x09:  # Lấy số người dùng
        user_count = (data1 << 8) | data2
        result.append(f"✅ Số lượng người dùng: {user_count}")
    
    elif cmd == 0x0C:  # Xác thực 1:N
        user_id = (data1 << 8) | data2
        status = data3
        if status == 0x00 and user_id > 0:
            result.append(f"✅ XÁC THỰC THÀNH CÔNG! User ID: {user_id}")
        elif status == 0x11:
            result.append("❌ KHÔNG KHỚP! Vân tay không tìm thấy.")
        elif status == 0x01:
            result.append("⏳ Đang chờ chạm vân tay...")
        else:
            status_msg = STATUS_CODES.get(status, f"Mã lỗi: 0x{status:02X}")
            result.append(f"ℹ️  Trạng thái: {status_msg}")
    
    elif cmd == 0x01:  # Đăng ký vân tay
        status = data3
        step = data4
        if status == 0x00:
            result.append(f"✅ ĐĂNG KÝ THÀNH CÔNG!")
        elif status == 0x01:
            result.append(f"⏳ Đang chờ chạm ngón tay... (Bước {step}/3)")
        elif status == 0x02:
            result.append(f"🔄 Đang xử lý ảnh vân tay... (Bước {step}/3)")
        else:
            status_msg = STATUS_CODES.get(status, f"Mã lỗi: 0x{status:02X}")
            result.append(f"ℹ️  Trạng thái: {status_msg}, Bước: {step}")
    
    elif cmd == 0x0B:  # Xác thực 1:1
        status = data3
        if status == 0x00:
            result.append("✅ XÁC THỰC 1:1 THÀNH CÔNG!")
        elif status == 0x11:
            result.append("❌ KHÔNG KHỚP! Vân tay không đúng với ID đã chỉ định.")
        else:
            status_msg = STATUS_CODES.get(status, f"Mã lỗi: 0x{status:02X}")
            result.append(f"ℹ️  Trạng thái: {status_msg}")
    
    else:
        # Các lệnh khác - hiển thị trạng thái chung
        if data3 in STATUS_CODES:
            result.append(f"ℹ️  Trạng thái: {STATUS_CODES[data3]}")
        else:
            result.append(f"ℹ️  Dữ liệu: D1={data1:02X} D2={data2:02X} D3={data3:02X} D4={data4:02X}")
    
    return '\n'.join(result)


def send_command(ser, cmd_num):
    """Gửi lệnh đến module và nhận phản hồi"""
    if cmd_num not in COMMANDS:
        print(f"❌ Lệnh {cmd_num} không tồn tại!")
        return
    
    cmd_info = COMMANDS[cmd_num]
    print(f"\n{'='*60}")
    print(f"📤 Gửi lệnh {cmd_num}: {cmd_info['desc']}")
    print(f"{'='*60}")
    
    # Chuyển HEX string thành bytes
    cmd_bytes = hex_to_bytes(cmd_info['hex'])
    print(f"HEX gửi đi: {bytes_to_hex(cmd_bytes)}")
    
    # Gửi lệnh
    ser.write(cmd_bytes)
    time.sleep(0.1)
    
    # Đọc phản hồi
    response = ser.read(8)
    
    if len(response) > 0:
        print(f"\n📥 Phản hồi HEX: {bytes_to_hex(response)}")
        print(f"\n{parse_response(response)}")
    else:
        print("⚠️  Không nhận được phản hồi từ module!")
    
    print(f"{'='*60}\n")


def show_menu():
    """Hiển thị menu lệnh"""
    print("\n" + "="*60)
    print("🔐 TEST MODULE VÂN TAY TZM1026 - MENU LỆNH")
    print("="*60)
    print("\n📋 DANH SÁCH LỆNH:")
    print("-"*60)
    for num, info in sorted(COMMANDS.items()):
        print(f"  [{num:2d}] {info['desc']}")
    print("-"*60)
    print("\n💡 HƯỚNG DẪN:")
    print("  - Nhập số lệnh (0-20) để thực thi")
    print("  - Nhập 'menu' để hiển thị lại menu")
    print("  - Nhập 'exit' hoặc 'quit' để thoát")
    print("="*60 + "\n")


def main():
    """Hàm chính"""
    print("\n🚀 Khởi động script test module vân tay TZM1026...")
    print(f"📡 Đang kết nối với {PORT} @ {BAUDRATE} baud...\n")
    
    try:
        # Mở kết nối serial
        ser = serial.Serial(
            port=PORT,
            baudrate=BAUDRATE,
            timeout=TIMEOUT,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE
        )
        
        print(f"✅ Kết nối thành công với {PORT}!")
        time.sleep(0.5)
        
        # Xóa buffer
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        # Hiển thị menu
        show_menu()
        
        # Vòng lặp chính
        while True:
            try:
                user_input = input("👉 Nhập lệnh: ").strip().lower()
                
                if user_input in ['exit', 'quit', 'q']:
                    print("\n👋 Đang đóng kết nối...")
                    break
                
                if user_input == 'menu':
                    show_menu()
                    continue
                
                # Kiểm tra nếu là số
                try:
                    cmd_num = int(user_input)
                    send_command(ser, cmd_num)
                except ValueError:
                    print("❌ Vui lòng nhập số từ 0-20, 'menu', hoặc 'exit'")
            
            except KeyboardInterrupt:
                print("\n\n⚠️  Nhận Ctrl+C, đang thoát...")
                break
        
        # Đóng kết nối
        ser.close()
        print("✅ Đã đóng kết nối. Tạm biệt!\n")
    
    except serial.SerialException as e:
        print(f"❌ Lỗi kết nối serial: {e}")
        print(f"\n💡 Kiểm tra lại:")
        print(f"   - Cổng {PORT} có đúng không?")
        print(f"   - Module đã được kết nối chưa?")
        print(f"   - Driver USB-Serial đã cài đặt chưa?")
        sys.exit(1)
    
    except Exception as e:
        print(f"❌ Lỗi không xác định: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
