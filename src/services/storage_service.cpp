#include "storage_service.h"
#include "log.h"

static const char *TAG = "StorageService";

bool StorageService::s_mounted = false;
#if !defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(ARDUINO_ARCH_ESP32S3) && !defined(BOARD_ESP32S3)
SPIClass* StorageService::s_sdSPI = nullptr;
#endif
SemaphoreHandle_t StorageService::s_sdMutex = nullptr;

bool StorageService::begin() {
    if (s_mounted) {
        if (STORAGE_FS.cardType() != CARD_NONE) return true;
        s_mounted = false;
    }

    if (!s_sdMutex) {
        s_sdMutex = xSemaphoreCreateMutex();
    }

#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_ARCH_ESP32S3) || defined(BOARD_ESP32S3)
    // ESP32-S3 sử dụng giao tiếp SDIO/SD_MMC (4-bit hoặc 1-bit)
    LOG_I(TAG, "Khởi tạo SD_MMC (CLK=%d, CMD=%d, D0=%d, D1=%d, D2=%d, D3=%d)...",
          PIN_SD_CLK, PIN_SD_CMD, PIN_SD_D0, PIN_SD_D1, PIN_SD_D2, PIN_SD_D3);
    
    SD_MMC.setPins(PIN_SD_CLK, PIN_SD_CMD, PIN_SD_D0, PIN_SD_D1, PIN_SD_D2, PIN_SD_D3);

    // Thử mount ở chế độ 4-bit High Speed 40MHz trước
    if (!SD_MMC.begin("/sd", false, false, SDMMC_FREQ_HIGHSPEED)) {
        LOG_W(TAG, "HS 40MHz thất bại, thử 4-bit Default 20MHz...");
        if (!SD_MMC.begin("/sd", false, false, SDMMC_FREQ_DEFAULT)) {
            LOG_W(TAG, "Thử lại SD_MMC ở chế độ 1-bit...");
            if (!SD_MMC.begin("/sd", true, false, SDMMC_FREQ_DEFAULT)) {
                LOG_E(TAG, "Không thể kết nối thẻ MicroSD qua SD_MMC!");
                s_mounted = false;
                return false;
            }
        }
    }
#else
    // ESP32 CYD sử dụng giao tiếp SPI
    if (!s_sdSPI) {
        s_sdSPI = new SPIClass(VSPI);
        s_sdSPI->begin(PIN_SD_SCLK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
    }

    pinMode(PIN_SD_CS, OUTPUT);
    digitalWrite(PIN_SD_CS, HIGH);
    delay(20);

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
#endif

    uint8_t cardType = STORAGE_FS.cardType();
    if (cardType == CARD_NONE) {
        LOG_W(TAG, "Không tìm thấy thẻ nhớ trong khe cắm.");
        s_mounted = false;
        return false;
    }

    uint64_t totalBytes = STORAGE_FS.totalBytes();
    uint64_t cardSize = STORAGE_FS.cardSize();
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
        STORAGE_FS.end();
        s_mounted = false;
    }
#if !defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(ARDUINO_ARCH_ESP32S3) && !defined(BOARD_ESP32S3)
    if (s_sdSPI) {
        delete s_sdSPI;
        s_sdSPI = nullptr;
    }
#endif
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
    bool exists = STORAGE_FS.exists(path);
    unlock();
    return exists;
}

uint32_t StorageService::getFileSize(const char* path) {
    if (!s_mounted || !path) return 0;
    if (!lock()) return 0;
    uint32_t size = 0;
    File f = STORAGE_FS.open(path, FILE_READ);
    if (f) {
        size = f.size();
        f.close();
    }
    unlock();
    return size;
}
