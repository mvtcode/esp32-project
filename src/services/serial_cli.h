#pragma once

#include <Arduino.h>
#include "ble_hid_service.h"

class SerialCli {
public:
  SerialCli(BleHidService *hidService);
  ~SerialCli();

  // Khởi tạo Serial CLI
  void begin();

  // Kiểm tra và xử lý buffer Serial không chặn (gọi trong loop)
  void update();

  // Hiển thị danh sách câu lệnh trợ giúp
  void printHelp();

  // Hiển thị trạng thái hệ thống
  void printStatus();

private:
  BleHidService *_hidService;
  String _inputBuffer;

  void processCommand(const String &cmdLine);
  void handleMouseCommand(const String &args);
  void handleClickCommand(const String &args);
  void handleTypeCommand(const String &args);
  void handlePressCommand(const String &args);
  void handleShortcutCommand(const String &args);
  void handleMediaCommand(const String &args);
};
