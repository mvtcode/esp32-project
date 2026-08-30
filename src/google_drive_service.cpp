#include "google_drive_service.h"
#include "log.h"

const char *GoogleDriveService::ssid = "";
const char *GoogleDriveService::password = "";
const char *GoogleDriveService::scriptUrl = "";
QueueHandle_t GoogleDriveService::uploadQueue = NULL;
SemaphoreHandle_t GoogleDriveService::sdMutex = NULL;
UploadStatus GoogleDriveService::current_status = UPLOAD_IDLE;
uint32_t GoogleDriveService::last_status_change_time = 0;
bool GoogleDriveService::is_wifi_connected = false;
bool GoogleDriveService::is_sd_mounted = false;
bool GoogleDriveService::is_ntp_synced = false;
bool GoogleDriveService::upload_enabled = false;
uint32_t GoogleDriveService::upload_cooldown_ms = 5000;

bool GoogleDriveService::face_previously_present = false;
uint32_t GoogleDriveService::last_face_seen_time = 0;
uint32_t GoogleDriveService::last_upload_time = 0;
int GoogleDriveService::last_center_x = -1;
int GoogleDriveService::last_center_y = -1;
int GoogleDriveService::consecutive_frames = 0;

void GoogleDriveService::syncNTPTime() {
    LOG_I("NTP", "Synchronizing time from Internet (GMT+7)...");
    configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com", "time.nist.gov");

    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 3000)) {
        is_ntp_synced = true;
        char timeStr[32];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
        LOG_I("NTP", "Time synchronized successfully: %s", timeStr);
    } else {
        LOG_W("NTP", "Time sync timeout (will retry later)");
    }
}

String GoogleDriveService::generateTimestampFilename() {
    char fname[48];
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 100)) {
        // Định dạng YYYYMMDDHHmmss (ví dụ: face_20260829194530.jpg)
        strftime(fname, sizeof(fname), "face_%Y%m%d%H%M%S.jpg", &timeinfo);
    } else {
        // Fallback nếu chưa đồng bộ NTP
        snprintf(fname, sizeof(fname), "face_00000000000000_%lu.jpg", millis());
    }
    return String(fname);
}

