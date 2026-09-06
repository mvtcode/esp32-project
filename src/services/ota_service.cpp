#include "ota_service.h"
#include "log.h"
#include "audio_player_service.h"
#include "config_manager.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

static const char* TAG = "OTA";

OtaState OtaService::s_state = OtaState::IDLE;
char OtaService::s_errorMsg[128] = {0};
char OtaService::s_statusMsg[128] = {0};
volatile int OtaService::s_progressPercent = 0;
volatile size_t OtaService::s_downloadedBytes = 0;
volatile size_t OtaService::s_totalBytes = 0;
bool OtaService::s_isUpdating = false;
bool OtaService::s_clearNvs = false;
TaskHandle_t OtaService::s_otaTaskHandle = nullptr;

OtaProgressCallback OtaService::s_progressCb = nullptr;
OtaStatusCallback OtaService::s_statusCb = nullptr;
String OtaService::s_downloadUrl = "";
String OtaService::s_pendingChangelog = "";
String OtaService::s_pendingReleaseDate = "";

void OtaService::init() {
    s_state = OtaState::IDLE;
    s_isUpdating = false;
    s_clearNvs = false;
    s_progressPercent = 0;
    s_downloadedBytes = 0;
    s_totalBytes = 0;
    s_errorMsg[0] = '\0';
    s_statusMsg[0] = '\0';
    s_pendingChangelog = "";
    s_pendingReleaseDate = "";
}

bool OtaService::isUpdating() {
    return s_isUpdating;
}

OtaState OtaService::getState() {
    return s_state;
}

int OtaService::getProgressPercent() {
    return s_progressPercent;
}

size_t OtaService::getDownloadedBytes() {
    return s_downloadedBytes;
}

size_t OtaService::getTotalBytes() {
    return s_totalBytes;
}

const char* OtaService::getStatusMessage() {
    return s_statusMsg;
}

const char* OtaService::getErrorMessage() {
    return s_errorMsg;
}


String OtaService::extractJsonString(const String& json, const char* key) {
    String searchKey = "\"" + String(key) + "\":";
    int keyIdx = json.indexOf(searchKey);
    if (keyIdx == -1) return "";

    int startQuote = json.indexOf('"', keyIdx + searchKey.length());
    if (startQuote == -1) return "";

    int endQuote = json.indexOf('"', startQuote + 1);
    if (endQuote == -1) return "";

    return json.substring(startQuote + 1, endQuote);
}

int OtaService::extractJsonInt(const String& json, const char* key) {
    String searchKey = "\"" + String(key) + "\":";
    int keyIdx = json.indexOf(searchKey);
    if (keyIdx == -1) return 0;

    int valStart = keyIdx + searchKey.length();
    while (valStart < (int)json.length() && (json[valStart] == ' ' || json[valStart] == '\t')) {
        valStart++;
    }

    int valEnd = valStart;
    while (valEnd < (int)json.length() && (isdigit(json[valEnd]) || json[valEnd] == '-')) {
        valEnd++;
    }

    if (valStart < valEnd) {
        return json.substring(valStart, valEnd).toInt();
    }
    return 0;
}

bool OtaService::extractJsonBool(const String& json, const char* key) {
    String searchKey = "\"" + String(key) + "\":";
    int keyIdx = json.indexOf(searchKey);
    if (keyIdx == -1) return false;

    int valStart = keyIdx + searchKey.length();
    while (valStart < (int)json.length() && (json[valStart] == ' ' || json[valStart] == '\t')) {
        valStart++;
    }

    if (valStart < (int)json.length()) {
        if (json.substring(valStart).startsWith("true") || json.substring(valStart).startsWith("1")) {
            return true;
        }
    }
    return false;
}

