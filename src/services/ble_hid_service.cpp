#include "ble_hid_service.h"
#include "log.h"
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <NimBLEHIDDevice.h>

static const char *TAG = "BleHidService";

// Report IDs trong Composite HID Map
#define REPORT_ID_KEYBOARD  0x01
#define REPORT_ID_CONSUMER  0x02
#define REPORT_ID_MOUSE     0x03

// Bảng mô tả HID Report Descriptor chuẩn đa thiết bị (Composite HID)
static const uint8_t s_hidReportDescriptor[] = {
  // ------------------------------------------------- BÀN PHÍM (KEYBOARD) - ID 1
  0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
  0x09, 0x06,                    // USAGE (Keyboard)
  0xA1, 0x01,                    // COLLECTION (Application)
  0x85, REPORT_ID_KEYBOARD,      //   REPORT_ID (1)
  0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
  0x19, 0xE0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
  0x29, 0xE7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
  0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
  0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
  0x75, 0x01,                    //   REPORT_SIZE (1)
  0x95, 0x08,                    //   REPORT_COUNT (8)
  0x81, 0x02,                    //   INPUT (Data,Var,Abs)
  0x95, 0x01,                    //   REPORT_COUNT (1)
  0x75, 0x08,                    //   REPORT_SIZE (8)
  0x81, 0x03,                    //   INPUT (Cnst,Var,Abs) ; Reserved
  0x95, 0x05,                    //   REPORT_COUNT (5)
  0x75, 0x01,                    //   REPORT_SIZE (1)
  0x05, 0x08,                    //   USAGE_PAGE (LEDs)
  0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
  0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
  0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
  0x95, 0x01,                    //   REPORT_COUNT (1)
  0x75, 0x03,                    //   REPORT_SIZE (3)
  0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs) ; LED Padding
  0x95, 0x06,                    //   REPORT_COUNT (6)
  0x75, 0x08,                    //   REPORT_SIZE (8)
  0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
  0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
  0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
  0x19, 0x00,                    //   USAGE_MINIMUM (Reserved)
  0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
  0x81, 0x00,                    //   INPUT (Data,Ary,Abs) ; 6 keycodes
  0xC0,                          // END_COLLECTION

  // ------------------------------------------------- ĐA PHƯƠNG TIỆN (CONSUMER CONTROL) - ID 2
  0x05, 0x0C,                    // USAGE_PAGE (Consumer Devices)
  0x09, 0x01,                    // USAGE (Consumer Control)
  0xA1, 0x01,                    // COLLECTION (Application)
  0x85, REPORT_ID_CONSUMER,      //   REPORT_ID (2)
  0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
  0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
  0x75, 0x01,                    //   REPORT_SIZE (1)
  0x95, 0x10,                    //   REPORT_COUNT (16)
  0x09, 0xB5,                    //   USAGE (Scan Next Track)
  0x09, 0xB6,                    //   USAGE (Scan Previous Track)
  0x09, 0xB7,                    //   USAGE (Stop)
  0x09, 0xCD,                    //   USAGE (Play/Pause)
  0x09, 0xE2,                    //   USAGE (Mute)
  0x09, 0xE9,                    //   USAGE (Volume Increment)
  0x09, 0xEA,                    //   USAGE (Volume Decrement)
  0x0A, 0x23, 0x02,              //   USAGE (AL OEM Features / WWW Home)
  0x0A, 0x94, 0x01,              //   USAGE (AL Local Machine Browser / My Computer)
  0x0A, 0x92, 0x01,              //   USAGE (AL Calculator)
  0x0A, 0x2A, 0x02,              //   USAGE (AC Bookmarks)
  0x0A, 0x21, 0x02,              //   USAGE (AC Search)
  0x0A, 0x26, 0x02,              //   USAGE (AC Stop)
  0x0A, 0x24, 0x02,              //   USAGE (AC Back)
  0x0A, 0x83, 0x01,              //   USAGE (AL Consumer Control Configuration)
  0x0A, 0x8A, 0x01,              //   USAGE (AL Email Reader)
  0x81, 0x02,                    //   INPUT (Data,Var,Abs)
  0xC0,                          // END_COLLECTION

  // ------------------------------------------------- CHUỘT (MOUSE) - ID 3
  0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
  0x09, 0x02,                    // USAGE (Mouse)
  0xA1, 0x01,                    // COLLECTION (Application)
  0x09, 0x01,                    //   USAGE (Pointer)
  0xA1, 0x00,                    //   COLLECTION (Physical)
  0x85, REPORT_ID_MOUSE,         //     REPORT_ID (3)
  0x05, 0x09,                    //     USAGE_PAGE (Button)
  0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
  0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)
  0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
  0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
  0x95, 0x03,                    //     REPORT_COUNT (3)
  0x75, 0x01,                    //     REPORT_SIZE (1)
  0x81, 0x02,                    //     INPUT (Data,Var,Abs)
  0x95, 0x01,                    //     REPORT_COUNT (1)
  0x75, 0x05,                    //     REPORT_SIZE (5)
  0x81, 0x03,                    //     INPUT (Cnst,Var,Abs) ; Button Padding
  0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
  0x09, 0x30,                    //     USAGE (X)
  0x09, 0x31,                    //     USAGE (Y)
  0x09, 0x38,                    //     USAGE (Wheel)
  0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
  0x25, 0x7F,                    //     LOGICAL_MAXIMUM (127)
  0x75, 0x08,                    //     REPORT_SIZE (8)
  0x95, 0x03,                    //     REPORT_COUNT (3)
  0x81, 0x06,                    //     INPUT (Data,Var,Rel)
  0xC0,                          //   END_COLLECTION
  0xC0                           // END_COLLECTION
};

