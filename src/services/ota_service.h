#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <functional>

#include "version.h"

struct OtaInfo {
    bool hasUpdate = false;
    String target;
    String hardware;
    String version;
    int versionCode = 0;
    String releaseDate;
    String firmwareUrl;
    bool clearNvs = false;
    String clearNvsBelow;
    String changelog;
    String author;
    String email;
    String project;
};


enum class OtaState {
    IDLE,
    CHECKING,
    UPDATE_AVAILABLE,
    UP_TO_DATE,
    DOWNLOADING,
    SUCCESS,
    ERROR
};

typedef std::function<void(int percent, size_t downloadedBytes, size_t totalBytes)> OtaProgressCallback;
typedef std::function<void(OtaState state, const char* message)> OtaStatusCallback;

class OtaService {
public:
    static void init();

    /**
     * @brief Kiểm tra phiên bản mới từ version.json qua OTA Manifest URL
     * @param info Cấu trúc nhận thông tin phiên bản mới
     * @return true nếu gọi API kiểm tra thành công
     */
    static bool checkUpdate(OtaInfo& info);

    /**
     * @brief Bắt đầu quá trình tải và ghi firmware OTA trên Task riêng
     * @param firmwareUrl Đường dẫn tải file firmware.bin
     * @param clearNvs Xóa sạch NVS Flash (reset gốc) sau khi nạp thành công
     * @param progressCb Callback báo tiến trình tải %
     * @param statusCb Callback báo trạng thái (hoàn thành hoặc lỗi)
     * @return true nếu bắt đầu thành công
     */
    static bool startUpdate(const String& firmwareUrl, bool clearNvs, const String& changelog, const String& releaseDate, OtaProgressCallback progressCb, OtaStatusCallback statusCb);
    static bool startUpdate(const String& firmwareUrl, bool clearNvs, OtaProgressCallback progressCb, OtaStatusCallback statusCb) {
        return startUpdate(firmwareUrl, clearNvs, "", "", progressCb, statusCb);
    }
    static bool startUpdate(const String& firmwareUrl, OtaProgressCallback progressCb, OtaStatusCallback statusCb) {
        return startUpdate(firmwareUrl, false, "", "", progressCb, statusCb);
    }

    static bool isUpdating();
    static OtaState getState();
    static int getProgressPercent();
    static size_t getDownloadedBytes();
    static size_t getTotalBytes();
    static const char* getStatusMessage();
    static const char* getErrorMessage();

    /**
     * @brief Quy đổi chuỗi phiên bản dạng vX.Y.Z thành số nguyên để so sánh (Major*10^6 + Minor*10^3 + Patch)
     */
    static uint32_t parseVersion(const String& verStr);


private:
    static OtaState s_state;
    static char s_errorMsg[128];
    static char s_statusMsg[128];
    static volatile int s_progressPercent;
    static volatile size_t s_downloadedBytes;
    static volatile size_t s_totalBytes;
    static bool s_isUpdating;
    static bool s_clearNvs;
    static TaskHandle_t s_otaTaskHandle;

    static OtaProgressCallback s_progressCb;
    static OtaStatusCallback s_statusCb;
    static String s_downloadUrl;
    static String s_pendingChangelog;
    static String s_pendingReleaseDate;

    static void otaTask(void* param);
    static String extractJsonString(const String& json, const char* key);
    static int extractJsonInt(const String& json, const char* key);
    static bool extractJsonBool(const String& json, const char* key);
};

