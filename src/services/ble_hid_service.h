#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

// Các cờ chuột (Mouse buttons)
#define MOUSE_BUTTON_LEFT   0x01
#define MOUSE_BUTTON_RIGHT  0x02
#define MOUSE_BUTTON_MIDDLE 0x04

// Cờ phím bổ trợ bàn phím (Keyboard modifier flags)
#define KEY_MOD_NONE        0x00
#define KEY_MOD_LCTRL       0x01
#define KEY_MOD_LSHIFT      0x02
#define KEY_MOD_LALT        0x04
#define KEY_MOD_LMETA       0x08  // Windows Key / Command Key
#define KEY_MOD_RCTRL       0x10
#define KEY_MOD_RSHIFT      0x20
#define KEY_MOD_RALT        0x40
#define KEY_MOD_RMETA       0x80

// HID Keycodes phổ biến
#define HID_KEY_NONE        0x00
#define HID_KEY_A           0x04
#define HID_KEY_B           0x05
#define HID_KEY_C           0x06
#define HID_KEY_D           0x07
#define HID_KEY_E           0x08
#define HID_KEY_F           0x09
#define HID_KEY_G           0x0A
#define HID_KEY_H           0x0B
#define HID_KEY_I           0x0C
#define HID_KEY_J           0x0D
#define HID_KEY_K           0x0E
#define HID_KEY_L           0x0F
#define HID_KEY_M           0x10
#define HID_KEY_N           0x11
#define HID_KEY_O           0x12
#define HID_KEY_P           0x13
#define HID_KEY_Q           0x14
#define HID_KEY_R           0x15
#define HID_KEY_S           0x16
#define HID_KEY_T           0x17
#define HID_KEY_U           0x18
#define HID_KEY_V           0x19
#define HID_KEY_W           0x1A
#define HID_KEY_X           0x1B
#define HID_KEY_Y           0x1C
#define HID_KEY_Z           0x1D
#define HID_KEY_1           0x1E
#define HID_KEY_2           0x1F
#define HID_KEY_3           0x20
#define HID_KEY_4           0x21
#define HID_KEY_5           0x22
#define HID_KEY_6           0x23
#define HID_KEY_7           0x24
#define HID_KEY_8           0x25
#define HID_KEY_9           0x26
#define HID_KEY_0           0x27
#define HID_KEY_ENTER       0x28
#define HID_KEY_ESC         0x29
#define HID_KEY_BACKSPACE   0x2A
#define HID_KEY_TAB         0x2B
#define HID_KEY_SPACE       0x2C
#define HID_KEY_MINUS       0x2D
#define HID_KEY_EQUAL       0x2E
#define HID_KEY_LEFTBRACE   0x2F
#define HID_KEY_RIGHTBRACE  0x30
#define HID_KEY_BACKSLASH   0x31
#define HID_KEY_SEMICOLON   0x33
#define HID_KEY_QUOTE       0x34
#define HID_KEY_TILDE       0x35
#define HID_KEY_COMMA       0x36
#define HID_KEY_PERIOD      0x37
#define HID_KEY_SLASH       0x38
#define HID_KEY_CAPSLOCK    0x39
#define HID_KEY_F1          0x3A
#define HID_KEY_F2          0x3B
#define HID_KEY_F3          0x3C
#define HID_KEY_F4          0x3D
#define HID_KEY_F5          0x3E
#define HID_KEY_F6          0x3F
#define HID_KEY_F7          0x40
#define HID_KEY_F8          0x41
#define HID_KEY_F9          0x42
#define HID_KEY_F10         0x43
#define HID_KEY_F11         0x44
#define HID_KEY_F12         0x45
#define HID_KEY_PRINTSCREEN 0x46
#define HID_KEY_SCROLLLOCK  0x47
#define HID_KEY_PAUSE       0x48
#define HID_KEY_INSERT      0x49
#define HID_KEY_HOME        0x4A
#define HID_KEY_PAGEUP      0x4B
#define HID_KEY_DELETE      0x4C
#define HID_KEY_END         0x4D
#define HID_KEY_PAGEDOWN    0x4E
#define HID_KEY_RIGHT       0x4F
#define HID_KEY_LEFT        0x50
#define HID_KEY_DOWN        0x51
#define HID_KEY_UP          0x52

// Consumer Control (Media Keys) bitmasks
#define MEDIA_KEY_NEXT      (1 << 0)
#define MEDIA_KEY_PREV      (1 << 1)
#define MEDIA_KEY_STOP      (1 << 2)
#define MEDIA_KEY_PLAY_PAUSE (1 << 3)
#define MEDIA_KEY_MUTE      (1 << 4)
#define MEDIA_KEY_VOL_UP    (1 << 5)
#define MEDIA_KEY_VOL_DOWN  (1 << 6)
#define MEDIA_KEY_HOME      (1 << 7)
#define MEDIA_KEY_CALCULATOR (1 << 9)

