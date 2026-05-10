import serial
import wave
import struct
import datetime
import sys
import os

# Cấu hình mặc định (có thể truyền qua argument khi chạy)
PORT = 'COM3' 
BAUD = 115200
SAMPLE_RATE = 16000

if len(sys.argv) > 1:
    PORT = sys.argv[1]

print(f"Connecting to {PORT} at {BAUD} baud...")
try:
    ser = serial.Serial(PORT, BAUD, timeout=1)
except Exception as e:
    print(f"Error opening serial port: {e}")
    print("Vui lòng tắt 'pio device monitor' hoặc Serial Monitor khác trước khi chạy script này!")
    sys.exit(1)

print("Listening for audio dumps... (Press Ctrl+C to stop)")

recording = False
hex_data = ""

# Tạo thư mục chứa file ghi âm nếu chưa có
if not os.path.exists("records"):
    os.makedirs("records")

try:
    while True:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
        except:
            continue
            
        if not line:
            continue
            
        if "---AUDIO_START" in line:
            # Format: ---AUDIO_START:label---
            label = "unknown"
            if ":" in line:
                label = line.split(":")[1].replace("---", "").strip()
                
            print(f"\n[+] ESP32 is sending audio data (Label: {label})...")
            recording = True
            hex_data = ""
            # Lưu biến label vào cấp global hoặc instance, dùng tạm cách này:
            ser.audio_label = label 
            continue
            
        if "---AUDIO_END---" in line and recording:
            print("[-] Finished receiving audio data. Saving to WAV...")
            recording = False
            
            # Giải mã chuỗi Hex về dạng bytes
            try:
                raw_bytes = bytes.fromhex(hex_data)
                
                # Ghi ra file WAV
                timestamp = datetime.datetime.now().strftime("%H%M%S")
                label = getattr(ser, 'audio_label', 'unknown')
                filename = f"records/audio_{label}_{timestamp}.wav"
                
                with wave.open(filename, 'wb') as wav_file:
                    wav_file.setnchannels(1) # Mono
                    wav_file.setsampwidth(2) # 16-bit (int16)
                    wav_file.setframerate(SAMPLE_RATE)
                    wav_file.writeframes(raw_bytes)
                    
                print(f"[OK] Saved to: {filename}")
                print("-" * 40)
            except Exception as e:
                print(f"[ERROR] Failed to save WAV: {e}")
            continue
            
        if recording:
            # Lọc chỉ lấy các ký tự Hex (bỏ qua nếu có nhiễu rác từ log khác xen vào)
            clean_line = ''.join(c for c in line if c in '0123456789ABCDEFabcdef')
            hex_data += clean_line
        else:
            # Nếu không đang thu âm, cứ in log bình thường từ ESP32 ra màn hình
            print(line)
            
except KeyboardInterrupt:
    print("\nExiting...")
    ser.close()