BleHidService *BleHidService::s_instance = nullptr;

class BleHidServerCallbacks : public NimBLEServerCallbacks {
public:
  BleHidServerCallbacks(BleHidService *service) : _service(service) {}

  void onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc) override {
    if (_service != nullptr) {
      _service->_connected = true;
      if (desc != nullptr) {
        char addrStr[32];
        snprintf(addrStr, sizeof(addrStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 desc->peer_id_addr.val[5], desc->peer_id_addr.val[4],
                 desc->peer_id_addr.val[3], desc->peer_id_addr.val[2],
                 desc->peer_id_addr.val[1], desc->peer_id_addr.val[0]);
        _service->_connectedDeviceName = addrStr;
        LOG_I(TAG, "BLE Host da ket noi: dia chi = %s (Handle=%d)", addrStr, desc->conn_handle);

        // Cap nhat thong so ket noi toi uu cho HID (Latency < 15ms)
        pServer->updateConnParams(desc->conn_handle, 6, 12, 0, 400);
      } else {
        _service->_connectedDeviceName = "Connected Host";
        LOG_I(TAG, "BLE Host da ket noi thanh cong!");
      }
    }
  }

  void onDisconnect(NimBLEServer *pServer, ble_gap_conn_desc *desc) override {
    if (_service != nullptr) {
      _service->_connected = false;
      _service->_connectedDeviceName = "";
      _service->keyboardReleaseAll();
      _service->mouseRelease(0xFF);
      _service->mediaKeyRelease();
      LOG_W(TAG, "BLE Host ngat ket noi! Tu dong bat lai quang ba (Advertising)...");
      NimBLEDevice::startAdvertising();
    }
  }

  void onAuthenticationComplete(ble_gap_conn_desc *desc) override {
    if (desc != nullptr) {
      LOG_I(TAG, "BLE Pairing/Xac thuc hoan tat: Encrypted=%d, Authenticated=%d, Bonded=%d",
            desc->sec_state.encrypted, desc->sec_state.authenticated, desc->sec_state.bonded);
    }
  }

  bool onConfirmPIN(uint32_t pin) override {
    LOG_I(TAG, "Tu dong chap nhan ma PIN: %u", pin);
    return true;
  }

private:
  BleHidService *_service;
};

class VolumeCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
public:
  VolumeCharacteristicCallbacks(BleHidService *service) : _service(service) {}

  void onWrite(NimBLECharacteristic *pCharacteristic) override {
    std::string val = pCharacteristic->getValue();
    if (val.length() > 0 && _service != nullptr) {
      uint8_t vol = (uint8_t)val[0];
      if (vol > 100) vol = 100;
      _service->_remoteVolume = vol;
      LOG_I("BleHidService", "Nhan du lieu Volume tu Companion App: %d%%", vol);
      if (_service->_volChangeCb != nullptr) {
        _service->_volChangeCb(vol);
      }
    }
  }

private:
  BleHidService *_service;
};

BleHidService::BleHidService()
  : _server(nullptr)
  , _hidDevice(nullptr)
  , _inputKeyboard(nullptr)
  , _outputKeyboard(nullptr)
  , _inputMouse(nullptr)
  , _inputConsumer(nullptr)
  , _volCharacteristic(nullptr)
  , _taskHandle(nullptr)
  , _cmdQueue(nullptr)
  , _connected(false)
  , _connectedDeviceName("")
  , _batteryLevel(100)
  , _currentMouseButtons(0)
  , _keyboardModifiers(0)
  , _consumerMask(0)
  , _remoteVolume(50)
  , _volChangeCb(nullptr)
{
  memset(_keyboardKeys, 0, sizeof(_keyboardKeys));
  s_instance = this;
}

BleHidService::~BleHidService() {
  end();
  if (s_instance == this) {
    s_instance = nullptr;
  }
}

BleHidService *BleHidService::getInstance() {
  return s_instance;
}

