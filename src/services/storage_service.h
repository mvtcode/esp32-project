#pragma once

#include <Arduino.h>
#include <FS.h>
#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_ARCH_ESP32S3) || defined(BOARD_ESP32S3)
#include <SD_MMC.h>
#define STORAGE_FS SD_MMC
#else
#include <SD.h>
#include <SPI.h>
#define STORAGE_FS SD
#endif
#include "pin_config.h"

class StorageService {
public:
    static bool begin();
    static bool isMounted();
    static void end();

    // Mutex cho truy xuất thẻ nhớ đa luồng (Audio + Video)
    static bool lock(TickType_t waitTicks = pdMS_TO_TICKS(100));
    static void unlock();

    static bool fileExists(const char* path);
    static uint32_t getFileSize(const char* path);
    static File openFile(const char* path, const char* mode = FILE_READ) {
        return STORAGE_FS.open(path, mode);
    }

private:
    static bool s_mounted;
#if !defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(ARDUINO_ARCH_ESP32S3) && !defined(BOARD_ESP32S3)
    static SPIClass* s_sdSPI;
#endif
    static SemaphoreHandle_t s_sdMutex;
};
