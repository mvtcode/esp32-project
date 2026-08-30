#include "storage_service.h"

bool StorageService::mounted = false;
SPIClass* StorageService::sdSPI = nullptr;

bool StorageService::init() {
    // Already mounted and verified — do not re-init to avoid SPI conflict with TFT
    if (mounted) {
        if (SD.cardType() != CARD_NONE) return true;
        mounted = false;
    }

    if (!sdSPI) {
        sdSPI = new SPIClass(VSPI);
        sdSPI->begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    }

    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    delay(50);

    // Thử kết nối ở tốc độ 10MHz (chuẩn ổn định cho ESP32 SD SPI), nếu không được thì thử lại 4MHz
    if (!SD.begin(SD_CS_PIN, *sdSPI, 10000000, "/sd", 8)) {
        if (!SD.begin(SD_CS_PIN, *sdSPI, 4000000, "/sd", 8)) {
            Serial.println("[Storage] SD Card Mount Failed or Not Inserted.");
            mounted = false;
            return false;
        }
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("[Storage] No SD card attached.");
        mounted = false;
        return false;
    }

    uint64_t totalBytes = SD.totalBytes();
    uint64_t usedBytes = SD.usedBytes();
    uint64_t cardSize = SD.cardSize();

    mounted = true;
    Serial.printf("[Storage] SD Card Mounted. Card Size: %llu MB | FATFS Total: %llu MB, Used: %llu MB\n", 
                  (uint64_t)(cardSize / (1024 * 1024)),
                  (uint64_t)(totalBytes / (1024 * 1024)),
                  (uint64_t)(usedBytes / (1024 * 1024)));

    if (totalBytes == 0) {
        Serial.println("[Storage] WARNING: FATFS reported 0 total bytes! Card may be formatted as exFAT or NTFS. ESP32 requires FAT32 / FAT16 format.");
    }
    return true;
}

bool StorageService::isMounted() {
    // Trust mounted flag — do NOT re-check SD.cardType() here because it
    // requires SPI access and will conflict if TFT is using the bus
    if (!mounted) {
        init();
    }
    return mounted;
}

StorageInfo StorageService::getInfo() {
    StorageInfo info;
    info.isMounted = isMounted();
    info.cardType = "Chưa cắm thẻ";
    info.totalBytes = 0;
    info.usedBytes = 0;
    info.freeBytes = 0;

    if (!info.isMounted) {
        return info;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_MMC) {
        info.cardType = "MMC";
    } else if (cardType == CARD_SD) {
        info.cardType = "SDSC";
    } else if (cardType == CARD_SDHC) {
        info.cardType = "SDHC/XC";
    } else {
        info.cardType = "MicroSD";
    }

    // 1. Lấy dung lượng từ filesystem FATFS
    info.totalBytes = SD.totalBytes();
    info.usedBytes = SD.usedBytes();

    // 2. Dự phòng: Nếu FATFS trả về 0 (ví dụ chưa định dạng hoặc exFAT), lấy trực tiếp từ phần cứng SD.cardSize()
    if (info.totalBytes == 0) {
        info.totalBytes = SD.cardSize();
    }

    if (info.totalBytes >= info.usedBytes) {
        info.freeBytes = info.totalBytes - info.usedBytes;
    } else {
        info.freeBytes = info.totalBytes;
    }

    return info;
}

bool StorageService::formatCard() {
    if (!isMounted()) return false;
    Serial.println("[Storage] Starting SD card clean...");
    
    File root = SD.open("/");
    if (!root) return false;

    File file = root.openNextFile();
    while (file) {
        String path = file.name();
        bool isDir = file.isDirectory();
        file.close();
        
        if (isDir) {
            SD.rmdir(path.c_str());
        } else {
            SD.remove(path.c_str());
        }
        file = root.openNextFile();
    }
    root.close();

    Serial.println("[Storage] SD Card format complete.");
    return true;
}

SPIClass* StorageService::getSPI() {
    return sdSPI;
}
