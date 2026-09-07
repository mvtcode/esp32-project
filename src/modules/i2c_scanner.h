#pragma once
#include <Arduino.h>
#include <vector>

class I2cScanner {
public:
  I2cScanner(int sdaPin, int sclPin);
  ~I2cScanner();

  bool begin();
  std::vector<uint8_t> scan();
  const char* identifyDevice(uint8_t address) const;

private:
  int _sda;
  int _scl;
  bool _initialized;
};
