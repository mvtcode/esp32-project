#pragma once

/**
 * @file version.h
 * @brief Quản lý tập trung các thông số phiên bản và cấu hình OTA.
 *        Toàn bộ giá trị BẮT BUỘC phải được định nghĩa qua `build_flags` trong platformio.ini.
 *        Tuyệt đối không hardcode giá trị tại file mã nguồn C++.
 */

#ifndef FIRMWARE_VERSION
#error "FIRMWARE_VERSION chua duoc dinh nghia trong platformio.ini! Hay them: -DFIRMWARE_VERSION=\\\"v1.0.0\\\" vao build_flags"
#endif

#ifndef FIRMWARE_RELEASE_DATE
#error "FIRMWARE_RELEASE_DATE chua duoc dinh nghia trong platformio.ini! Hay them: -DFIRMWARE_RELEASE_DATE=\\\"06/09/2026\\\" vao build_flags"
#endif

#ifndef OTA_MANIFEST_URL
#error "OTA_MANIFEST_URL chua duoc dinh nghia trong platformio.ini! Hay them: -DOTA_MANIFEST_URL=\\\"https://...\\\" vao build_flags"
#endif
