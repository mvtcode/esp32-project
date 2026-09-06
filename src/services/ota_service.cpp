#include "ota_service.h"
#include "log.h"
#include "audio_player_service.h"
#include "config_manager.h"

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

void OtaService::init() {
    s_state = OtaState::IDLE;
    s_isUpdating = false;
    s_clearNvs = false;
    s_progressPercent = 0;
    s_downloadedBytes = 0;
    s_totalBytes = 0;
    s_errorMsg[0] = '\0';
    s_statusMsg[0] = '\0';
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
    }

    return true;
}

bool OtaService::startUpdate(const String& firmwareUrl, bool clearNvs, OtaProgressCallback progressCb, OtaStatusCallback statusCb) {
    if (s_isUpdating) {
        LOG_W(TAG, "OTA update already in progress.");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        snprintf(s_errorMsg, sizeof(s_errorMsg), "Chưa kết nối WiFi.");
        if (statusCb) statusCb(OtaState::ERROR, s_errorMsg);
        return false;
    }

    s_downloadUrl = firmwareUrl;
    s_clearNvs = clearNvs;
    s_progressCb = progressCb;
    s_statusCb = statusCb;
    s_isUpdating = true;
    s_state = OtaState::DOWNLOADING;

    // Tạm dừng phát nhạc và giải phóng tài nguyên I2S DMA/SD buffer để nhường RAM cho HTTPS TLS
    if (AudioPlayerService::isInitialized()) {
        AudioPlayerService::stop();
    }

    BaseType_t res = xTaskCreatePinnedToCore(
        otaTask,
        "otaTask",
        8192, // 8KB Stack an toàn cho TLS handshake
        NULL,
        1,    // Priority 1
        &s_otaTaskHandle,
        0     // Chạy trên Core 0 (để Core 1 render UI)
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
    LOG_I(TAG, "OTA download task started. URL: %s", s_downloadUrl.c_str());
    if (s_statusCb) s_statusCb(OtaState::DOWNLOADING, "Đang kết nối máy chủ tải firmware...");

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(15000); // 15s timeout

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setUserAgent("ESP32-CYD-Updater");

    if (!http.begin(client, s_downloadUrl)) {
        snprintf(s_errorMsg, sizeof(s_errorMsg), "Kết nối URL firmware thất bại.");
        LOG_E(TAG, "%s", s_errorMsg);
        s_state = OtaState::ERROR;
        s_isUpdating = false;
        if (s_statusCb) s_statusCb(OtaState::ERROR, s_errorMsg);
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
        vTaskDelete(NULL);
        return;
    }

    int contentLength = http.getSize();
    LOG_I(TAG, "Firmware size: %d bytes (~%.2f MB)", contentLength, (float)contentLength / (1024.0f * 1024.0f));

    if (contentLength <= 0) {
        snprintf(s_errorMsg, sizeof(s_errorMsg), "Kích thước firmware không hợp lệ.");
        LOG_E(TAG, "%s", s_errorMsg);
        http.end();
        s_state = OtaState::ERROR;
        s_isUpdating = false;
        if (s_statusCb) s_statusCb(OtaState::ERROR, s_errorMsg);
        vTaskDelete(NULL);
        return;
    }

    if (!Update.begin(contentLength, U_FLASH)) {
        snprintf(s_errorMsg, sizeof(s_errorMsg), "Không đủ dung lượng Flash cho OTA: %s", Update.errorString());
        LOG_E(TAG, "%s", s_errorMsg);
        http.end();
        s_state = OtaState::ERROR;
        s_isUpdating = false;
        if (s_statusCb) s_statusCb(OtaState::ERROR, s_errorMsg);
        vTaskDelete(NULL);
        return;
    }

    WiFiClient* stream = http.getStreamPtr();
    size_t bufferSize = 4096; // Cấp phát 4KB buffer trên Heap (Rule 4)
    uint8_t* buffer = (uint8_t*)malloc(bufferSize);
    if (!buffer) {
        bufferSize = 1024;
        buffer = (uint8_t*)malloc(bufferSize);
    }

    if (!buffer) {
        snprintf(s_errorMsg, sizeof(s_errorMsg), "Không đủ RAM cấp phát buffer OTA.");
        LOG_E(TAG, "%s", s_errorMsg);
        Update.abort();
        http.end();
        s_state = OtaState::ERROR;
        s_isUpdating = false;
        if (s_statusCb) s_statusCb(OtaState::ERROR, s_errorMsg);
        vTaskDelete(NULL);
        return;
    }

    size_t totalDownloaded = 0;
    int lastPercent = -1;
    unsigned long lastActivity = millis();
    bool streamError = false;

    while (http.connected() && (totalDownloaded < (size_t)contentLength)) {
        size_t availableBytes = stream->available();
        if (availableBytes > 0) {
            size_t bytesToRead = (availableBytes > bufferSize) ? bufferSize : availableBytes;
            size_t bytesRead = stream->readBytes(buffer, bytesToRead);

            if (bytesRead > 0) {
                size_t written = Update.write(buffer, bytesRead);
                if (written != bytesRead) {
                    snprintf(s_errorMsg, sizeof(s_errorMsg), "Ghi Flash thất bại: %s", Update.errorString());
                    LOG_E(TAG, "%s", s_errorMsg);
                    streamError = true;
                    break;
                }

                totalDownloaded += written;
                lastActivity = millis();

                s_downloadedBytes = totalDownloaded;
                s_totalBytes = contentLength;

                int currentPercent = (int)((totalDownloaded * 100) / contentLength);
                if (currentPercent != lastPercent) {
                    lastPercent = currentPercent;
                    s_progressPercent = currentPercent;
                    snprintf(s_statusMsg, sizeof(s_statusMsg), "Đang nạp: %d%% (%u / %u KB)", currentPercent, (uint32_t)(totalDownloaded / 1024), (uint32_t)(contentLength / 1024));
                    if (s_progressCb) {
                        s_progressCb(currentPercent, totalDownloaded, contentLength);
                    }
                }
            }
        } else {
            // Kiểm tra timeout nếu stream ngừng phản hồi quá 10s
            if (millis() - lastActivity > 10000) {
                snprintf(s_errorMsg, sizeof(s_errorMsg), "Mất kết nối mạng khi đang nạp OTA.");
                LOG_E(TAG, "%s", s_errorMsg);
                streamError = true;
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        // Nhường 1 tick cho CPU Core 0 reset Watchdog (Rule 8)
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    free(buffer);
    http.end();

    if (streamError || totalDownloaded < (size_t)contentLength) {
        Update.abort();
        s_state = OtaState::ERROR;
        s_isUpdating = false;
        if (s_statusCb) s_statusCb(OtaState::ERROR, s_errorMsg);
        vTaskDelete(NULL);
        return;
    }

    if (Update.end(true)) {
        if (Update.isFinished()) {
            LOG_I(TAG, "OTA update SUCCESSFUL! (%u bytes written)", totalDownloaded);
            
            if (s_clearNvs) {
                LOG_W(TAG, "clearNvs flag is TRUE -> Resetting NVS Flash to factory defaults before reboot...");
                ConfigManager::resetToDefaults();
                vTaskDelay(pdMS_TO_TICKS(500));
            }

            s_state = OtaState::SUCCESS;
            s_isUpdating = false;
            if (s_statusCb) s_statusCb(OtaState::SUCCESS, "Cập nhật thành công! Đang khởi động lại...");
            
            // Chờ 1.5s để UI hiển thị thông báo trước khi reboot
            vTaskDelay(pdMS_TO_TICKS(1500));
            ESP.restart();
        } else {
            snprintf(s_errorMsg, sizeof(s_errorMsg), "OTA chưa hoàn tất toàn vẹn.");
            LOG_E(TAG, "%s", s_errorMsg);
            s_state = OtaState::ERROR;
            s_isUpdating = false;
            if (s_statusCb) s_statusCb(OtaState::ERROR, s_errorMsg);
        }
    } else {
        snprintf(s_errorMsg, sizeof(s_errorMsg), "Lỗi kết thúc OTA: %s", Update.errorString());
        LOG_E(TAG, "%s", s_errorMsg);
        s_state = OtaState::ERROR;
        s_isUpdating = false;
        if (s_statusCb) s_statusCb(OtaState::ERROR, s_errorMsg);
    }

    vTaskDelete(NULL);
}