uint32_t OtaService::parseVersion(const String& verStr) {
    if (verStr.length() == 0) return 0;

    int start = 0;
    if (verStr[0] == 'v' || verStr[0] == 'V') {
        start = 1;
    }

    int major = 0, minor = 0, patch = 0;
    int firstDot = verStr.indexOf('.', start);
    if (firstDot != -1) {
        major = verStr.substring(start, firstDot).toInt();
        int secondDot = verStr.indexOf('.', firstDot + 1);
        if (secondDot != -1) {
            minor = verStr.substring(firstDot + 1, secondDot).toInt();
            patch = verStr.substring(secondDot + 1).toInt();
        } else {
            minor = verStr.substring(firstDot + 1).toInt();
        }
    } else {
        major = verStr.substring(start).toInt();
    }

    return (uint32_t)(major * 1000000 + minor * 1000 + patch);
}

bool OtaService::checkUpdate(OtaInfo& info) {
    if (WiFi.status() != WL_CONNECTED) {
        snprintf(s_errorMsg, sizeof(s_errorMsg), "Chưa kết nối WiFi.");
        LOG_W(TAG, "%s", s_errorMsg);
        return false;
    }

    LOG_I(TAG, "Checking for updates at: %s", OTA_MANIFEST_URL);
    s_state = OtaState::CHECKING;

    WiFiClientSecure client;
    client.setInsecure(); // GitHub raw sử dụng HTTPS
    client.setTimeout(8000);

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setUserAgent("ESP32-CYD-Updater");

    if (!http.begin(client, OTA_MANIFEST_URL)) {
        snprintf(s_errorMsg, sizeof(s_errorMsg), "Không thể kết nối máy chủ OTA.");
        LOG_E(TAG, "%s", s_errorMsg);
        s_state = OtaState::ERROR;
        return false;
    }

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        snprintf(s_errorMsg, sizeof(s_errorMsg), "HTTP lỗi: %d", httpCode);
        LOG_W(TAG, "%s", s_errorMsg);
        http.end();
        s_state = OtaState::ERROR;
        return false;
    }

    String payload = http.getString();
    http.end();

    LOG_D(TAG, "Received manifest: %s", payload.c_str());

    info.target = extractJsonString(payload, "target");
    info.hardware = extractJsonString(payload, "hardware");
    info.version = extractJsonString(payload, "version");
    info.versionCode = extractJsonInt(payload, "version_code");
    info.releaseDate = extractJsonString(payload, "release_date");
    info.firmwareUrl = extractJsonString(payload, "firmware_url");

    bool explicitClear = extractJsonBool(payload, "clearNvs") || extractJsonBool(payload, "clear_nvs");
    String clearBelow = extractJsonString(payload, "clearNvsBelow");
    if (clearBelow.length() == 0) {
        clearBelow = extractJsonString(payload, "clear_nvs_below");
    }
    info.clearNvsBelow = clearBelow;

    bool belowClear = false;
    if (clearBelow.length() > 0) {
        uint32_t currentVerNum = parseVersion(FIRMWARE_VERSION);
        uint32_t clearBelowNum = parseVersion(clearBelow);
        if (clearBelowNum > 0 && currentVerNum < clearBelowNum) {
            belowClear = true;
            LOG_W(TAG, "Current version (%s) < clearNvsBelow (%s) -> clearNvs triggered!", FIRMWARE_VERSION, clearBelow.c_str());
        }
    }
    info.clearNvs = explicitClear || belowClear;

    info.changelog = extractJsonString(payload, "changelog");
    info.author = extractJsonString(payload, "author");
    info.email = extractJsonString(payload, "email");
    info.project = extractJsonString(payload, "project");


    if (info.version.length() == 0 || info.firmwareUrl.length() == 0) {
        snprintf(s_errorMsg, sizeof(s_errorMsg), "Dữ liệu version.json không hợp lệ.");
        LOG_E(TAG, "%s", s_errorMsg);
        s_state = OtaState::ERROR;
        return false;
    }

    // So sánh phiên bản với bản build hiện tại
    String currentVersion = FIRMWARE_VERSION;
    if (info.version != currentVersion) {
        info.hasUpdate = true;
        s_state = OtaState::UPDATE_AVAILABLE;
        LOG_I(TAG, "New version found: %s (Current: %s)", info.version.c_str(), currentVersion.c_str());
    } else {
        info.hasUpdate = false;
        s_state = OtaState::UP_TO_DATE;
        LOG_I(TAG, "Device is up to date: %s", currentVersion.c_str());

        // Đồng bộ changelog và ngày phát hành vào NVS khi thiết bị đang ở phiên bản mới nhất
        if (info.changelog.length() > 0) {
            ConfigManager::setChangelog(info.changelog);
        }
        if (info.releaseDate.length() > 0) {
            ConfigManager::setReleaseDate(info.releaseDate);
        }
    }

    return true;
}

