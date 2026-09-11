#include "serial_cli.h"
#include "log.h"

static const char *TAG = "SerialCli";

SerialCli::SerialCli(BleHidService *hidService)
  : _hidService(hidService)
  , _inputBuffer("")
{
  _inputBuffer.reserve(128);
}

SerialCli::~SerialCli() {
  _hidService = nullptr;
}

void SerialCli::begin() {
  LOG_I(TAG, "Serial CLI da san sang nhan lenh kiem thu. Go 'help' de xem danh sach lenh.");
}

void SerialCli::update() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      if (_inputBuffer.length() > 0) {
        processCommand(_inputBuffer);
        _inputBuffer = "";
      }
    } else {
      if (_inputBuffer.length() < 120) {
        _inputBuffer += c;
      }
    }
  }
}

void SerialCli::printHelp() {
  LOG_I(TAG, "========== DANH SACH LENH KIEM THU BLE HID (PHASE 1) ==========");
  LOG_I(TAG, "1. help                                : Xem danh sach cau lenh");
  LOG_I(TAG, "2. status                              : Kiem tra trang thai BLE, RAM, Pin");
  LOG_I(TAG, "3. mouse <dx> <dy> [wheel]             : Di chuyen chuot (vd: mouse 30 -20)");
  LOG_I(TAG, "4. click [left|right|middle]           : Nhan chuot (vd: click left)");
  LOG_I(TAG, "5. type <van_ban>                      : Go van ban qua ban phim (vd: type Hello)");
  LOG_I(TAG, "6. press <enter|esc|tab|space|backspace>: Nhan 1 phim chuc nang");
  LOG_I(TAG, "7. shortcut <copy|paste|cut|selectall|save|undo>: Gui to hop phim Ctrl+C, Ctrl+V...");
  LOG_I(TAG, "8. media <volup|voldown|mute|play|next|prev|stop|calc|home>: Phim da phuong tien");
  LOG_I(TAG, "9. batt <0-100>                        : Cap nhat muc pin gia lap");
  LOG_I(TAG, "===============================================================");
}

void SerialCli::printStatus() {
  bool connected = (_hidService != nullptr && _hidService->isConnected());
  String host = connected ? _hidService->getConnectedDeviceName() : "Chua ket noi (Dang quang ba...)";
  uint8_t batt = (_hidService != nullptr) ? _hidService->getBatteryLevel() : 0;
  uint32_t freeHeap = ESP.getFreeHeap() / 1024;
  uint32_t freePsram = ESP.getFreePsram() / 1024;

  LOG_I(TAG, "--- TRANG THAI HE THONG ---");
  LOG_I(TAG, "Ket noi BLE : %s", connected ? "DA KET NOI (CONNECTED)" : "CHUA KET NOI (ADVERTISING)");
  LOG_I(TAG, "Thiet bi chu: %s", host.c_str());
  LOG_I(TAG, "Muc Pin     : %u%%", batt);
  LOG_I(TAG, "Heap Free   : %u KB", freeHeap);
  LOG_I(TAG, "PSRAM Free  : %u KB", freePsram);
}

void SerialCli::handleMouseCommand(const String &args) {
  if (_hidService == nullptr) return;
  if (!_hidService->isConnected()) {
    LOG_W(TAG, "Chua ket noi BLE voi PC! Vao cai dat Bluetooth de ghep noi.");
    return;
  }

  int firstSpace = args.indexOf(' ');
  if (firstSpace == -1) {
    LOG_W(TAG, "Cu phap sai: mouse <dx> <dy> [wheel] (vi du: mouse 20 -15)");
    return;
  }

  int dx = args.substring(0, firstSpace).toInt();
  String remain = args.substring(firstSpace + 1);
  remain.trim();

  int secondSpace = remain.indexOf(' ');
  int dy = 0;
  int wheel = 0;
  if (secondSpace == -1) {
    dy = remain.toInt();
  } else {
    dy = remain.substring(0, secondSpace).toInt();
    wheel = remain.substring(secondSpace + 1).toInt();
  }

  _hidService->mouseMove((int8_t)dx, (int8_t)dy, (int8_t)wheel);
  LOG_I(TAG, "Di chuyen chuot: dx=%d, dy=%d, wheel=%d", dx, dy, wheel);
}

