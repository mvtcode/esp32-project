#!/usr/bin/env python3
"""
flash_srmodels.py — PlatformIO extra_script (post:)
Flashes srmodels.bin (ESP-SR model data) to the 'model' partition after upload.

Usage in platformio.ini:
    extra_scripts = post:scripts/flash_srmodels.py

How to get srmodels.bin:
    1. Download from esp-sr releases:
       https://github.com/espressif/esp-sr/releases
    2. Or build from ESP-Skainet examples via ESP-IDF
    3. Place the file at: models/srmodels.bin
"""

Import("env")
import os
import subprocess

def flash_sr_models(source, target, env):
    models_bin = os.path.join(env.subst("$PROJECT_DIR"), "models", "srmodels.bin")

    if not os.path.isfile(models_bin):
        print(f"\n[flash_srmodels] WARNING: models/srmodels.bin not found.")
        print(f"[flash_srmodels] Skipping model flash. Download srmodels.bin and place in models/ folder.")
        return

    # Get upload port
    upload_port = env.GetProjectOption("upload_port", default=None)
    if not upload_port:
        # Try auto-detect
        upload_port = env.subst("$UPLOAD_PORT")

    # Get partition offset for 'model' — matches partitions.csv offset 0x310000
    model_offset = "0x310000"

    esptool = env.subst("$PYTHONEXE") + " -m esptool"
    cmd = (
        f'{esptool} --chip esp32s3 --port {upload_port} '
        f'--baud 921600 write_flash {model_offset} "{models_bin}"'
    )

    print(f"\n[flash_srmodels] Flashing srmodels.bin to partition 'model' @ {model_offset}")
    print(f"[flash_srmodels] Command: {cmd}\n")

    ret = subprocess.call(cmd, shell=True)
    if ret != 0:
        print(f"\n[flash_srmodels] ERROR: Failed to flash srmodels.bin (exit code {ret})")
    else:
        print(f"\n[flash_srmodels] srmodels.bin flashed successfully!")

env.AddPostAction("upload", flash_sr_models)
