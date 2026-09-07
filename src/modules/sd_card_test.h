#pragma once
#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SD_MMC.h>
#include <SPI.h>
#include "pin_config.h"

enum class SdMode {
  NONE,
  SD_MMC_1BIT,
  SD_SPI
};

struct SdCardStatus {
  bool mounted;
  uint64_t totalBytes;
  uint64_t usedBytes;
  String cardType;
  bool readWriteOk;
  String message;
};

class SdCardTest {
public:
  SdCardTest();
  ~SdCardTest();

  bool begin();
  SdCardStatus runDiagnostic();

private:
  SPIClass* _spi;
  bool _mounted;
  SdMode _mode;
  fs::FS* _fs;
  static const uint32_t TEST_MAGIC_HEADER = 0x53445453; // 'SDTS' (SD Test Signature)
};
