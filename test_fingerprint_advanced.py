#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Script test nâng cao cho module vân tay TZM1026
Bao gồm các chức năng: auto-test, logging, và custom commands
"""

import serial
import time
import sys
from datetime import datetime

PORT = "COM8"
BAUDRATE = 115200
TIMEOUT = 2

# Màu sắc cho terminal (Windows)
class Colors:
    RESET = '\033[0m'
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'


def log_message(message, color=Colors.RESET):
    """In thông báo có màu với timestamp"""
    timestamp = datetime.now().strftime("%H:%M:%S")
    print(f"{color}[{timestamp}] {message}{Colors.RESET}")


def hex_to_bytes(hex_string):
    """Chuyển chuỗi HEX thành bytes"""
    return bytes.fromhex(hex_string)


def bytes_to_hex(data):
    """Chuyển bytes thành chuỗi HEX"""
    return ' '.join([f'{b:02X}' for b in data])


def calculate_checksum(cmd, d1, d2, d3, d4):
    """Tính checksum cho gói tin F5"""
    return cmd ^ d1 ^ d2 ^ d3 ^ d4


def create_packet(cmd, d1=0x00, d2=0x00, d3=0x00, d4=0x00):
    """Tạo gói tin F5 protocol"""
    checksum = calculate_checksum(cmd, d1, d2, d3, d4)
    packet = bytes([0xF5, cmd, d1, d2, d3, d4, checksum, 0xF5])
    return packet


def send_custom_command(ser, cmd, d1=0x00, d2=0x00, d3=0x00, d4=0x00, description=""):
    """Gửi lệnh tùy chỉnh"""
    packet = create_packet(cmd, d1, d2, d3, d4)
    
    log_message(f"Gửi lệnh tùy chỉnh: {description}", Colors.CYAN)
    log_message(f"HEX: {bytes_to_hex(packet)}", Colors.BLUE)
    
    ser.write(packet)
    time.sleep(0.2)
    
    response = ser.read(8)
    if len(response) > 0:
        log_message(f"Phản hồi: {bytes_to_hex(response)}", Colors.GREEN)
        return response
    else:
        log_message("Không nhận được phản hồi!", Colors.RED)
        return None


def get_user_count(ser):
    """Lấy số lượng người dùng"""
    packet = hex_to_bytes("F5090000000009F5")
    ser.write(packet)
    time.sleep(0.2)
    
    response = ser.read(8)
    if len(response) >= 8:
        count = (response[2] << 8) | response[3]
        return count
    return -1


def get_next_available_id(ser):
    """Tìm ID trống tiếp theo (ID hợp lệ: 1-200)"""
    count = get_user_count(ser)
    if count < 0:
        log_message("⚠️  Không thể lấy số người dùng, dùng ID = 1", Colors.YELLOW)
        return 1
    
    # ID bắt đầu từ 1, không phải 0
    next_id = count + 1
    if next_id > 200:
        log_message("❌ Database đã đầy (max 200 người dùng)!", Colors.RED)
        return None
    
    return next_id


def enroll_fingerprint(ser, user_id=None):
    """Đăng ký vân tay với ID cụ thể hoặc tự động"""
    log_message("=== BẮT ĐẦU QUÁ TRÌNH ĐĂNG KÝ VÂN TAY ===", Colors.MAGENTA)
    
    if user_id is None:
        # Tự động tìm ID trống tiếp theo
        user_id = get_next_available_id(ser)
        if user_id is None:
            return False
        log_message(f"Chế độ: Tự động gán ID = {user_id}", Colors.CYAN)
    else:
        log_message(f"Chế độ: Đăng ký với ID = {user_id}", Colors.CYAN)
    
    # Luôn dùng chế độ 3CNR với ID cụ thể (DATA3 = 0x01, DATA4 = 0x01)
    id_msb = (user_id >> 8) & 0xFF
    id_lsb = user_id & 0xFF
    packet = create_packet(0x01, id_msb, id_lsb, 0x01, 0x01)
    
    log_message(f"📤 Gửi lệnh: {bytes_to_hex(packet)}", Colors.BLUE)
    ser.write(packet)
    
    # Vòng lặp chờ hoàn thành đăng ký
    max_attempts = 30  # 30 giây timeout
    enrolled_successfully = False
    final_user_id = None
    
    for attempt in range(max_attempts):
        time.sleep(1)
        
        if ser.in_waiting > 0:
            response = ser.read(8)
            log_message(f"📥 Phản hồi RAW: {bytes_to_hex(response)}", Colors.BLUE)
            
            if len(response) >= 8:
                cmd = response[1]
                resp_id_msb = response[2]
                resp_id_lsb = response[3]
                status = response[4]
                current_step = response[5]
                
                resp_id = (resp_id_msb << 8) | resp_id_lsb
                
                log_message(f"   CMD=0x{cmd:02X}, ID={resp_id}, Status=0x{status:02X}, Step={current_step}", Colors.CYAN)
                
                if status == 0x00:
                    # Kiểm tra xem có phải là phản hồi đăng ký thành công không
                    if cmd == 0x01:
                        log_message(f"✅ ĐĂNG KÝ THÀNH CÔNG! User ID: {resp_id}", Colors.GREEN)
                        enrolled_successfully = True
                        final_user_id = resp_id
                        # Đợi thêm 1 giây để module lưu dữ liệu
                        time.sleep(1)
                        break
                    else:
                        log_message(f"⚠️  Nhận được phản hồi thành công nhưng CMD=0x{cmd:02X} (không phải 0x01)", Colors.YELLOW)
                elif status == 0x01:
                    log_message(f"⏳ Đang chờ chạm ngón tay... (Bước {current_step}/3)", Colors.YELLOW)
                elif status == 0x02:
                    log_message(f"🔄 Đang xử lý ảnh vân tay... (Bước {current_step}/3)", Colors.CYAN)
                elif status == 0x11:
                    log_message("❌ Lỗi: Vân tay đã tồn tại hoặc không khớp!", Colors.RED)
                    return False
                elif status == 0x12:
                    log_message("❌ Lỗi: Không đọc được vân tay (chất lượng kém)", Colors.RED)
                    return False
                else:
                    log_message(f"⚠️  Trạng thái không xác định: 0x{status:02X}", Colors.YELLOW)
    
    if enrolled_successfully:
        # Verify bằng cách kiểm tra số người dùng
        log_message("🔍 Đang verify...", Colors.CYAN)
        time.sleep(0.5)
        new_count = get_user_count(ser)
        log_message(f"📊 Số người dùng hiện tại: {new_count}", Colors.CYAN)
        
        if new_count > 0:
            log_message(f"✅ Xác nhận: Vân tay đã được lưu vào bộ nhớ!", Colors.GREEN)
        else:
            log_message(f"⚠️  Cảnh báo: Số người dùng vẫn là 0, có thể chưa lưu vào flash!", Colors.YELLOW)
        
        return True
    else:
        log_message("⏱️  Timeout: Hết thời gian chờ!", Colors.RED)
        return False


def verify_fingerprint(ser, user_id=None):
    """Xác thực vân tay (1:1 hoặc 1:N)"""
    if user_id is None:
        # Xác thực 1:N (tìm trong toàn bộ database)
        log_message("=== XÁC THỰC 1:N (TÌM TRONG DATABASE) ===", Colors.MAGENTA)
        packet = hex_to_bytes("F50C000000000CF5")
    else:
        # Xác thực 1:1 (verify với ID cụ thể)
        log_message(f"=== XÁC THỰC 1:1 (ID = {user_id}) ===", Colors.MAGENTA)
        id_msb = (user_id >> 8) & 0xFF
        id_lsb = user_id & 0xFF
        packet = create_packet(0x0B, id_msb, id_lsb, 0x00, 0x00)
    
    ser.write(packet)
    log_message("⏳ Vui lòng chạm ngón tay...", Colors.YELLOW)
    
    max_attempts = 10
    for attempt in range(max_attempts):
        time.sleep(1)
        
        if ser.in_waiting > 0:
            response = ser.read(8)
            
            if len(response) >= 8:
                status = response[4]
                matched_id = (response[2] << 8) | response[3]
                
                if status == 0x00 and matched_id > 0:
                    log_message(f"✅ XÁC THỰC THÀNH CÔNG! User ID: {matched_id}", Colors.GREEN)
                    return matched_id
                elif status == 0x11:
                    log_message("❌ KHÔNG KHỚP! Vân tay không tìm thấy.", Colors.RED)
                    return None
                elif status == 0x01:
                    log_message("⏳ Đang chờ chạm vân tay...", Colors.YELLOW)
    
    log_message("⏱️  Timeout: Hết thời gian chờ!", Colors.RED)
    return None


def delete_all_users(ser):
    """Xóa toàn bộ dữ liệu người dùng"""
    log_message("⚠️  CẢNH BÁO: Sắp xóa toàn bộ dữ liệu!", Colors.RED)
    confirm = input("Nhập 'YES' để xác nhận: ")
    
    if confirm != "YES":
        log_message("Đã hủy thao tác xóa.", Colors.YELLOW)
        return False
    
    packet = hex_to_bytes("F505000000000005F5")
    ser.write(packet)
    time.sleep(0.5)
    
    response = ser.read(8)
    if len(response) >= 8:
        log_message("✅ Đã xóa toàn bộ dữ liệu!", Colors.GREEN)
        return True
    
    log_message("❌ Lỗi khi xóa dữ liệu!", Colors.RED)
    return False


def auto_test(ser):
    """Chạy test tự động"""
    log_message("\n" + "="*60, Colors.CYAN)
    log_message("🤖 BẮT ĐẦU AUTO TEST", Colors.CYAN)
    log_message("="*60 + "\n", Colors.CYAN)
    
    # Test 1: Kiểm tra kết nối
    log_message("Test 1: Kiểm tra kết nối và số người dùng", Colors.YELLOW)
    count = get_user_count(ser)
    if count >= 0:
        log_message(f"✅ Kết nối OK. Số người dùng hiện tại: {count}", Colors.GREEN)
    else:
        log_message("❌ Không kết nối được với module!", Colors.RED)
        return
    
    time.sleep(2)
    
    # Test 2: Đăng ký vân tay
    log_message("\nTest 2: Đăng ký vân tay mới", Colors.YELLOW)
    log_message("Vui lòng chuẩn bị ngón tay để đăng ký...", Colors.CYAN)
    time.sleep(3)
    
    success = enroll_fingerprint(ser)
    if not success:
        log_message("❌ Đăng ký thất bại! Dừng auto test.", Colors.RED)
        return
    
    time.sleep(2)
    
    # Test 3: Kiểm tra lại số người dùng
    log_message("\nTest 3: Kiểm tra lại số người dùng sau khi đăng ký", Colors.YELLOW)
    new_count = get_user_count(ser)
    if new_count == count + 1:
        log_message(f"✅ Số người dùng tăng lên: {count} → {new_count}", Colors.GREEN)
    else:
        log_message(f"⚠️  Số người dùng: {new_count} (kỳ vọng: {count + 1})", Colors.YELLOW)
    
    time.sleep(2)
    
    # Test 4: Xác thực vân tay vừa đăng ký
    log_message("\nTest 4: Xác thực vân tay vừa đăng ký", Colors.YELLOW)
    log_message("Vui lòng chạm lại ngón tay vừa đăng ký...", Colors.CYAN)
    time.sleep(2)
    
    matched_id = verify_fingerprint(ser)
    if matched_id:
        log_message(f"✅ Xác thực thành công với ID: {matched_id}", Colors.GREEN)
    else:
        log_message("❌ Xác thực thất bại!", Colors.RED)
    
    log_message("\n" + "="*60, Colors.CYAN)
    log_message("🏁 KẾT THÚC AUTO TEST", Colors.CYAN)
    log_message("="*60 + "\n", Colors.CYAN)


def interactive_mode(ser):
    """Chế độ tương tác nâng cao"""
    while True:
        print("\n" + "="*60)
        print("🔐 MENU CHỨC NĂNG NÂNG CAO")
        print("="*60)
        print("\n📋 CHỨC NĂNG:")
        print("  [1] Kiểm tra số người dùng")
        print("  [2] Đăng ký vân tay (ID tự động)")
        print("  [3] Đăng ký vân tay (chỉ định ID)")
        print("  [4] Xác thực 1:N (tìm trong database)")
        print("  [5] Xác thực 1:1 (verify ID cụ thể)")
        print("  [6] Xóa toàn bộ dữ liệu")
        print("  [7] Gửi lệnh tùy chỉnh")
        print("  [8] Chạy auto test")
        print("  [0] Thoát")
        print("="*60)
        
        choice = input("\n👉 Chọn chức năng: ").strip()
        
        if choice == '0':
            break
        elif choice == '1':
            count = get_user_count(ser)
            log_message(f"Số người dùng: {count}", Colors.GREEN)
        elif choice == '2':
            enroll_fingerprint(ser)
        elif choice == '3':
            user_id = int(input("Nhập User ID (1-200): "))
            enroll_fingerprint(ser, user_id)
        elif choice == '4':
            verify_fingerprint(ser)
        elif choice == '5':
            user_id = int(input("Nhập User ID cần verify: "))
            verify_fingerprint(ser, user_id)
        elif choice == '6':
            delete_all_users(ser)
        elif choice == '7':
            print("\nNhập lệnh tùy chỉnh (HEX):")
            cmd = int(input("CMD (hex, vd: 0x09): "), 16)
            d1 = int(input("DATA1 (hex, vd: 0x00): "), 16)
            d2 = int(input("DATA2 (hex, vd: 0x00): "), 16)
            d3 = int(input("DATA3 (hex, vd: 0x00): "), 16)
            d4 = int(input("DATA4 (hex, vd: 0x00): "), 16)
            desc = input("Mô tả lệnh: ")
            send_custom_command(ser, cmd, d1, d2, d3, d4, desc)
        elif choice == '8':
            auto_test(ser)
        else:
            log_message("Lựa chọn không hợp lệ!", Colors.RED)


def main():
    """Hàm chính"""
    print("\n🚀 Script Test Nâng cao - Module Vân tay TZM1026")
    print(f"📡 Đang kết nối với {PORT} @ {BAUDRATE} baud...\n")
    
    try:
        ser = serial.Serial(
            port=PORT,
            baudrate=BAUDRATE,
            timeout=TIMEOUT,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE
        )
        
        log_message(f"Kết nối thành công với {PORT}!", Colors.GREEN)
        time.sleep(0.5)
        
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        # Chạy chế độ tương tác
        interactive_mode(ser)
        
        ser.close()
        log_message("Đã đóng kết nối. Tạm biệt!", Colors.CYAN)
    
    except serial.SerialException as e:
        log_message(f"Lỗi kết nối serial: {e}", Colors.RED)
        sys.exit(1)
    except Exception as e:
        log_message(f"Lỗi không xác định: {e}", Colors.RED)
        sys.exit(1)


if __name__ == "__main__":
    main()
