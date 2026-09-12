#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "services/ble_hid_service.h"

enum class OsMode : uint8_t {
  WINDOWS,
  MACOS
};

class UiManager {
public:
  UiManager(BleHidService *hidService);
  ~UiManager();

  // Khởi tạo giao diện đồ họa LVGL
  bool begin();

  // Cập nhật trạng thái BLE lên giao diện (gọi khi có kết nối/ngắt kết nối)
  void setBleConnected(bool connected, const String &peerName = "");

  // Cập nhật mức pin lên thanh trạng thái
  void setBatteryLevel(uint8_t level);

  // Đổi chế độ hệ điều hành (Windows / macOS)
  void toggleOsMode();
  OsMode getOsMode() const { return _osMode; }

  // Đổi trạng thái âm thanh click phản hồi
  void toggleSoundFeedback();
  bool isSoundFeedbackEnabled() const { return _soundEnabled; }

  // Đồng bộ âm lượng 2 chiều từ Companion App
  void syncVolumeFromRemote(uint8_t volumePercent);

  // Singleton instance
  static UiManager *getInstance();

private:
  static UiManager *s_instance;
  BleHidService *_hidService;

  // Screens
  lv_obj_t *_pairingScreen;
  lv_obj_t *_mainScreen;

  // Pairing screen widgets
  lv_obj_t *_pairingStatusLabel;
  lv_obj_t *_pairingHostLabel;

  // Top Status Bar widgets
  lv_obj_t *_statusBar;
  lv_obj_t *_bleBadge;
  lv_obj_t *_bleLabel;
  lv_obj_t *_osBtn;
  lv_obj_t *_osLabel;
  lv_obj_t *_soundBtn;
  lv_obj_t *_soundLabel;
  lv_obj_t *_batteryLabel;

  // Tabview
  lv_obj_t *_tabview;
  lv_obj_t *_tabTouchpad;
  lv_obj_t *_tabShortcuts;
  lv_obj_t *_tabNumpad;
  lv_obj_t *_tabMedia;
  lv_obj_t *_tabSettings;

  // Trackpad widgets
  lv_obj_t *_trackpadArea;

  // State
  bool _connected;
  String _peerName;
  OsMode _osMode;
  bool _soundEnabled;
  uint8_t _batteryLevel;
  bool _isSyncingVolume;

  void createPairingScreen();
  void createMainDashboard();
  void setupTabTouchpad();
  void setupTabShortcuts();
  void setupTabNumpad();
  void setupTabMedia();
  void setupTabSettings();

  // Media & Navigation widgets
  lv_obj_t *_volSlider;
  lv_obj_t *_volLabel;
  int32_t _lastVolumeSliderVal;

  // Event Callbacks
  static void onTrackpadEvent(lv_event_t *e);
  static void onLeftClickEvent(lv_event_t *e);
  static void onRightClickEvent(lv_event_t *e);
  static void onScrollUpEvent(lv_event_t *e);
  static void onScrollDownEvent(lv_event_t *e);
  static void onShortcutBtnEvent(lv_event_t *e);
  static void onNumpadBtnEvent(lv_event_t *e);
  static void onMediaBtnEvent(lv_event_t *e);
  static void onVolumeSliderEvent(lv_event_t *e);
  static void onArrowKeyBtnEvent(lv_event_t *e);
  static void onOsToggleEvent(lv_event_t *e);
  static void onSoundToggleEvent(lv_event_t *e);
};