bool OtaService::startUpdate(const String& firmwareUrl, bool clearNvs, const String& changelog, const String& releaseDate, OtaProgressCallback progressCb, OtaStatusCallback statusCb) {
    if (s_isUpdating) {
        LOG_W(TAG, "OTA update already in progress.");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        snprintf(s_errorMsg, sizeof(s_errorMsg), "Chưa kết nối WiFi.");
        if (statusCb) statusCb(OtaState::ERROR, s_errorMsg);
        return false;
    }

    // Reset lại toàn bộ tiến trình & giải phóng Update nếu còn sót
    if (s_otaTaskHandle != nullptr) {
        vTaskDelete(s_otaTaskHandle);
        s_otaTaskHandle = nullptr;
    }
    if (Update.isRunning()) {
        Update.end(false);
    }
    Update.abort();
    Update.clearError();
    s_downloadUrl = firmwareUrl;
    s_clearNvs = clearNvs;
    s_pendingChangelog = changelog;
    s_pendingReleaseDate = releaseDate;
    s_progressCb = progressCb;
    s_statusCb = statusCb;
    s_isUpdating = true;
    s_progressPercent = 0;
    s_downloadedBytes = 0;
    s_totalBytes = 0;
    s_errorMsg[0] = '\0';
    s_statusMsg[0] = '\0';
    s_state = OtaState::DOWNLOADING;

    // Giải phóng hoàn toàn AudioPlayer (Task, Decoders, I2S DMA) để giải phóng >40KB DRAM cho HTTPS TLS & OTA buffer
    if (AudioPlayerService::isInitialized()) {
        AudioPlayerService::releaseForOta();
    }

    BaseType_t res = xTaskCreatePinnedToCore(
        otaTask,
        "otaTask",
        8192,  // 8KB Stack an toan va tiet kiem DRAM cho TLS handshake
        NULL,
        1,     // Priority 1
        &s_otaTaskHandle,
        1      // Chạy trên Core 1 (để Core 0 rảnh 100% cho WiFi Driver & TCP/IP stack)
    );

    if (res != pdPASS) {
        s_isUpdating = false;
        s_state = OtaState::ERROR;
        snprintf(s_errorMsg, sizeof(s_errorMsg), "Không thể khởi tạo task OTA.");
        LOG_E(TAG, "%s", s_errorMsg);
        if (s_statusCb) s_statusCb(OtaState::ERROR, s_errorMsg);
        return false;
    }

    return true;
}