void SerialCli::handleClickCommand(const String &args) {
  if (_hidService == nullptr) return;
  if (!_hidService->isConnected()) {
    LOG_W(TAG, "Chua ket noi BLE voi PC!");
    return;
  }

  String btn = args;
  btn.trim();
  btn.toLowerCase();

  uint8_t mask = MOUSE_BUTTON_LEFT;
  if (btn == "right" || btn == "phai") {
    mask = MOUSE_BUTTON_RIGHT;
    LOG_I(TAG, "Click chuot phai (Right Click)");
  } else if (btn == "middle" || btn == "giua") {
    mask = MOUSE_BUTTON_MIDDLE;
    LOG_I(TAG, "Click chuot giua (Middle Click)");
  } else {
    LOG_I(TAG, "Click chuot trai (Left Click)");
  }

  _hidService->mouseClick(mask);
}

void SerialCli::handleTypeCommand(const String &args) {
  if (_hidService == nullptr) return;
  if (!_hidService->isConnected()) {
    LOG_W(TAG, "Chua ket noi BLE voi PC!");
    return;
  }

  LOG_I(TAG, "Dang go van ban qua ban phim: \"%s\"", args.c_str());
  _hidService->keyboardPrint(args.c_str());
}

void SerialCli::handlePressCommand(const String &args) {
  if (_hidService == nullptr) return;
  if (!_hidService->isConnected()) {
    LOG_W(TAG, "Chua ket noi BLE voi PC!");
    return;
  }

  String key = args;
  key.trim();
  key.toLowerCase();

  if (key == "enter") {
    _hidService->keyboardWrite(HID_KEY_ENTER);
    LOG_I(TAG, "Nhan phim: ENTER");
  } else if (key == "esc") {
    _hidService->keyboardWrite(HID_KEY_ESC);
    LOG_I(TAG, "Nhan phim: ESC");
  } else if (key == "tab") {
    _hidService->keyboardWrite(HID_KEY_TAB);
    LOG_I(TAG, "Nhan phim: TAB");
  } else if (key == "space") {
    _hidService->keyboardWrite(HID_KEY_SPACE);
    LOG_I(TAG, "Nhan phim: SPACE");
  } else if (key == "backspace") {
    _hidService->keyboardWrite(HID_KEY_BACKSPACE);
    LOG_I(TAG, "Nhan phim: BACKSPACE");
  } else if (key == "win") {
    _hidService->keyboardWrite(HID_KEY_NONE, KEY_MOD_LMETA);
    LOG_I(TAG, "Nhan phim: WINDOWS / GUI");
  } else if (key == "up") {
    _hidService->keyboardWrite(HID_KEY_UP);
    LOG_I(TAG, "Nhan phim: UP");
  } else if (key == "down") {
    _hidService->keyboardWrite(HID_KEY_DOWN);
    LOG_I(TAG, "Nhan phim: DOWN");
  } else if (key == "left") {
    _hidService->keyboardWrite(HID_KEY_LEFT);
    LOG_I(TAG, "Nhan phim: LEFT");
  } else if (key == "right") {
    _hidService->keyboardWrite(HID_KEY_RIGHT);
    LOG_I(TAG, "Nhan phim: RIGHT");
  } else {
    LOG_W(TAG, "Phim khong ho tro: %s", key.c_str());
  }
}

void SerialCli::handleShortcutCommand(const String &args) {
  if (_hidService == nullptr) return;
  if (!_hidService->isConnected()) {
    LOG_W(TAG, "Chua ket noi BLE voi PC!");
    return;
  }

  String sc = args;
  sc.trim();
  sc.toLowerCase();

  if (sc == "copy") {
    _hidService->keyboardWrite(HID_KEY_C, KEY_MOD_LCTRL);
    LOG_I(TAG, "Phim tat: Ctrl + C (Copy)");
  } else if (sc == "paste") {
    _hidService->keyboardWrite(HID_KEY_V, KEY_MOD_LCTRL);
    LOG_I(TAG, "Phim tat: Ctrl + V (Paste)");
  } else if (sc == "cut") {
    _hidService->keyboardWrite(HID_KEY_X, KEY_MOD_LCTRL);
    LOG_I(TAG, "Phim tat: Ctrl + X (Cut)");
  } else if (sc == "selectall") {
    _hidService->keyboardWrite(HID_KEY_A, KEY_MOD_LCTRL);
    LOG_I(TAG, "Phim tat: Ctrl + A (Select All)");
  } else if (sc == "save") {
    _hidService->keyboardWrite(HID_KEY_S, KEY_MOD_LCTRL);
    LOG_I(TAG, "Phim tat: Ctrl + S (Save)");
  } else if (sc == "undo") {
    _hidService->keyboardWrite(HID_KEY_Z, KEY_MOD_LCTRL);
    LOG_I(TAG, "Phim tat: Ctrl + Z (Undo)");
  } else if (sc == "redo") {
    _hidService->keyboardWrite(HID_KEY_Y, KEY_MOD_LCTRL);
    LOG_I(TAG, "Phim tat: Ctrl + Y (Redo)");
  } else {
    LOG_W(TAG, "Phim tat khong ho tro: %s (Ho tro: copy, paste, cut, selectall, save, undo, redo)", sc.c_str());
  }
}