bool BleHidService::begin(const char *deviceName) {
  LOG_I(TAG, "Khoi tao NimBLE HID Composite Device voi ten: %s", deviceName);

  // 1. Khoi tao hang doi lenh HID (Capacity: 32 commands)
  if (_cmdQueue == nullptr) {
    _cmdQueue = xQueueCreate(32, sizeof(BleHidCommand));
    if (_cmdQueue == nullptr) {
      LOG_E(TAG, "Khong the tao Queue cho BLE HID!");
      return false;
    }
  }

  // 2. Khoi tao NimBLE Stack
  NimBLEDevice::init(deviceName);
  // Bonding=true, MITM=false (Just Works khong hoi PIN), SC=true (Secure Connections)
  NimBLEDevice::setSecurityAuth(true, false, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
  NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);

  _server = NimBLEDevice::createServer();
  if (_server == nullptr) {
    LOG_E(TAG, "Loi tao NimBLEServer!");
    return false;
  }
  _server->setCallbacks(new BleHidServerCallbacks(this));

  // 3. Khoi tao HID Device Service
  _hidDevice = new NimBLEHIDDevice(_server);
  _hidDevice->manufacturer()->setValue("Espressif Systems");
  _hidDevice->pnp(0x02, 0x0e5e, 0x028e, 0x0110);
  _hidDevice->hidInfo(0x00, 0x01);
  _hidDevice->reportMap((uint8_t*)s_hidReportDescriptor, sizeof(s_hidReportDescriptor));

  // 4. Lay cac Characteristic tuong ung voi Report ID
  _inputKeyboard = _hidDevice->inputReport(REPORT_ID_KEYBOARD);
  _outputKeyboard = _hidDevice->outputReport(REPORT_ID_KEYBOARD);
  _inputConsumer = _hidDevice->inputReport(REPORT_ID_CONSUMER);
  _inputMouse = _hidDevice->inputReport(REPORT_ID_MOUSE);

  _hidDevice->setBatteryLevel(_batteryLevel);

  // 5. BAT BUOC: Khoi dong cac GATT Services cho HID, Battery va Device Info
  _hidDevice->startServices();

  // 5b. Khoi dong Custom Volume GATT Service (0xFFE0 / 0xFFE1) cho Companion App
  NimBLEService *pVolService = _server->createService(VOLUME_SERVICE_UUID);
  if (pVolService != nullptr) {
    _volCharacteristic = pVolService->createCharacteristic(
      VOLUME_CHARACTERISTIC_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::NOTIFY
    );
    if (_volCharacteristic != nullptr) {
      uint8_t initVol = 50;
      _volCharacteristic->setValue(&initVol, 1);
      _volCharacteristic->setCallbacks(new VolumeCharacteristicCallbacks(this));
    }
    pVolService->start();
    LOG_I(TAG, "Custom Volume GATT Service (0xFFE0 / 0xFFE1) da khoi tao thanh cong!");
  }

  // 6. Khoi dong BLE Advertising (Chi quang ba HID Service UUID de khong tran 31 bytes)
  NimBLEAdvertising *pAdv = NimBLEDevice::getAdvertising();
  pAdv->setAppearance(HID_MOUSE); // 0x03C2: Windows/Mac nhan dien icon Chuot/Trackpad
  pAdv->addServiceUUID(_hidDevice->hidService()->getUUID());
  pAdv->setScanResponse(true);
  pAdv->start();

  // 6. Tao FreeRTOS Task tren Core 0 de xu ly HID khong anh huong Core 1 (UI & Sensors)
  BaseType_t taskRes = xTaskCreatePinnedToCore(
    bleHidTask,
    "bleHidTask",
    4096,
    this,
    5,
    &_taskHandle,
    0 // Core 0
  );

  if (taskRes != pdPASS) {
    LOG_E(TAG, "Khong the tao bleHidTask tren Core 0!");
    return false;
  }

  LOG_I(TAG, "BLE HID Service da bat dau quang ba thanh cong tren Core 0!");
  return true;
}

void BleHidService::end() {
  LOG_I(TAG, "Dung BLE HID Service...");
  if (_taskHandle != nullptr) {
    vTaskDelete(_taskHandle);
    _taskHandle = nullptr;
  }
  if (_cmdQueue != nullptr) {
    vQueueDelete(_cmdQueue);
    _cmdQueue = nullptr;
  }
  if (_server != nullptr) {
    NimBLEDevice::deinit(true);
    _server = nullptr;
    _hidDevice = nullptr;
  }
  _connected = false;
}

bool BleHidService::isConnected() const {
  return _connected;
}

String BleHidService::getConnectedDeviceName() const {
  return _connectedDeviceName;
}

uint8_t BleHidService::getBatteryLevel() const {
  return _batteryLevel;
}

void BleHidService::setBatteryLevel(uint8_t level) {
  _batteryLevel = (level > 100) ? 100 : level;
  if (_hidDevice != nullptr) {
    _hidDevice->setBatteryLevel(_batteryLevel);
  }
}

bool BleHidService::sendCommand(const BleHidCommand &cmd) {
  if (_cmdQueue == nullptr) return false;
  // Non-blocking đẩy lệnh vào queue, nếu đầy thì bỏ qua để tránh nghẽn
  return (xQueueSend(_cmdQueue, &cmd, 0) == pdTRUE);
}

void BleHidService::mouseMove(int8_t dx, int8_t dy, int8_t wheel) {
  BleHidCommand cmd;
  cmd.type = BleHidCommandType::MOUSE_MOVE;
  cmd.data.mouseMove.dx = dx;
  cmd.data.mouseMove.dy = dy;
  cmd.data.mouseMove.wheel = wheel;
  sendCommand(cmd);
}

