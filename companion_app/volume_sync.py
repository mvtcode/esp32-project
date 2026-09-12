#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ESP32-S3 Smart TouchPad - Bi-directional Volume Sync Companion App
Đồng bộ âm lượng 2 chiều giữa Windows Core Audio và ESP32-S3 qua BLE Custom GATT Service.
"""

import sys
import time
import asyncio
import platform
from typing import Optional

# Thư viện BLE Client
from bleak import BleakClient, BleakScanner
from bleak.backends.characteristic import BleakGATTCharacteristic

# UUID của Custom Volume Service trên ESP32
VOLUME_SERVICE_UUID = "0000ffe0-0000-1000-8000-00805f9b34fb"
VOLUME_CHAR_UUID    = "0000ffe1-0000-1000-8000-00805f9b34fb"
TARGET_DEVICE_NAME  = "ESP32-S3 Smart TouchPad"

# Cấu hình âm thanh hệ thống Windows
try:
    from comtypes import CLSCTX_ALL, COMObject
    from pycaw.pycaw import AudioUtilities, IAudioEndpointVolume, IAudioEndpointVolumeCallback
    WINDOWS_AUDIO_AVAILABLE = True
except ImportError:
    WINDOWS_AUDIO_AVAILABLE = False


class WindowsVolumeController:
    """Quản lý đọc/ghi và lắng nghe thay đổi âm lượng trên Windows Core Audio"""
    def __init__(self, on_volume_changed_callback):
        self.on_volume_changed_callback = on_volume_changed_callback
        self.volume_interface = None
        self.callback_obj = None
        self._init_audio()

    def _init_audio(self):
        if not WINDOWS_AUDIO_AVAILABLE:
            print("[WARN] Thư viện pycaw/comtypes chưa sẵn sàng. Tính năng đọc âm thanh PC bị vô hiệu hóa.")
            return

        try:
            devices = AudioUtilities.GetSpeakers()
            interface = devices.Activate(IAudioEndpointVolume._iid_, CLSCTX_ALL, None)
            self.volume_interface = interface.QueryInterface(IAudioEndpointVolume)

            # Lớp callback lắng nghe sự kiện từ Windows
            class AudioCallback(COMObject):
                _com_interfaces_ = [IAudioEndpointVolumeCallback]

                def __init__(outer_self, parent):
                    super().__init__()
                    outer_self.parent = parent

                def OnNotify(outer_self, pNotify):
                    data = pNotify.contents
                    vol_scalar = data.fMasterVolume
                    vol_percent = int(round(vol_scalar * 100))
                    if outer_self.parent.on_volume_changed_callback:
                        outer_self.parent.on_volume_changed_callback(vol_percent)

            self.callback_obj = AudioCallback(self)
            self.volume_interface.RegisterControlChangeNotify(self.callback_obj)
            print("[INFO] Windows Core Audio Endpoint đã kết nối thành công!")
        except Exception as e:
            print(f"[ERROR] Không thể khởi tạo Windows Audio: {e}")

    def get_volume(self) -> int:
        if self.volume_interface:
            try:
                scalar = self.volume_interface.GetMasterVolumeLevelScalar()
                return int(round(scalar * 100))
            except Exception:
                pass
        return 50

    def set_volume(self, percent: int):
        if self.volume_interface:
            try:
                scalar = max(0.0, min(1.0, percent / 100.0))
                self.volume_interface.SetMasterVolumeLevelScalar(scalar, None)
            except Exception as e:
                print(f"[ERROR] Lỗi đặt âm lượng Windows: {e}")

    def cleanup(self):
        if self.volume_interface and self.callback_obj:
            try:
                self.volume_interface.UnregisterControlChangeNotify(self.callback_obj)
            except Exception:
                pass


class VolumeSyncApp:
    def __init__(self):
        self.client: Optional[BleakClient] = None
        self.loop = asyncio.get_event_loop()
        self.audio_ctrl = WindowsVolumeController(self._on_windows_volume_changed)
        self.last_synced_vol = -1
        self.is_updating_from_esp32 = False
        self.is_updating_from_pc = False

    def _on_windows_volume_changed(self, vol_percent: int):
        """Được gọi từ Windows khi âm lượng máy tính thay đổi"""
        if self.is_updating_from_esp32:
            return

        if abs(vol_percent - self.last_synced_vol) >= 1:
            self.last_synced_vol = vol_percent
            print(f"[PC -> ESP32] Âm lượng máy tính thay đổi: {vol_percent}%")
            # Gửi giá trị qua BLE
            asyncio.run_coroutine_threadsafe(self.send_volume_to_esp32(vol_percent), self.loop)

    async def send_volume_to_esp32(self, vol_percent: int):
        """Ghi giá trị âm lượng vào Characteristic FFE1 của ESP32"""
        if self.client and self.client.is_connected:
            try:
                self.is_updating_from_pc = True
                data = bytearray([vol_percent])
                await self.client.write_gatt_char(VOLUME_CHAR_UUID, data, response=False)
            except Exception as e:
                print(f"[WARN] Không thể gửi âm lượng sang ESP32: {e}")
            finally:
                await asyncio.sleep(0.05)
                self.is_updating_from_pc = False

    def _on_esp32_notify(self, sender: BleakGATTCharacteristic, data: bytearray):
        """Được gọi khi ESP32 gửi thông báo Notify (kéo slider trên màn hình)"""
        if self.is_updating_from_pc:
            return

        if len(data) > 0:
            vol = int(data[0])
            self.last_synced_vol = vol
            print(f"[ESP32 -> PC] Nhận từ ESP32 Slider: {vol}%")
            self.is_updating_from_esp32 = True
            try:
                self.audio_ctrl.set_volume(vol)
            finally:
                self.loop.call_later(0.1, self._clear_esp32_flag)

    def _clear_esp32_flag(self):
        self.is_updating_from_esp32 = False

    async def run(self):
        print("=" * 60)
        print("  ESP32-S3 Realtime Bi-directional Volume Sync Companion")
        print("=" * 60)

        while True:
            print("\n[SCAN] Đang tìm kiếm thiết bị ESP32-S3 BLE...")
            device = None

            try:
                devices = await BleakScanner.discover(timeout=5.0)
                for d in devices:
                    if d.name and TARGET_DEVICE_NAME.lower() in d.name.lower():
                        device = d
                        break

                if not device:
                    print(f"[INFO] Không tìm thấy '{TARGET_DEVICE_NAME}'. Thử lại sau 3 giây...")
                    await asyncio.sleep(3.0)
                    continue

                print(f"[FOUND] Đã tìm thấy ESP32: {device.name} [{device.address}]")
                print("[CONNECT] Đang kết nối BLE GATT...")

                async with BleakClient(device.address) as client:
                    self.client = client
                    print("[CONNECTED] Đã kết nối thành công tới ESP32!")

                    # Đồng bộ âm lượng ban đầu từ PC sang ESP32
                    initial_vol = self.audio_ctrl.get_volume()
                    print(f"[SYNC] Đồng bộ âm lượng ban đầu từ PC: {initial_vol}%")
                    await self.send_volume_to_esp32(initial_vol)

                    # Lắng nghe Notify từ Characteristic FFE1
                    await client.start_notify(VOLUME_CHAR_UUID, self._on_esp32_notify)
                    print("[READY] Đang lắng nghe đồng bộ 2 chiều thời gian thực! (Nhấn Ctrl+C để thoát)")

                    # Giữ kết nối
                    while client.is_connected:
                        await asyncio.sleep(1.0)

                print("[DISCONNECT] Mất kết nối tới ESP32. Đang chuẩn bị quét lại...")

            except asyncio.CancelledError:
                break
            except Exception as e:
                print(f"[ERROR] Lỗi trong phiên kết nối BLE: {e}")
                await asyncio.sleep(3.0)


def main():
    app = VolumeSyncApp()
    try:
        asyncio.run(app.run())
    except KeyboardInterrupt:
        print("\n[EXIT] Đang đóng ứng dụng...")
    finally:
        app.audio_ctrl.cleanup()
        print("[EXIT] Đã dừng Companion App thành công.")


if __name__ == "__main__":
    main()