void SerialCli::handleMediaCommand(const String &args) {
  if (_hidService == nullptr) return;
  if (!_hidService->isConnected()) {
    LOG_W(TAG, "Chua ket noi BLE voi PC!");
    return;
  }

  String act = args;
  act.trim();
  act.toLowerCase();

  if (act == "play" || act == "pause") {
    _hidService->mediaKeyWrite(MEDIA_KEY_PLAY_PAUSE);
    LOG_I(TAG, "Phim Media: PLAY / PAUSE");
  } else if (act == "volup") {
    _hidService->mediaKeyWrite(MEDIA_KEY_VOL_UP);
    LOG_I(TAG, "Phim Media: VOLUME UP");
  } else if (act == "voldown") {
    _hidService->mediaKeyWrite(MEDIA_KEY_VOL_DOWN);
    LOG_I(TAG, "Phim Media: VOLUME DOWN");
  } else if (act == "mute") {
    _hidService->mediaKeyWrite(MEDIA_KEY_MUTE);
    LOG_I(TAG, "Phim Media: MUTE");
  } else if (act == "next") {
    _hidService->mediaKeyWrite(MEDIA_KEY_NEXT);
    LOG_I(TAG, "Phim Media: NEXT TRACK");
  } else if (act == "prev") {
    _hidService->mediaKeyWrite(MEDIA_KEY_PREV);
    LOG_I(TAG, "Phim Media: PREVIOUS TRACK");
  } else if (act == "stop") {
    _hidService->mediaKeyWrite(MEDIA_KEY_STOP);
    LOG_I(TAG, "Phim Media: STOP");
  } else if (act == "calc") {
    _hidService->mediaKeyWrite(MEDIA_KEY_CALCULATOR);
    LOG_I(TAG, "Phim Media: CALCULATOR");
  } else if (act == "home") {
    _hidService->mediaKeyWrite(MEDIA_KEY_HOME);
    LOG_I(TAG, "Phim Media: HOME BROWSER");
  } else {
    LOG_W(TAG, "Lenh Media khong hop le: %s", act.c_str());
  }
}

void SerialCli::processCommand(const String &cmdLine) {
  String line = cmdLine;
  line.trim();
  if (line.length() == 0) return;

  int spaceIndex = line.indexOf(' ');
  String cmd = (spaceIndex == -1) ? line : line.substring(0, spaceIndex);
  String args = (spaceIndex == -1) ? "" : line.substring(spaceIndex + 1);
  cmd.toLowerCase();
  args.trim();

  if (cmd == "help" || cmd == "?") {
    printHelp();
  } else if (cmd == "status") {
    printStatus();
  } else if (cmd == "mouse") {
    handleMouseCommand(args);
  } else if (cmd == "click") {
    handleClickCommand(args);
  } else if (cmd == "type") {
    handleTypeCommand(args);
  } else if (cmd == "press") {
    handlePressCommand(args);
  } else if (cmd == "shortcut") {
    handleShortcutCommand(args);
  } else if (cmd == "media") {
    handleMediaCommand(args);
  } else if (cmd == "batt") {
    int level = args.toInt();
    if (_hidService != nullptr) {
      _hidService->setBatteryLevel(level);
      LOG_I(TAG, "Da cap nhat muc pin gia lap thanh: %d%%", level);
    }
  } else {
    LOG_W(TAG, "Lenh khong hop le: '%s'. Go 'help' de xem huong dan.", cmd.c_str());
  }
}