void BleHidService::mousePress(uint8_t buttons) {
  BleHidCommand cmd;
  cmd.type = BleHidCommandType::MOUSE_BUTTONS;
  _currentMouseButtons |= buttons;
  cmd.data.mouseButton.buttons = _currentMouseButtons;
  sendCommand(cmd);
}

void BleHidService::mouseRelease(uint8_t buttons) {
  BleHidCommand cmd;
  cmd.type = BleHidCommandType::MOUSE_BUTTONS;
  _currentMouseButtons &= ~buttons;
  cmd.data.mouseButton.buttons = _currentMouseButtons;
  sendCommand(cmd);
}

void BleHidService::mouseClick(uint8_t buttons) {
  BleHidCommand cmd;
  cmd.type = BleHidCommandType::MOUSE_CLICK;
  cmd.data.mouseButton.buttons = buttons;
  sendCommand(cmd);
}

void BleHidService::keyboardPress(uint8_t keycode, uint8_t modifiers) {
  BleHidCommand cmd;
  cmd.type = BleHidCommandType::KEYBOARD_PRESS;
  cmd.data.keyboard.keycode = keycode;
  cmd.data.keyboard.modifiers = modifiers;
  sendCommand(cmd);
}

void BleHidService::keyboardRelease(uint8_t keycode) {
  BleHidCommand cmd;
  cmd.type = BleHidCommandType::KEYBOARD_RELEASE;
  cmd.data.keyboard.keycode = keycode;
  cmd.data.keyboard.modifiers = 0;
  sendCommand(cmd);
}

void BleHidService::keyboardReleaseAll() {
  BleHidCommand cmd;
  cmd.type = BleHidCommandType::KEYBOARD_RELEASE_ALL;
  sendCommand(cmd);
}

void BleHidService::keyboardWrite(uint8_t keycode, uint8_t modifiers) {
  BleHidCommand cmd;
  cmd.type = BleHidCommandType::KEYBOARD_WRITE;
  cmd.data.keyboard.keycode = keycode;
  cmd.data.keyboard.modifiers = modifiers;
  sendCommand(cmd);
}

// Bảng ánh xạ ASCII ký tự thường sang HID keycode và modifier Shift
static bool asciiToHid(char c, uint8_t &keycode, uint8_t &modifier) {
  modifier = KEY_MOD_NONE;
  if (c >= 'a' && c <= 'z') {
    keycode = HID_KEY_A + (c - 'a');
    return true;
  }
  if (c >= 'A' && c <= 'Z') {
    keycode = HID_KEY_A + (c - 'A');
    modifier = KEY_MOD_LSHIFT;
    return true;
  }
  if (c >= '1' && c <= '9') {
    keycode = HID_KEY_1 + (c - '1');
    return true;
  }
  switch (c) {
    case '0': keycode = HID_KEY_0; return true;
    case ' ': keycode = HID_KEY_SPACE; return true;
    case '\n': keycode = HID_KEY_ENTER; return true;
    case '\t': keycode = HID_KEY_TAB; return true;
    case '-': keycode = HID_KEY_MINUS; return true;
    case '=': keycode = HID_KEY_EQUAL; return true;
    case ';': keycode = HID_KEY_SEMICOLON; return true;
    case '\'': keycode = HID_KEY_QUOTE; return true;
    case ',': keycode = HID_KEY_COMMA; return true;
    case '.': keycode = HID_KEY_PERIOD; return true;
    case '/': keycode = HID_KEY_SLASH; return true;
    // Ký tự Shift
    case '!': keycode = HID_KEY_1; modifier = KEY_MOD_LSHIFT; return true;
    case '@': keycode = HID_KEY_2; modifier = KEY_MOD_LSHIFT; return true;
    case '#': keycode = HID_KEY_3; modifier = KEY_MOD_LSHIFT; return true;
    case '$': keycode = HID_KEY_4; modifier = KEY_MOD_LSHIFT; return true;
    case '%': keycode = HID_KEY_5; modifier = KEY_MOD_LSHIFT; return true;
    case '^': keycode = HID_KEY_6; modifier = KEY_MOD_LSHIFT; return true;
    case '&': keycode = HID_KEY_7; modifier = KEY_MOD_LSHIFT; return true;
    case '*': keycode = HID_KEY_8; modifier = KEY_MOD_LSHIFT; return true;
    case '(': keycode = HID_KEY_9; modifier = KEY_MOD_LSHIFT; return true;
    case ')': keycode = HID_KEY_0; modifier = KEY_MOD_LSHIFT; return true;
    case '_': keycode = HID_KEY_MINUS; modifier = KEY_MOD_LSHIFT; return true;
    case '+': keycode = HID_KEY_EQUAL; modifier = KEY_MOD_LSHIFT; return true;
    case ':': keycode = HID_KEY_SEMICOLON; modifier = KEY_MOD_LSHIFT; return true;
    case '\"': keycode = HID_KEY_QUOTE; modifier = KEY_MOD_LSHIFT; return true;
    case '<': keycode = HID_KEY_COMMA; modifier = KEY_MOD_LSHIFT; return true;
    case '>': keycode = HID_KEY_PERIOD; modifier = KEY_MOD_LSHIFT; return true;
    case '?': keycode = HID_KEY_SLASH; modifier = KEY_MOD_LSHIFT; return true;
    default: return false;
  }
}