enum class BleHidCommandType : uint8_t {
  MOUSE_MOVE,
  MOUSE_BUTTONS,
  MOUSE_CLICK,
  KEYBOARD_PRESS,
  KEYBOARD_RELEASE,
  KEYBOARD_RELEASE_ALL,
  KEYBOARD_WRITE,
  CONSUMER_PRESS,
  CONSUMER_RELEASE,
  CONSUMER_WRITE
};

struct BleHidCommand {
  BleHidCommandType type;
  union {
    struct {
      int8_t dx;
      int8_t dy;
      int8_t wheel;
    } mouseMove;
    struct {
      uint8_t buttons;
    } mouseButton;
    struct {
      uint8_t keycode;
      uint8_t modifiers;
    } keyboard;
    struct {
      uint16_t mediaMask;
    } consumer;
  } data;
};

// Forward declaration của NimBLE classes
class NimBLEServer;
class NimBLEHIDDevice;
class NimBLECharacteristic;

// UUID cho Custom Volume GATT Service
#define VOLUME_SERVICE_UUID        "FFE0"
#define VOLUME_CHARACTERISTIC_UUID "FFE1"

typedef void (*VolumeChangeCallback)(uint8_t volumePercent);

class BleHidService {
public:
  // Quản lý vòng đời bộ nhớ (RAII)
  BleHidService();
  ~BleHidService();

  // Khởi động dịch vụ BLE HID
  bool begin(const char *deviceName = "ESP32-S3 Smart TouchPad");

  // Dừng dịch vụ và giải phóng tài nguyên
  void end();

  // Trạng thái kết nối
  bool isConnected() const;
  String getConnectedDeviceName() const;
  uint8_t getBatteryLevel() const;
  void setBatteryLevel(uint8_t level);

  // --- API Chuột (Mouse) ---
  void mouseMove(int8_t dx, int8_t dy, int8_t wheel = 0);
  void mousePress(uint8_t buttons);
  void mouseRelease(uint8_t buttons);
  void mouseClick(uint8_t buttons = MOUSE_BUTTON_LEFT);

  // --- API Bàn phím (Keyboard) ---
  void keyboardPress(uint8_t keycode, uint8_t modifiers = KEY_MOD_NONE);
  void keyboardRelease(uint8_t keycode);
  void keyboardReleaseAll();
  void keyboardWrite(uint8_t keycode, uint8_t modifiers = KEY_MOD_NONE);
  void keyboardPrint(const char *text);

  // --- API Đa phương tiện (Consumer Control) ---
  void mediaKeyPress(uint16_t mediaMask);
  void mediaKeyRelease();
  void mediaKeyWrite(uint16_t mediaMask);

  // --- API Đồng Bộ Âm Lượng Hai Chiều (Bi-directional Volume Sync) ---
  void setRemoteVolume(uint8_t volumePercent);
  uint8_t getRemoteVolume() const { return _remoteVolume; }
  void setVolumeChangeCallback(VolumeChangeCallback cb) { _volChangeCb = cb; }

  // Singleton instance truy cập toàn cục an toàn
  static BleHidService *getInstance();

private:
  static BleHidService *s_instance;

  // Thành viên BLE NimBLE
  NimBLEServer *_server;
  NimBLEHIDDevice *_hidDevice;
  NimBLECharacteristic *_inputKeyboard;
  NimBLECharacteristic *_outputKeyboard;
  NimBLECharacteristic *_inputMouse;
  NimBLECharacteristic *_inputConsumer;
  NimBLECharacteristic *_volCharacteristic;

  // FreeRTOS Task và Queue trên Core 0
  TaskHandle_t _taskHandle;
  QueueHandle_t _cmdQueue;

  // Trạng thái nội bộ
  bool _connected;
  String _connectedDeviceName;
  uint8_t _batteryLevel;
  uint8_t _currentMouseButtons;
  uint8_t _keyboardModifiers;
  uint8_t _keyboardKeys[6];
  uint16_t _consumerMask;
  uint8_t _remoteVolume;
  VolumeChangeCallback _volChangeCb;

  // Task xử lý HID trên Core 0
  static void bleHidTask(void *param);

  // Đẩy lệnh vào Queue an toàn luồng
  bool sendCommand(const BleHidCommand &cmd);

  // Gửi trực tiếp report qua BLE Characteristic (chỉ gọi trong task Core 0)
  void sendKeyboardReport();
  void sendMouseReport(int8_t dx, int8_t dy, int8_t wheel);
  void sendConsumerReport();

  // Callbacks
  friend class BleHidServerCallbacks;
  friend class VolumeCharacteristicCallbacks;
};
