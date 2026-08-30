#ifndef STORAGE_SERVICE_H
#define STORAGE_SERVICE_H

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

// ESP32 CYD 3.5" SD Card Pinout (Dedicated VSPI bus)
#define SD_CS_PIN    5
#define SD_MOSI_PIN  23
#define SD_MISO_PIN  19
#define SD_SCLK_PIN  18

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

#endif // STORAGE_SERVICE_H
