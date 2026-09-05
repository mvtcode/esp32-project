#pragma once

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
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

private:
    static bool s_mounted;
    static SPIClass* s_sdSPI;
    static SemaphoreHandle_t s_sdMutex;
};
