#ifndef GOOGLE_DRIVE_SERVICE_H
#define GOOGLE_DRIVE_SERVICE_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "time.h"
#include "FS.h"
#include "SD_MMC.h"
#include "mbedtls/base64.h"
#include "camera_service.h"
#include "face_detector.h"
#include "upload_types.h"  // UploadStatus enum (shared)

// Cấu trúc gói tin upload đưa vào Queue RAM
struct UploadJob {
    uint8_t *jpg_buf = nullptr;
    size_t jpg_len = 0;
    char filename[48];
};

class GoogleDriveService {
private:
    static const char *ssid;
    static const char *password;
    static const char *scriptUrl;
    static QueueHandle_t uploadQueue;
    static UploadStatus current_status;
    static uint32_t last_status_change_time;
    static bool is_wifi_connected;
    static bool is_sd_mounted;
    static bool is_ntp_synced;

    // Bộ theo dõi trạng thái kích hoạt upload thông minh
    static bool face_previously_present;
    static uint32_t last_face_seen_time;
    static uint32_t last_upload_time;
    static int last_center_x;
    static int last_center_y;
    static int consecutive_frames;

    // Trạng thái bật/tắt tính năng Upload và Tần suất Cooldown
    static bool upload_enabled;
    static uint32_t upload_cooldown_ms;

    static void uploadTaskWorker(void *param);
    static bool uploadSingleFile(const uint8_t *data, size_t len, const char *filename);
    
    // Các hàm xử lý chịu lỗi và NTP
    static void syncNTPTime();
    static void cleanupOrphanTmpFiles();
    static bool isValidJPEG(const uint8_t *data, size_t len);
    static String generateTimestampFilename();
    static bool saveToSDAtomic(const uint8_t *data, size_t len, const String &filename);

public:
    static bool init(const char *wifi_ssid, const char *wifi_password, const char *google_script_url, uint32_t cooldown_sec = 5);

    // Bật / tắt tính năng upload
    static void setUploadEnabled(bool enabled) { upload_enabled = enabled; }
    static bool isUploadEnabled() { return upload_enabled; }
    static void setUploadCooldown(uint32_t seconds) { upload_cooldown_ms = max(3u, seconds) * 1000; }

    // Xử lý logic trigger thông minh, crop độ phân giải cao & đẩy job vào hàng đợi
    static void processFaceTrigger(const CameraFrame &frame, const FaceDetectionResult &aiResult);

    // Lấy trạng thái upload hiện tại để hiển thị HUD trên màn hình
    static UploadStatus getStatus() { return current_status; }
    static bool isConnected() { return is_wifi_connected; }
    static bool hasSDCard() { return is_sd_mounted; }
    static bool isTimeSynced() { return is_ntp_synced; }
};


#endif // GOOGLE_DRIVE_SERVICE_H