void GoogleDriveService::cleanupOrphanTmpFiles() {
    if (!is_sd_mounted) return;

    if (sdMutex != NULL && xSemaphoreTake(sdMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        File root = SD_MMC.open("/faces");
        if (root && root.isDirectory()) {
            File file = root.openNextFile();
            while (file) {
                String name = String(file.name());
                file.close();

                // Xóa tất cả các file .tmp bị ghi dở do mất điện hoặc reset ở lần chạy trước
                if (name.endsWith(".tmp")) {
                    String fullPath = "/faces/" + name;
                    SD_MMC.remove(fullPath.c_str());
                    LOG_I("FaultTolerance", "Cleaned up corrupted/incomplete file: %s", fullPath.c_str());
                }
                file = root.openNextFile();
            }
            root.close();
        }
        xSemaphoreGive(sdMutex);
    }
}

bool GoogleDriveService::isValidJPEG(const uint8_t *data, size_t len) {
    if (data == nullptr || len < 4) return false;
    // Kiểm tra Start of Image (0xFF, 0xD8) và End of Image (0xFF, 0xD9)
    return (data[0] == 0xFF && data[1] == 0xD8 && data[len - 2] == 0xFF && data[len - 1] == 0xD9);
}

bool GoogleDriveService::saveToSDAtomic(const uint8_t *data, size_t len, const String &filename) {
    if (!is_sd_mounted || data == nullptr || len == 0) return false;

    if (sdMutex == NULL || xSemaphoreTake(sdMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        LOG_W("FaultTolerance", "Could not acquire sdMutex for writing");
        return false;
    }

    // 1. Kiểm tra dung lượng thẻ nhớ còn trống (Nếu < 10MB -> xóa file cũ)
    uint64_t totalBytes = SD_MMC.totalBytes();
    uint64_t usedBytes = SD_MMC.usedBytes();
    if (totalBytes > usedBytes && (totalBytes - usedBytes) < (10 * 1024 * 1024)) {
        LOG_W("FaultTolerance", "SD card low space! Cleaning oldest files...");
        File root = SD_MMC.open("/faces");
        if (root && root.isDirectory()) {
            File f = root.openNextFile();
            if (f) {
                String oldName = String(f.name());
                f.close();
                String delPath = "/faces/" + oldName;
                SD_MMC.remove(delPath.c_str());
                LOG_I("FaultTolerance", "Deleted old cache file: %s", delPath.c_str());
            }
            root.close();
        }
    }

    // 2. Ghi vào file tạm .tmp trước
    String baseName = filename;
    if (baseName.endsWith(".jpg")) {
        baseName = baseName.substring(0, baseName.length() - 4);
    }
    String tmpPath = "/faces/" + baseName + ".tmp";
    String finalPath = "/faces/" + filename;

    File f = SD_MMC.open(tmpPath.c_str(), FILE_WRITE);
    if (!f) {
        LOG_E("FaultTolerance", "Error opening temp file: %s", tmpPath.c_str());
        xSemaphoreGive(sdMutex);
        return false;
    }

    size_t written = f.write(data, len);
    f.flush();
    f.close();

    if (written != len) {
        LOG_E("FaultTolerance", "Write incomplete (%u/%u bytes), removing temp file", (unsigned)written, (unsigned)len);
        SD_MMC.remove(tmpPath.c_str());
        xSemaphoreGive(sdMutex);
        return false;
    }

    // 3. Đổi tên nguyên tử sang .jpg (Atomic Rename)
    if (SD_MMC.exists(finalPath.c_str())) {
        SD_MMC.remove(finalPath.c_str());
    }

    bool success = false;
    if (SD_MMC.rename(tmpPath.c_str(), finalPath.c_str())) {
        LOG_I("FaultTolerance", "Atomically saved file to SD: %s", finalPath.c_str());
        success = true;
    } else {
        LOG_E("FaultTolerance", "Rename failed from %s to %s", tmpPath.c_str(), finalPath.c_str());
        SD_MMC.remove(tmpPath.c_str());
    }

    xSemaphoreGive(sdMutex);
    return success;
}

bool GoogleDriveService::uploadSingleFile(const uint8_t *data, size_t len, const char *filename) {
    if (!is_wifi_connected || WiFi.status() != WL_CONNECTED || data == nullptr || len == 0) {
        return false;
    }

    // Kiểm tra tính toàn vẹn của file ảnh JPEG trước khi upload
    if (!isValidJPEG(data, len)) {
        LOG_W("FaultTolerance", "Skipping corrupted JPEG file: %s", filename);
        return false;
    }

    // 1. Mã hóa Base64
    size_t base64_len = 0;
    mbedtls_base64_encode(nullptr, 0, &base64_len, data, len);
    char *base64_buf = (char *)ps_malloc(base64_len + 1);
    if (!base64_buf) base64_buf = (char *)malloc(base64_len + 1);

    if (!base64_buf) {
        LOG_E("GoogleDrive", "Memory allocation failed for Base64");
        return false;
    }

    size_t written = 0;
    mbedtls_base64_encode((unsigned char *)base64_buf, base64_len + 1, &written, data, len);
    base64_buf[written] = '\0';

    // 2. Chuẩn bị JSON Payload (Dùng reserve tránh phân mảnh Heap)
    String jsonPayload;
    jsonPayload.reserve(base64_len + 128);
    jsonPayload += "{\"image\":\"";
    jsonPayload += base64_buf;
    jsonPayload += "\",\"filename\":\"";
    jsonPayload += filename;
    jsonPayload += "\"}";
    free(base64_buf);
    base64_buf = nullptr;

    // 3. Gửi HTTPS POST
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(12000);

    HTTPClient http;
    http.begin(client, scriptUrl);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.addHeader("Content-Type", "application/json");

    LOG_I("GoogleDrive", "Uploading %s (%u bytes) to Google Apps Script...", filename, (unsigned)len);
    int httpCode = http.POST(jsonPayload);

    bool success = false;
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_FOUND || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String response = http.getString();
        LOG_I("GoogleDrive", "Upload SUCCESS! Response: %s", response.c_str());
        success = true;
    } else {
        LOG_E("GoogleDrive", "Upload FAILED! HTTP code: %d", httpCode);
    }
    http.end();
    return success;
}

void GoogleDriveService::uploadTaskWorker(void *param) {
    LOG_I("GoogleDrive", "Background Worker Task started on Core 0");

    if (WiFi.getMode() == WIFI_OFF) {
        WiFi.mode(WIFI_STA);
    }
    WiFi.begin(ssid, password);
    LOG_I("GoogleDrive", "Connecting to WiFi: %s ...", ssid);

    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 25) {
        vTaskDelay(pdMS_TO_TICKS(500));
        retry++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        is_wifi_connected = true;
        LOG_I("GoogleDrive", "WiFi Connected! IP: %s", WiFi.localIP().toString().c_str());
        // Đồng bộ thời gian từ Internet NTP ngay khi có mạng
        syncNTPTime();
    } else {
        LOG_W("GoogleDrive", "WiFi not connected yet (will auto-reconnect)");
    }

    UploadJob job;
    while (true) {
        if (WiFi.status() != WL_CONNECTED) {
            is_wifi_connected = false;
            WiFi.reconnect();
            vTaskDelay(pdMS_TO_TICKS(3000));
            if (WiFi.status() == WL_CONNECTED) {
                is_wifi_connected = true;
                if (!is_ntp_synced) syncNTPTime();
            }
        }

        // 1. Quét và tải các file lưu trong cache SD Card /faces/
        if (is_sd_mounted && is_wifi_connected && upload_enabled) {
            if (sdMutex != NULL && xSemaphoreTake(sdMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
                File root = SD_MMC.open("/faces");
                if (root && root.isDirectory()) {
                    File file = root.openNextFile();
                    while (file) {
                        if (!file.isDirectory() && String(file.name()).endsWith(".jpg")) {
                            size_t fileSize = file.size();
                            String fileNameStr = String(file.name());
                            String fullPath = "/faces/" + fileNameStr;

                            if (fileSize > 0) {
                                uint8_t *fileBuf = (uint8_t *)ps_malloc(fileSize);
                                if (!fileBuf) fileBuf = (uint8_t *)malloc(fileSize);

                                if (fileBuf) {
                                    file.read(fileBuf, fileSize);
                                    file.close();
                                    // Mở lock trước khi upload HTTP để không chặn Core 1 ghi ảnh
                                    xSemaphoreGive(sdMutex);

                                    current_status = UPLOAD_IN_PROGRESS;
                                    last_status_change_time = millis();

                                    bool ok = uploadSingleFile(fileBuf, fileSize, fileNameStr.c_str());
                                    free(fileBuf);

                                    // Lấy lại lock để xóa file hoặc cập nhật SD
                                    if (sdMutex != NULL && xSemaphoreTake(sdMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
                                        if (ok) {
                                            current_status = UPLOAD_SUCCESS;
                                            SD_MMC.remove(fullPath.c_str());
                                            LOG_I("FaultTolerance", "Uploaded & safely removed: %s", fullPath.c_str());
                                        } else {
                                            current_status = UPLOAD_FAILED;
                                            LOG_W("FaultTolerance", "Retaining file for next retry: %s", fullPath.c_str());
                                        }
                                        xSemaphoreGive(sdMutex);
                                    }
                                    last_status_change_time = millis();
                                    vTaskDelay(pdMS_TO_TICKS(1000));
                                    break;
                                } else {
                                    file.close();
                                }
                            } else {
                                // File 0 byte bị lỗi do mất điện -> Xóa ngay
                                file.close();
                                SD_MMC.remove(fullPath.c_str());
                                LOG_W("FaultTolerance", "Removed empty 0-byte file: %s", fullPath.c_str());
                            }
                        } else {
                            file.close();
                        }
                        file = root.openNextFile();
                    }
                    root.close();
                } else {
                    xSemaphoreGive(sdMutex);
                }
            }
        }

        // 2. Nhận job từ hàng đợi RAM Queue (cho trường hợp không dùng SD card)
        if (xQueueReceive(uploadQueue, &job, pdMS_TO_TICKS(500)) == pdTRUE) {
            if (job.jpg_buf != nullptr && job.jpg_len > 0) {
                if (is_wifi_connected && upload_enabled) {
                    current_status = UPLOAD_IN_PROGRESS;
                    last_status_change_time = millis();

                    bool ok = uploadSingleFile(job.jpg_buf, job.jpg_len, job.filename);
                    current_status = ok ? UPLOAD_SUCCESS : UPLOAD_FAILED;
                    last_status_change_time = millis();
                }
                free(job.jpg_buf);
                job.jpg_buf = nullptr;
            }
        }
    }
}

bool GoogleDriveService::init(const char *wifi_ssid, const char *wifi_password, const char *google_script_url, uint32_t cooldown_sec) {
    ssid = wifi_ssid;
    password = wifi_password;
    scriptUrl = google_script_url;
    upload_cooldown_ms = max(3u, cooldown_sec) * 1000;

    if (sdMutex == NULL) {
        sdMutex = xSemaphoreCreateMutex();
    }

    // 1. Thử khởi tạo SD Card (SD_MMC 1-bit mode)
    if (SD_MMC.begin("/sdcard", true)) {
        is_sd_mounted = true;
        uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
        LOG_I("GoogleDrive", "SD Card mounted successfully! Size: %llu MB", cardSize);
        if (!SD_MMC.exists("/faces")) {
            SD_MMC.mkdir("/faces");
        }
        // Dọn sạch các file .tmp bị ghi dở ở các phiên chạy trước
        cleanupOrphanTmpFiles();
    } else {
        is_sd_mounted = false;
        LOG_W("GoogleDrive", "SD Card not mounted, using PSRAM Memory Queue cache");
    }

    uploadQueue = xQueueCreate(3, sizeof(UploadJob));
    if (uploadQueue == NULL) {
        LOG_E("GoogleDrive", "Failed to create Upload Queue");
        return false;
    }

    // Khởi chạy Task upload chạy nền trên Core 0
    xTaskCreatePinnedToCore(
        uploadTaskWorker,
        "GoogleDriveWorker",
        12 * 1024,
        NULL,
        1,
        NULL,
        0
    );

    return true;
}

void GoogleDriveService::processFaceTrigger(const CameraFrame &vgaFrame, const FaceDetectionResult &aiResult) {
    if (!upload_enabled || !vgaFrame.isValid()) {
        current_status = UPLOAD_IDLE;
        return;
    }

    uint32_t now = millis();

    // Reset status hiển thị sau 3 giây
    if (current_status != UPLOAD_IDLE && current_status != UPLOAD_IN_PROGRESS && (now - last_status_change_time > 3000)) {
        current_status = UPLOAD_IDLE;
    }

    bool has_face = !aiResult.faces.empty();

    if (has_face) {
        consecutive_frames++;
        last_face_seen_time = now;
        const FaceBox &best_face = aiResult.faces[0];

        // Cooldown đủ thời gian VÀ không đang upload dở
        // → Đảm bảo SD queue chỉ chứa tối đa 1 file pending tại mọi thời điểm.
        // Nếu upload mất 8s nhưng cooldown chỉ 5s, file tiếp theo sẽ không được
        // trigger cho đến khi upload hiện tại hoàn thành (SUCCESS hoặc FAILED).
        bool cooldown_ok = (now - last_upload_time >= upload_cooldown_ms) &&
                           (current_status != UPLOAD_IN_PROGRESS);

        int cx = (best_face.x1 + best_face.x2) / 2;
        int cy = (best_face.y1 + best_face.y2) / 2;

        bool should_trigger = false;

        if (!face_previously_present) {
            if (consecutive_frames >= 2 && best_face.score >= 0.40f && cooldown_ok) {
                should_trigger = true;
                face_previously_present = true;
            }
        } else {
            int dx = abs(cx - last_center_x);
            int dy = abs(cy - last_center_y);
            if ((dx > 80 || dy > 80) && cooldown_ok && best_face.score >= 0.40f) {
                should_trigger = true;
            }
        }

        if (should_trigger) {
            last_upload_time = now;
            last_center_x = cx;
            last_center_y = cy;

            // Tính tỷ lệ scale theo độ rộng thực tế của vgaFrame (VGA: 640x480 -> scale 2, QVGA: 320x240 -> scale 1)
            int scale = (vgaFrame.width > 320) ? 2 : 1;
            int frame_x1 = best_face.x1 * scale;
            int frame_y1 = best_face.y1 * scale;
            int frame_w  = best_face.width() * scale;
            int frame_h  = best_face.height() * scale;

            int pad_w = frame_w * 0.25f;
            int pad_h = frame_h * 0.25f;

            int crop_x1 = max(0, frame_x1 - pad_w);
            int crop_y1 = max(0, frame_y1 - pad_h);
            int crop_x2 = min(vgaFrame.width - 1, frame_x1 + frame_w + pad_w);
            int crop_y2 = min(vgaFrame.height - 1, frame_y1 + frame_h + pad_h);

            int crop_w = (crop_x2 - crop_x1) & ~1;
            int crop_h = (crop_y2 - crop_y1) & ~1;

            if (crop_w > 30 && crop_h > 30) {
                uint8_t *crop_buf = (uint8_t *)ps_malloc(crop_w * crop_h * sizeof(uint16_t));
                if (!crop_buf) crop_buf = (uint8_t *)malloc(crop_w * crop_h * sizeof(uint16_t));

                if (crop_buf) {
                    const uint16_t *src = (const uint16_t *)vgaFrame.buffer;
                    uint16_t *dst = (uint16_t *)crop_buf;

                    for (int r = 0; r < crop_h; ++r) {
                        int src_idx = (crop_y1 + r) * vgaFrame.width + crop_x1;
                        memcpy(&dst[r * crop_w], &src[src_idx], crop_w * sizeof(uint16_t));
                    }

                    uint8_t *jpg_buf = nullptr;
                    size_t jpg_len = 0;
                    bool converted = fmt2jpg(crop_buf, crop_w * crop_h * 2, crop_w, crop_h, PIXFORMAT_RGB565, 90, &jpg_buf, &jpg_len);
                    free(crop_buf);

                    if (converted && jpg_buf != nullptr && jpg_len > 0) {
                        // Sinh tên file chuẩn YYYYMMDDHHmmss
                        String filename = generateTimestampFilename();

                        // 1. Thử lưu vào SD Card bằng cơ chế Atomic Write
                        bool saved_to_sd = false;
                        if (is_sd_mounted) {
                            saved_to_sd = saveToSDAtomic(jpg_buf, jpg_len, filename);
                        }

                        // 2. Nếu không có SD card hoặc lưu SD thất bại -> Fallback vào RAM Queue
                        if (!saved_to_sd) {
                            UploadJob job;
                            job.jpg_buf = jpg_buf;
                            job.jpg_len = jpg_len;
                            strncpy(job.filename, filename.c_str(), sizeof(job.filename) - 1);
                            job.filename[sizeof(job.filename) - 1] = '\0';

                            if (xQueueSend(uploadQueue, &job, 0) != pdTRUE) {
                                LOG_W("GoogleDrive", "Upload Queue Full, dropping frame");
                                free(jpg_buf);
                            }
                        } else {
                            free(jpg_buf);
                        }
                    }
                }
            }
        }
    } else {
        consecutive_frames = 0;
        if (now - last_face_seen_time > 1500) {
            face_previously_present = false;
            last_center_x = -1;
            last_center_y = -1;
        }
    }
}