void BleHidService::keyboardPrint(const char *text) {
  if (text == nullptr) return;
  while (*text) {
    uint8_t keycode = 0;
    uint8_t modifier = 0;
    if (asciiToHid(*text, keycode, modifier)) {
      keyboardWrite(keycode, modifier);
    }
    text++;
  }
}

void BleHidService::mediaKeyPress(uint16_t mediaMask) {
  BleHidCommand cmd;
  cmd.type = BleHidCommandType::CONSUMER_PRESS;
  cmd.data.consumer.mediaMask = mediaMask;
  sendCommand(cmd);
}

void BleHidService::mediaKeyRelease() {
  BleHidCommand cmd;
  cmd.type = BleHidCommandType::CONSUMER_RELEASE;
  cmd.data.consumer.mediaMask = 0;
  sendCommand(cmd);
}

void BleHidService::mediaKeyWrite(uint16_t mediaMask) {
  BleHidCommand cmd;
  cmd.type = BleHidCommandType::CONSUMER_WRITE;
  cmd.data.consumer.mediaMask = mediaMask;
  sendCommand(cmd);
}

void BleHidService::setRemoteVolume(uint8_t volumePercent) {
  if (volumePercent > 100) volumePercent = 100;
  _remoteVolume = volumePercent;
  if (_volCharacteristic != nullptr && _connected) {
    _volCharacteristic->setValue(&volumePercent, 1);
    _volCharacteristic->notify();
    LOG_D(TAG, "Goi Notify Volume len Companion App: %d%%", volumePercent);
  }
}

void BleHidService::sendKeyboardReport() {
  if (_inputKeyboard == nullptr || !_connected) return;
  uint8_t report[8];
  report[0] = _keyboardModifiers;
  report[1] = 0x00; // Reserved
  memcpy(&report[2], _keyboardKeys, 6);
  _inputKeyboard->setValue(report, sizeof(report));
  _inputKeyboard->notify();
}

void BleHidService::sendMouseReport(int8_t dx, int8_t dy, int8_t wheel) {
  if (_inputMouse == nullptr || !_connected) return;
  uint8_t report[4];
  report[0] = _currentMouseButtons;
  report[1] = (uint8_t)dx;
  report[2] = (uint8_t)dy;
  report[3] = (uint8_t)wheel;
  _inputMouse->setValue(report, sizeof(report));
  _inputMouse->notify();
}

