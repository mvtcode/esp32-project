#include "storage_service.h"
#include "log.h"

static const char *TAG = "StorageService";

bool StorageService::s_mounted = false;
SPIClass* StorageService::s_sdSPI = nullptr;
SemaphoreHandle_t StorageService::s_sdMutex = nullptr;

bool StorageService::begin() {
    if (s_mounted) {
        if (SD.cardType() != CARD_NONE) return true;
        s_mounted = false;
    }

    if (!s_sdMutex) {
        s_sdMutex = xSemaphoreCreateMutex();
    }

    if (!s_sdSPI) {
        s_sdSPI = new SPIClass(VSPI);
        s_sdSPI->begin(PIN_SD_SCLK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
    }

    pinMode(PIN_SD_CS, OUTPUT);
    digitalWrite(PIN_SD_CS, HIGH);
    delay(20);

    // Khởi tạo SD với tần số 25MHz cho tốc độ đọc tối ưu, dự phòng 20MHz/10MHz
    if (!SD.begin(PIN_SD_CS, *s_sdSPI, 25000000, "/sd", 8)) {
        LOG_W(TAG, "Thử lại SD với tần số 20MHz...");
        digitalWrite(PIN_SD_CS, HIGH);
        delay(50);
        if (!SD.begin(PIN_SD_CS, *s_sdSPI, 20000000, "/sd", 8)) {
            LOG_E(TAG, "Không thể kết nối thẻ MicroSD!");
            s_mounted = false;
            return false;
        }
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        LOG_W(TAG, "Không tìm thấy thẻ nhớ trong khe cắm.");
        s_mounted = false;
        return false;
    }

    uint64_t totalBytes = SD.totalBytes();
    uint64_t cardSize = SD.cardSize();
    s_mounted = true;

    LOG_I(TAG, "Thẻ nhớ đã mount thành công: Dung lượng = %llu MB | FATFS = %llu MB",
          (uint64_t)(cardSize / (1024 * 1024)),
          (uint64_t)(totalBytes / (1024 * 1024)));

    return true;
}

bool StorageService::isMounted() {
    return s_mounted;
}

void StorageService::end() {
    if (s_mounted) {
        SD.end();
        s_mounted = false;
    }
    if (s_sdSPI) {
        delete s_sdSPI;
        s_sdSPI = nullptr;
    }
    if (s_sdMutex) {
        vSemaphoreDelete(s_sdMutex);
        s_sdMutex = nullptr;
    }
    LOG_I(TAG, "Đã giải phóng tài nguyên StorageService");
}

bool StorageService::lock(TickType_t waitTicks) {
    if (!s_sdMutex) return false;
    return (xSemaphoreTake(s_sdMutex, waitTicks) == pdTRUE);
}

void StorageService::unlock() {
    if (s_sdMutex) {
        xSemaphoreGive(s_sdMutex);
    }
}

bool StorageService::fileExists(const char* path) {
    if (!s_mounted || !path) return false;
    if (!lock()) return false;
    bool exists = SD.exists(path);
    unlock();
    return exists;
}

uint32_t StorageService::getFileSize(const char* path) {
    if (!s_mounted || !path) return 0;
    if (!lock()) return 0;
    uint32_t size = 0;
    File f = SD.open(path, FILE_READ);
    if (f) {
        size = f.size();
        f.close();
    }
    unlock();
    return size;
}