void OtaService::otaTask(void* param) {
    size_t freeDram = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    size_t maxDram = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    LOG_I(TAG, "OTA Task started. FreeDRAM: %u bytes, MaxBlockDRAM: %u bytes", freeDram, maxDram);
    if (s_statusCb) s_statusCb(OtaState::DOWNLOADING, "Đang kết nối máy chủ tải firmware...");

    // Kiểm tra phân vùng OTA hiện tại và phân vùng đích
    const esp_partition_t* runningPart = esp_ota_get_running_partition();
    const esp_partition_t* nextPart = esp_ota_get_next_update_partition(NULL);

    LOG_I(TAG, "Running partition: %s (offset 0x%06x, size %u KB)", 
          runningPart ? runningPart->label : "NULL", 
          runningPart ? runningPart->address : 0, 
          runningPart ? (uint32_t)(runningPart->size / 1024) : 0);
    LOG_I(TAG, "Next partition: %s (offset 0x%06x, size %u KB)", 
          nextPart ? nextPart->label : "NULL", 
          nextPart ? nextPart->address : 0, 
          nextPart ? (uint32_t)(nextPart->size / 1024) : 0);

    if (!nextPart) {
        snprintf(s_errorMsg, sizeof(s_errorMsg), "Chưa có phân vùng OTA! Cần flash merged_firmware lần đầu.");
        LOG_E(TAG, "%s", s_errorMsg);
        s_state = OtaState::ERROR;
        s_isUpdating = false;
        if (s_statusCb) s_statusCb(OtaState::ERROR, s_errorMsg);
        AudioPlayerService::init();
        vTaskDelete(NULL);
        return;
    }

    WiFiClientSecure client;
    client.setInsecure(); // Cloudflare CDN HTTPS với SSL/TLS tự động bỏ qua kiểm tra chứng chỉ lỗi thời
    client.setTimeout(20000); // 20s timeout per Rule 9

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setUserAgent("ESP32-CYD-Updater");
    http.setTimeout(20000); // 20s timeout per Rule 9

    LOG_I(TAG, "Connecting to firmware URL: %s", s_downloadUrl.c_str());
    if (!http.begin(client, s_downloadUrl)) {
        snprintf(s_errorMsg, sizeof(s_errorMsg), "Kết nối URL firmware thất bại.");
        LOG_E(TAG, "%s", s_errorMsg);
        s_state = OtaState::ERROR;
        s_isUpdating = false;
        if (s_statusCb) s_statusCb(OtaState::ERROR, s_errorMsg);
        AudioPlayerService::init();
        vTaskDelete(NULL);
        return;
    }

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        snprintf(s_errorMsg, sizeof(s_errorMsg), "HTTP tải firmware lỗi: %d", httpCode);
        LOG_E(TAG, "%s", s_errorMsg);
        http.end();
        s_state = OtaState::ERROR;
        s_isUpdating = false;
        if (s_statusCb) s_statusCb(OtaState::ERROR, s_errorMsg);
        AudioPlayerService::init();
        vTaskDelete(NULL);
        return;
    }

    int contentLength = http.getSize();
    LOG_I(TAG, "Firmware size from server: %d bytes (~%.2f MB), Partition size: %u bytes", 
          contentLength, (float)contentLength / (1024.0f * 1024.0f), nextPart->size);

    if (contentLength <= 0) {
        snprintf(s_errorMsg, sizeof(s_errorMsg), "Kích thước firmware không hợp lệ.");
        LOG_E(TAG, "%s", s_errorMsg);
        http.end();
        s_state = OtaState::ERROR;
        s_isUpdating = false;
        if (s_statusCb) s_statusCb(OtaState::ERROR, s_errorMsg);
        AudioPlayerService::init();
        vTaskDelete(NULL);
        return;
    }

    if ((size_t)contentLength > nextPart->size) {
        snprintf(s_errorMsg, sizeof(s_errorMsg), "Firmware (%u KB) lớn hơn phân vùng (%u KB)!", 
                 (uint32_t)(contentLength / 1024), (uint32_t)(nextPart->size / 1024));
        LOG_E(TAG, "%s", s_errorMsg);
        http.end();
        s_state = OtaState::ERROR;
        s_isUpdating = false;
        if (s_statusCb) s_statusCb(OtaState::ERROR, s_errorMsg);
        AudioPlayerService::init();
        vTaskDelete(NULL);
        return;
    }

    esp_ota_handle_t otaHandle = 0;
    esp_err_t otaErr = esp_ota_begin(nextPart, contentLength, &otaHandle);
    if (otaErr != ESP_OK) {
        snprintf(s_errorMsg, sizeof(s_errorMsg), "Khởi tạo Flash OTA thất bại (Mã 0x%x)", otaErr);
        LOG_E(TAG, "%s", s_errorMsg);
        http.end();
        client.stop();
        s_state = OtaState::ERROR;
        s_isUpdating = false;
        if (s_statusCb) s_statusCb(OtaState::ERROR, s_errorMsg);
        AudioPlayerService::init();
        vTaskDelete(NULL);
        return;
    }

    WiFiClient* stream = http.getStreamPtr();
    if (!stream) {
        snprintf(s_errorMsg, sizeof(s_errorMsg), "Không thể lấy luồng dữ liệu mạng.");
        LOG_E(TAG, "%s", s_errorMsg);
        esp_ota_abort(otaHandle);
        http.end();
        client.stop();
        s_state = OtaState::ERROR;
        s_isUpdating = false;
        if (s_statusCb) s_statusCb(OtaState::ERROR, s_errorMsg);
        AudioPlayerService::init();
        vTaskDelete(NULL);
        return;
    }

    const size_t bufSize = 2048; // 2KB buffer nhỏ gọn, an toàn tuyệt đối cho DRAM
    uint8_t* buffer = (uint8_t*)malloc(bufSize);
    if (!buffer) {
        snprintf(s_errorMsg, sizeof(s_errorMsg), "Không đủ RAM cho buffer OTA.");
        LOG_E(TAG, "%s", s_errorMsg);
        esp_ota_abort(otaHandle);
        http.end();
        client.stop();
        s_state = OtaState::ERROR;
        s_isUpdating = false;
        if (s_statusCb) s_statusCb(OtaState::ERROR, s_errorMsg);
        AudioPlayerService::init();
        vTaskDelete(NULL);
        return;
    }

    size_t totalWritten = 0;
    int lastPercent = -1;
    unsigned long lastActivity = millis();
    bool writeError = false;

    while (totalWritten < (size_t)contentLength) {
        size_t remaining = (size_t)contentLength - totalWritten;
        size_t toRead = (remaining < bufSize) ? remaining : bufSize;

        // Đọc trực tiếp từ stream (blocking an toàn với timeout từ socket, không phụ thuộc available())
        int bytesRead = stream->read(buffer, toRead);

        if (bytesRead > 0) {
            esp_err_t err = esp_ota_write(otaHandle, buffer, bytesRead);
            if (err != ESP_OK) {
                snprintf(s_errorMsg, sizeof(s_errorMsg), "Ghi Flash lỗi (%u / %u KB, Mã 0x%x)", 
                         (uint32_t)(totalWritten / 1024), (uint32_t)(contentLength / 1024), err);
                LOG_E(TAG, "%s", s_errorMsg);
                writeError = true;
                break;
            }

            totalWritten += bytesRead;
            lastActivity = millis();
            s_downloadedBytes = totalWritten;
            s_totalBytes = contentLength;

            int currentPercent = (int)(((int64_t)totalWritten * 100) / contentLength);
            if (currentPercent != lastPercent) {
                lastPercent = currentPercent;
                s_progressPercent = currentPercent;
                snprintf(s_statusMsg, sizeof(s_statusMsg), "Đang nạp: %d%% (%u / %u KB)", 
                         currentPercent, (uint32_t)(totalWritten / 1024), (uint32_t)(contentLength / 1024));
                if (s_progressCb) {
                    s_progressCb(currentPercent, totalWritten, contentLength);
                }
            }
        } else if (bytesRead == 0) {
            // Socket tạm thời chưa có dữ liệu mới, chờ tối đa 25s
            if (millis() - lastActivity > 25000) {
                snprintf(s_errorMsg, sizeof(s_errorMsg), "Quá thời gian tải dữ liệu OTA (Timeout 25s).");
                LOG_E(TAG, "%s", s_errorMsg);
                writeError = true;
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        } else {
            // bytesRead < 0: Socket bị ngắt kết nối
            if (!http.connected() && totalWritten < (size_t)contentLength) {
                snprintf(s_errorMsg, sizeof(s_errorMsg), "Mất kết nối máy chủ OTA (%u / %u KB).", 
                         (uint32_t)(totalWritten / 1024), (uint32_t)(contentLength / 1024));
                LOG_E(TAG, "%s", s_errorMsg);
                writeError = true;
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        // Nhường 1 tick cho FreeRTOS IDLE task reset Watchdog Timer (Rule 8)
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    free(buffer);
    http.end();
    client.stop(); // Giải phóng hoàn toàn kết nối TLS để giải phóng >35KB DRAM trước khi init AudioPlayer

    if (writeError || totalWritten != (size_t)contentLength) {
        if (!writeError) {
            snprintf(s_errorMsg, sizeof(s_errorMsg), "Tải firmware không đủ (%u / %u KB).", 
                     (uint32_t)(totalWritten / 1024), (uint32_t)(contentLength / 1024));
            LOG_E(TAG, "%s", s_errorMsg);
        }
        esp_ota_abort(otaHandle);
        s_state = OtaState::ERROR;
        s_isUpdating = false;
        if (s_statusCb) s_statusCb(OtaState::ERROR, s_errorMsg);
        AudioPlayerService::init();
        vTaskDelete(NULL);
        return;
    }

    esp_err_t finishErr = esp_ota_end(otaHandle);
    if (finishErr != ESP_OK) {
        snprintf(s_errorMsg, sizeof(s_errorMsg), "Xác thực OTA thất bại (Mã 0x%x)", finishErr);
        LOG_E(TAG, "%s", s_errorMsg);
        s_state = OtaState::ERROR;
        s_isUpdating = false;
        if (s_statusCb) s_statusCb(OtaState::ERROR, s_errorMsg);
        AudioPlayerService::init();
        vTaskDelete(NULL);
        return;
    }

    esp_err_t bootErr = esp_ota_set_boot_partition(nextPart);
    if (bootErr != ESP_OK) {
        snprintf(s_errorMsg, sizeof(s_errorMsg), "Thiết lập phân vùng boot thất bại (Mã 0x%x)", bootErr);
        LOG_E(TAG, "%s", s_errorMsg);
        s_state = OtaState::ERROR;
        s_isUpdating = false;
        if (s_statusCb) s_statusCb(OtaState::ERROR, s_errorMsg);
        AudioPlayerService::init();
        vTaskDelete(NULL);
        return;
    }

    LOG_I(TAG, "OTA update SUCCESSFUL! (%u bytes written)", totalWritten);

    if (s_clearNvs) {
        LOG_I(TAG, "clearNvs is TRUE -> Resetting NVS Flash to factory defaults before reboot...");
        ConfigManager::resetToDefaults();
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    // Ghi changelog và releaseDate của bản firmware mới vào NVS trước khi reboot
    if (s_pendingChangelog.length() > 0) {
        ConfigManager::setChangelog(s_pendingChangelog);
    }
    if (s_pendingReleaseDate.length() > 0) {
        ConfigManager::setReleaseDate(s_pendingReleaseDate);
    }
    ConfigManager::flush();

    s_state = OtaState::SUCCESS;
    s_isUpdating = false;
    if (s_statusCb) s_statusCb(OtaState::SUCCESS, "Cập nhật thành công! Đang khởi động lại...");

    // Chờ 1.5s để UI hiển thị thông báo trước khi reboot
    vTaskDelay(pdMS_TO_TICKS(1500));
    ESP.restart();
    vTaskDelete(NULL);
}