void BleHidService::sendConsumerReport() {
  if (_inputConsumer == nullptr || !_connected) return;
  uint8_t report[2];
  report[0] = (uint8_t)(_consumerMask & 0xFF);
  report[1] = (uint8_t)((_consumerMask >> 8) & 0xFF);
  _inputConsumer->setValue(report, sizeof(report));
  _inputConsumer->notify();
}

void BleHidService::bleHidTask(void *param) {
  BleHidService *service = static_cast<BleHidService*>(param);
  LOG_I(TAG, "bleHidTask da khoi chay thanh cong tren Core %d", xPortGetCoreID());

  BleHidCommand cmd;
  while (true) {
    if (xQueueReceive(service->_cmdQueue, &cmd, pdMS_TO_TICKS(10)) == pdTRUE) {
      if (service->_connected) {
        switch (cmd.type) {
          case BleHidCommandType::MOUSE_MOVE:
            service->sendMouseReport(cmd.data.mouseMove.dx,
                                     cmd.data.mouseMove.dy,
                                     cmd.data.mouseMove.wheel);
            break;

          case BleHidCommandType::MOUSE_BUTTONS:
            service->_currentMouseButtons = cmd.data.mouseButton.buttons;
            service->sendMouseReport(0, 0, 0);
            break;

          case BleHidCommandType::MOUSE_CLICK:
            service->_currentMouseButtons = cmd.data.mouseButton.buttons;
            service->sendMouseReport(0, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(15));
            service->_currentMouseButtons = 0;
            service->sendMouseReport(0, 0, 0);
            break;

          case BleHidCommandType::KEYBOARD_PRESS: {
            service->_keyboardModifiers |= cmd.data.keyboard.modifiers;
            bool added = false;
            for (int i = 0; i < 6; i++) {
              if (service->_keyboardKeys[i] == 0) {
                service->_keyboardKeys[i] = cmd.data.keyboard.keycode;
                added = true;
                break;
              }
            }
            if (added) {
              service->sendKeyboardReport();
            }
            break;
          }

          case BleHidCommandType::KEYBOARD_RELEASE: {
            bool removed = false;
            for (int i = 0; i < 6; i++) {
              if (service->_keyboardKeys[i] == cmd.data.keyboard.keycode) {
                service->_keyboardKeys[i] = 0;
                removed = true;
                break;
              }
            }
            if (removed) {
              service->sendKeyboardReport();
            }
            break;
          }

          case BleHidCommandType::KEYBOARD_RELEASE_ALL:
            service->_keyboardModifiers = 0;
            memset(service->_keyboardKeys, 0, sizeof(service->_keyboardKeys));
            service->sendKeyboardReport();
            break;

          case BleHidCommandType::KEYBOARD_WRITE:
            service->_keyboardModifiers = cmd.data.keyboard.modifiers;
            service->_keyboardKeys[0] = cmd.data.keyboard.keycode;
            service->sendKeyboardReport();
            vTaskDelay(pdMS_TO_TICKS(12));
            service->_keyboardModifiers = 0;
            service->_keyboardKeys[0] = 0;
            service->sendKeyboardReport();
            vTaskDelay(pdMS_TO_TICKS(10));
            break;

          case BleHidCommandType::CONSUMER_PRESS:
            service->_consumerMask = cmd.data.consumer.mediaMask;
            service->sendConsumerReport();
            break;

          case BleHidCommandType::CONSUMER_RELEASE:
            service->_consumerMask = 0;
            service->sendConsumerReport();
            break;

          case BleHidCommandType::CONSUMER_WRITE:
            service->_consumerMask = cmd.data.consumer.mediaMask;
            service->sendConsumerReport();
            vTaskDelay(pdMS_TO_TICKS(15));
            service->_consumerMask = 0;
            service->sendConsumerReport();
            break;
        }
      }
    }
    // Yield cho FreeRTOS IDLE task để tránh kích hoạt Task WDT (Rule 8)
    vTaskDelay(1);
  }
}
