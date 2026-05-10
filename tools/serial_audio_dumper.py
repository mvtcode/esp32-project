import serial
import sys
import os
import time
import wave
import threading

# Cấu hình
BAUD_RATE = 921600
OUTPUT_DIR = "records"

if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

def save_wav(filename, data):
    with wave.open(filename, 'wb') as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2) # 16-bit
        wf.setframerate(16000)
        wf.writeframes(data)

def input_thread(ser):
    """Luồng lắng nghe phím Enter từ bàn phím để gửi lệnh dừng"""
    try:
        while ser.is_open:
            input() # Đợi nhấn Enter
            ser.write(b'\n')
            print("\n[!] Sent STOP command to ESP32")
    except EOFError:
        pass # Chế độ non-interactive

def run_dumper(port):
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=0.1)
        print(f"Connecting to {port} at {BAUD_RATE} baud...")
        print("Listening for AUDIO_START... (Press Enter to stop recording manually, Ctrl+C to quit)")

        # Chạy luồng gửi lệnh
        threading.Thread(target=input_thread, args=(ser,), daemon=True).start()

        recording = False
        audio_data = bytearray()
        filename = ""

        while True:
            line = ser.readline().decode('ascii', errors='ignore').strip()
            if not line:
                continue

            if "---AUDIO_START:" in line:
                label = line.split(":")[1].replace("---", "")
                timestamp = int(time.time())
                filename = os.path.join(OUTPUT_DIR, f"test_{label}_{timestamp}.wav")
                audio_data = bytearray()
                recording = True
                print(f"\n[+] Recording: {filename}")
            
            elif "---AUDIO_END---" in line:
                if recording:
                    save_wav(filename, audio_data)
                    print(f"\n[-] Saved.")
                    recording = False
            
            elif recording:
                try:
                    # Lọc Hex và ghi
                    hex_only = "".join(c for c in line if c in "0123456789ABCDEFabcdef")
                    if len(hex_only) >= 4:
                        raw_bytes = bytes.fromhex(hex_only)
                        audio_data.extend(raw_bytes)
                        sys.stdout.write(".")
                        sys.stdout.flush()
                except Exception as e:
                    pass
            else:
                print(line)

    except KeyboardInterrupt:
        print("\nExit.")
    except Exception as e:
        print(f"\nError: {e}")
    finally:
        if 'ser' in locals():
            ser.close()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python serial_audio_dumper.py <COM_PORT>")
    else:
        run_dumper(sys.argv[1])
