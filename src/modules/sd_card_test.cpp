#include "sd_card_test.h"
#include "log.h"

static const char *TAG = "SdCardTest";

SdCardTest::SdCardTest()
  : _spi(nullptr), _mounted(false), _mode(SdMode::NONE), _fs(nullptr) {}

SdCardTest::~SdCardTest() {
  if (_mounted) {
    if (_mode == SdMode::SD_MMC_1BIT) {
      SD_MMC.end();
    } else if (_mode == SdMode::SD_SPI) {
      SD.end();
    }
    _mounted = false;
  }
  if (_spi) {
    delete _spi;
    _spi = nullptr;
  }
}

bool SdCardTest::begin() {
  // 1. Thử chế độ SD_MMC 1-bit native trên ESP32-S3 (CLK=12, CMD=11, D0=13)
  LOG_I(TAG, "Thu mount the nho bang SD_MMC (1-bit) tai CLK=%d, CMD=%d, D0=%d...",
        PIN_SD_SCK, PIN_SD_MOSI, PIN_SD_MISO);

  pinMode(PIN_SD_MOSI, INPUT_PULLUP);
  pinMode(PIN_SD_MISO, INPUT_PULLUP);

  SD_MMC.setPins(PIN_SD_SCK, PIN_SD_MOSI, PIN_SD_MISO);
  if (SD_MMC.begin("/sdcard", true /* 1-bit mode */, false /* no format */, 20000)) {
    _mounted = true;
    _mode = SdMode::SD_MMC_1BIT;
    _fs = &SD_MMC;
    LOG_I(TAG, "-> Mount the nho thanh cong qua SD_MMC (1-bit Mode)!");
    return true;
  }

  LOG_W(TAG, "SD_MMC that bai, thu tiep phuong thuc SPI qua HSPI (SPI3_HOST)...");
  SD_MMC.end();

  // 2. Thử chế độ SPI truyền thống qua HSPI (CS=10, SCK=12, MOSI=11, MISO=13)
  pinMode(PIN_SD_CS, OUTPUT);
  digitalWrite(PIN_SD_CS, HIGH);

  if (_spi == nullptr) {
    _spi = new SPIClass(HSPI);
    _spi->begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  }

  // Thử tần số 4MHz để đạt độ tương thích cao nhất
  if (SD.begin(PIN_SD_CS, *_spi, 4000000)) {
    _mounted = true;
    _mode = SdMode::SD_SPI;
    _fs = &SD;
    LOG_I(TAG, "-> Mount the nho thanh cong qua SPI (CS=%d)!", PIN_SD_CS);
    return true;
  }

  SD.end();
  LOG_W(TAG, "Chua phat hien the nho MicroSD hoac mount that bai ca 2 che do SD_MMC va SPI.");
  return false;
}

SdCardStatus SdCardTest::runDiagnostic() {
  SdCardStatus status;
  status.mounted = false;
  status.totalBytes = 0;
  status.usedBytes = 0;
  status.cardType = "Unknown";
  status.readWriteOk = false;
  status.message = "Chua cam the";

  if (!_mounted) {
    if (!begin()) {
      status.message = "Khong tim thay the SD";
      return status;
    }
  }

  sdcard_type_t type = (_mode == SdMode::SD_MMC_1BIT) ? SD_MMC.cardType() : SD.cardType();
  if (type == CARD_NONE) {
    status.message = "Khong co the trong khe";
    LOG_W(TAG, "SD Card Slot trong");
    return status;
  }

  status.mounted = true;
  switch (type) {
    case CARD_MMC:  status.cardType = "MMC"; break;
    case CARD_SD:   status.cardType = "SDSC"; break;
    case CARD_SDHC: status.cardType = "SDHC/SDXC"; break;
    default:        status.cardType = "UNKNOWN"; break;
  }

  status.totalBytes = (_mode == SdMode::SD_MMC_1BIT) ? SD_MMC.totalBytes() : SD.totalBytes();
  status.usedBytes  = (_mode == SdMode::SD_MMC_1BIT) ? SD_MMC.usedBytes()  : SD.usedBytes();

  LOG_I(TAG, "Che do: %s | Loai: %s | Dung luong: %llu MB | Da dung: %llu MB",
        (_mode == SdMode::SD_MMC_1BIT ? "SD_MMC (1-bit)" : "SPI"),
        status.cardType.c_str(),
        status.totalBytes / (1024 * 1024),
        status.usedBytes / (1024 * 1024));

  if (_fs == nullptr) {
    status.message = "FS nullptr";
    return status;
  }

  // Kiem tra doc ghi file va xac thuc magic bytes (Tuan thu Rule 6 & Rule 10)
  const char* testPath = "/sd_diag.bin";
  File fWrite = _fs->open(testPath, FILE_WRITE);
  if (!fWrite) {
    status.message = "Loi tao file test";
    LOG_E(TAG, "Khong the mo file de ghi: %s", testPath);
    return status;
  }

  uint32_t magicHeader = TEST_MAGIC_HEADER;
  const char* testPayload = "ESP32S3_SD_OK";
  fWrite.write((const uint8_t*)&magicHeader, sizeof(magicHeader));
  fWrite.write((const uint8_t*)testPayload, strlen(testPayload));
  fWrite.flush();
  fWrite.close();

  // Doc lai va kiem tra Magic Bytes
  File fRead = _fs->open(testPath, FILE_READ);
  if (!fRead) {
    status.message = "Loi doc file test";
    LOG_E(TAG, "Khong the mo file de doc lai: %s", testPath);
    return status;
  }

  uint32_t readMagic = 0;
  if (fRead.read((uint8_t*)&readMagic, sizeof(readMagic)) != sizeof(readMagic)) {
    status.message = "Loi doc header";
    LOG_E(TAG, "Khong the doc Magic Header tu file test");
    fRead.close();
    return status;
  }

  if (readMagic != TEST_MAGIC_HEADER) {
    status.message = "Sai Magic Bytes";
    LOG_E(TAG, "Xac thuc Header that bai: 0x%08X != 0x%08X", readMagic, TEST_MAGIC_HEADER);
    fRead.close();
    return status;
  }

  char buf[32] = {0};
  int bytesRead = fRead.read((uint8_t*)buf, sizeof(buf) - 1);
  fRead.close();

  if (bytesRead > 0 && strcmp(buf, testPayload) == 0) {
    status.readWriteOk = true;
    String modeName = (_mode == SdMode::SD_MMC_1BIT) ? "MMC" : "SPI";
    status.message = status.cardType + " (" + modeName + ") RW OK";
    LOG_I(TAG, "Doc ghi the SD & Xac thuc Magic Header: HOAN HAO!");
  } else {
    status.message = "Loi du lieu payload";
    LOG_E(TAG, "Noi dung payload khong khop");
  }

  return status;
}
