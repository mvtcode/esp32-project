#include "i2c_scanner.h"
#include <Wire.h>
#include "log.h"

static const char *TAG = "I2cScanner";

I2cScanner::I2cScanner(int sdaPin, int sclPin)
  : _sda(sdaPin), _scl(sclPin), _initialized(false) {}

I2cScanner::~I2cScanner() {
  if (_initialized) {
    Wire.end();
    _initialized = false;
  }
}

bool I2cScanner::begin() {
  LOG_I(TAG, "Khoi tao I2C Bus tai SDA=%d, SCL=%d", _sda, _scl);
  bool success = Wire.begin(_sda, _scl, 100000);
  _initialized = success;
  if (!success) {
    LOG_E(TAG, "Loi khoi tao I2C!");
  }
  return success;
}

std::vector<uint8_t> I2cScanner::scan() {
  std::vector<uint8_t> devices;
  if (!_initialized) {
    if (!begin()) return devices;
  }

  LOG_I(TAG, "Bat dau quet cac thiet bi I2C...");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      devices.push_back(addr);
      LOG_I(TAG, "-> Phat hien thiet bi tai 0x%02X (%s)", addr, identifyDevice(addr));
    }
  }

  if (devices.empty()) {
    LOG_W(TAG, "Khong tim thay thiet bi I2C nao tren bus SDA=%d, SCL=%d", _sda, _scl);
  } else {
    LOG_I(TAG, "Hoan tat quet. Tong cong: %d thiet bi.", (int)devices.size());
  }

  return devices;
}

const char* I2cScanner::identifyDevice(uint8_t address) const {
  switch (address) {
    case 0x14:
    case 0x5D:
      return "Goodix GT911 / AXS15231B Touch IC";
    case 0x38:
      return "FocalTech FT6236/FT6336U Touch IC";
    case 0x75:
      return "IP5306 Power Management IC";
    case 0x34:
      return "AXP192/AXP2101 Power Management";
    case 0x10:
    case 0x11:
      return "ES8388 Audio Codec";
    case 0x68:
      return "MPU6050 / DS3231 RTC";
    case 0x51:
      return "PCF8563 RTC";
    default:
      return "Thiet bi ngoai vi khac";
  }
}
