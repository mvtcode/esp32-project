#pragma once

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include "pin_config.h"


struct StorageInfo {
    bool isMounted;
    const char* cardType;
    uint64_t totalBytes;
    uint64_t usedBytes;
    uint64_t freeBytes;
};

class StorageService {
public:
    static bool init();
    static bool isMounted();
    static StorageInfo getInfo();
    static bool formatCard();
    static SPIClass* getSPI();

private:
    static bool mounted;
    static SPIClass* sdSPI;
};

