#!/usr/bin/env python3
import os
import sys

PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD_DIR = os.path.join(PROJECT_DIR, ".pio", "build", "esp32dev")
TOOLS_DIR = os.path.join(PROJECT_DIR, "tools")
OUTPUT_FILE = os.path.join(PROJECT_DIR, "merged_firmware_0x00.bin")

FLASH_MAP = [
    (0x1000, os.path.join(BUILD_DIR, "bootloader.bin")),
    (0x8000, os.path.join(BUILD_DIR, "partitions.bin")),
    (0xe000, os.path.join(TOOLS_DIR, "boot_app0.bin")),
    (0x10000, os.path.join(BUILD_DIR, "firmware.bin")),
]

def merge():
    print("=" * 60)
    print("ESP32 FIRMWARE MERGE TOOL (Offset 0x00)")
    print("=" * 60)

    for offset, filepath in FLASH_MAP:
        if not os.path.isfile(filepath):
            print(f"[-] ERROR: Missing file: {filepath}")
            print("    Please run 'pio run' first to compile the project.")
            sys.exit(1)

    merged_data = bytearray()

    for offset, filepath in FLASH_MAP:
        with open(filepath, "rb") as f:
            data = f.read()
        
        current_len = len(merged_data)
        if current_len < offset:
            # Pad with 0xFF up to the target offset
            merged_data.extend(b"\xff" * (offset - current_len))
        elif current_len > offset:
            print(f"[-] ERROR: Overlapping partitions at offset {hex(offset)} (current length: {hex(current_len)})")
            sys.exit(1)

        print(f"[+] Added {os.path.basename(filepath):16} at offset {hex(offset)} ({len(data):,} bytes)")
        merged_data.extend(data)

    with open(OUTPUT_FILE, "wb") as f:
        f.write(merged_data)

    print("-" * 60)
    print(f"[OK] Merged firmware successfully created:")
    print(f"     File: {OUTPUT_FILE}")
    print(f"     Size: {len(merged_data):,} bytes ({len(merged_data)/1024/1024:.2f} MB)")
    print(f"     Ready to flash at address: 0x00000000 (0x00)")
    print("=" * 60)

if __name__ == "__main__":
    merge()
