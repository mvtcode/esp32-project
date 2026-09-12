#include "ui_manager.h"
#include "log.h"
#include "ui/fonts/vi_fonts.h"
#include "modules/audio_test.h"

static const char *TAG = "UiManager";

UiManager *UiManager::s_instance = nullptr;

// Màu sắc thiết kế hệ thống (Modern Dark Theme)
#define COLOR_BG_DARK        lv_color_hex(0x0E1117)
#define COLOR_PANEL_BG       lv_color_hex(0x181C26)
#define COLOR_PANEL_BORDER   lv_color_hex(0x272E3F)
#define COLOR_PRIMARY_BLUE   lv_color_hex(0x2563EB)
#define COLOR_ACTIVE_BLUE    lv_color_hex(0x3B82F6)
#define COLOR_SUCCESS_GREEN  lv_color_hex(0x10B981)
#define COLOR_WARN_AMBER     lv_color_hex(0xF59E0B)
#define COLOR_TEXT_WHITE     lv_color_hex(0xF8FAFC)
#define COLOR_TEXT_MUTED     lv_color_hex(0x94A3B8)

UiManager::UiManager(BleHidService *hidService)
  : _hidService(hidService)
  , _pairingScreen(nullptr)
  , _mainScreen(nullptr)
  , _pairingStatusLabel(nullptr)
  , _pairingHostLabel(nullptr)
  , _statusBar(nullptr)
  , _bleBadge(nullptr)
  , _bleLabel(nullptr)
  , _osBtn(nullptr)
  , _osLabel(nullptr)
  , _soundBtn(nullptr)
  , _soundLabel(nullptr)
  , _batteryLabel(nullptr)
  , _tabview(nullptr)
  , _tabTouchpad(nullptr)
  , _tabShortcuts(nullptr)
  , _tabNumpad(nullptr)
  , _tabMedia(nullptr)
  , _tabSettings(nullptr)
  , _trackpadArea(nullptr)
  , _volSlider(nullptr)
  , _volLabel(nullptr)
  , _lastVolumeSliderVal(50)
  , _connected(false)
  , _peerName("")
  , _osMode(OsMode::WINDOWS)
  , _soundEnabled(true)
  , _batteryLevel(100)
  , _isSyncingVolume(false)
{
  s_instance = this;
}

UiManager::~UiManager() {
  if (s_instance == this) {
    s_instance = nullptr;
  }
}

UiManager *UiManager::getInstance() {
  return s_instance;
}

bool UiManager::begin() {
  LOG_I(TAG, "Khoi tao giao dien LVGL UI Framework Portrait (320x480)...");

  // Áp dụng font tiếng Việt mặc định cho theme
  lv_theme_t *th = lv_theme_default_init(
    lv_disp_get_default(),
    COLOR_PRIMARY_BLUE,
    lv_color_hex(0x475569),
    true, // Dark mode
    &vi_font_montserrat_14
  );
  lv_disp_set_theme(lv_disp_get_default(), th);

  // Đăng ký callback nhận volume từ BLE Companion App
  if (_hidService != nullptr) {
    _hidService->setVolumeChangeCallback([](uint8_t vol) {
      if (UiManager::getInstance() != nullptr) {
        UiManager::getInstance()->syncVolumeFromRemote(vol);
      }
    });
  }

  // 1. Tạo Màn hình chờ kết nối (Pairing Screen)
  createPairingScreen();

  // 2. Tạo Màn hình chính (Main Dashboard)
  createMainDashboard();

  // Mặc định nạp màn hình Pairing Screen khi vừa khởi động
  lv_scr_load(_pairingScreen);
  lv_refr_now(nullptr);

  LOG_I(TAG, "UI Manager da san sang va da render frame dau tien!");
  return true;
}

void UiManager::createPairingScreen() {
  _pairingScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(_pairingScreen, COLOR_BG_DARK, 0);

  // Khung Card trung tâm Portrait 320x480 (kích thước 290 x 380)
  lv_obj_t *card = lv_obj_create(_pairingScreen);
  lv_obj_set_size(card, 290, 380);
  lv_obj_center(card);
  lv_obj_set_style_bg_color(card, COLOR_PANEL_BG, 0);
  lv_obj_set_style_border_color(card, COLOR_PANEL_BORDER, 0);
  lv_obj_set_style_border_width(card, 2, 0);
  lv_obj_set_style_radius(card, 16, 0);
  lv_obj_set_style_pad_all(card, 16, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  // Tiêu đề lớn
  lv_obj_t *title = lv_label_create(card);
  lv_obj_set_style_text_font(title, &vi_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title, COLOR_TEXT_WHITE, 0);
  lv_label_set_text(title, "ESP32-S3 HID");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  // Nhãn phụ
  lv_obj_t *subtitle = lv_label_create(card);
  lv_obj_set_style_text_font(subtitle, &vi_font_montserrat_12, 0);
  lv_obj_set_style_text_color(subtitle, COLOR_ACTIVE_BLUE, 0);
  lv_label_set_text(subtitle, "Bàn Di Chuột & Phím Tắt");
  lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 40);

  // Icon biểu tượng chính
  lv_obj_t *iconCenter = lv_label_create(card);
  lv_obj_set_style_text_font(iconCenter, &vi_icon_font_24, 0);
  lv_obj_set_style_text_color(iconCenter, COLOR_PRIMARY_BLUE, 0);
  lv_label_set_text(iconCenter, ICON_TRACKPAD);
  lv_obj_align(iconCenter, LV_ALIGN_TOP_MID, 0, 85);

  // Huy hiệu trạng thái BLE
  _bleBadge = lv_obj_create(card);
  lv_obj_set_size(_bleBadge, 258, 56);
  lv_obj_align(_bleBadge, LV_ALIGN_CENTER, 0, 20);
  lv_obj_set_style_bg_color(_bleBadge, lv_color_hex(0x1F2433), 0);
  lv_obj_set_style_border_color(_bleBadge, COLOR_WARN_AMBER, 0);
  lv_obj_set_style_border_width(_bleBadge, 1, 0);
  lv_obj_set_style_radius(_bleBadge, 10, 0);
  lv_obj_clear_flag(_bleBadge, LV_OBJ_FLAG_SCROLLABLE);

  _pairingStatusLabel = lv_label_create(_bleBadge);
  lv_obj_set_style_text_font(_pairingStatusLabel, &vi_font_montserrat_14, 0);
  lv_obj_set_style_text_color(_pairingStatusLabel, COLOR_WARN_AMBER, 0);
  lv_label_set_text(_pairingStatusLabel, "Chờ ghép nối Bluetooth...");
  lv_obj_center(_pairingStatusLabel);

  // Hướng dẫn người dùng bên dưới
  _pairingHostLabel = lv_label_create(card);
  lv_obj_set_style_text_font(_pairingHostLabel, &vi_font_montserrat_12, 0);
  lv_obj_set_style_text_color(_pairingHostLabel, COLOR_TEXT_MUTED, 0);
  lv_obj_set_style_text_align(_pairingHostLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_long_mode(_pairingHostLabel, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(_pairingHostLabel, 258);
  lv_label_set_text(_pairingHostLabel, "Mở Cài đặt Bluetooth trên máy tính để ghép nối");
  lv_obj_align(_pairingHostLabel, LV_ALIGN_BOTTOM_MID, 0, -15);
}

void UiManager::createMainDashboard() {
  _mainScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(_mainScreen, COLOR_BG_DARK, 0);
  lv_obj_clear_flag(_mainScreen, LV_OBJ_FLAG_SCROLLABLE);

  // 1. Tạo Tabview 5 Tabs (Portrait 320 x 480)
  _tabview = lv_tabview_create(_mainScreen, LV_DIR_TOP, 38);
  lv_obj_set_pos(_tabview, 0, 0);
  lv_obj_set_size(_tabview, 320, 480);
  lv_obj_set_style_bg_color(_tabview, COLOR_BG_DARK, 0);

  // Tắt swipe gesture chuyển tab để tránh nhầm lẫn khi kéo chuột
  lv_obj_clear_flag(lv_tabview_get_content(_tabview), LV_OBJ_FLAG_SCROLLABLE);

  // Tinh chỉnh thanh Tab bar nút bấm (Dùng icon font FontAwesome 20px)
  lv_obj_t *tab_btns = lv_tabview_get_tab_btns(_tabview);
  lv_obj_set_style_bg_color(tab_btns, lv_color_hex(0x131720), 0);
  lv_obj_set_style_text_font(tab_btns, &vi_icon_font_20, 0);
  lv_obj_set_style_text_color(tab_btns, COLOR_TEXT_MUTED, 0);
  lv_obj_set_style_text_color(tab_btns, COLOR_ACTIVE_BLUE, LV_STATE_CHECKED);
  lv_obj_set_style_border_side(tab_btns, LV_BORDER_SIDE_BOTTOM, 0);
  lv_obj_set_style_border_color(tab_btns, COLOR_PANEL_BORDER, 0);
  lv_obj_set_style_border_width(tab_btns, 1, 0);

  // Khởi tạo các Tab chỉ với icon font (không chữ)
  _tabTouchpad  = lv_tabview_add_tab(_tabview, ICON_TRACKPAD);
  _tabShortcuts = lv_tabview_add_tab(_tabview, ICON_KEYBOARD);
  _tabNumpad    = lv_tabview_add_tab(_tabview, ICON_NUMPAD);
  _tabMedia     = lv_tabview_add_tab(_tabview, ICON_AUDIO);
  _tabSettings  = lv_tabview_add_tab(_tabview, ICON_SETTINGS);

  // Tắt scrollable trên từng tab
  lv_obj_clear_flag(_tabTouchpad, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(_tabShortcuts, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(_tabNumpad, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(_tabMedia, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(_tabSettings, LV_OBJ_FLAG_SCROLLABLE);

  // Thiết lập nội dung cho từng Tab
  setupTabTouchpad();
  setupTabShortcuts();
  setupTabNumpad();
  setupTabMedia();
  setupTabSettings();
}

void UiManager::setupTabTouchpad() {
  lv_obj_set_style_pad_all(_tabTouchpad, 6, 0);
  lv_obj_clear_flag(_tabTouchpad, LV_OBJ_FLAG_SCROLLABLE);

  // Vùng Touchpad trung tâm rộng lớn (308 x 360 px)
  _trackpadArea = lv_obj_create(_tabTouchpad);
  lv_obj_set_size(_trackpadArea, 308, 360);
  lv_obj_align(_trackpadArea, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(_trackpadArea, COLOR_PANEL_BG, 0);
  lv_obj_set_style_border_color(_trackpadArea, COLOR_PANEL_BORDER, 0);
  lv_obj_set_style_border_width(_trackpadArea, 1, 0);
  lv_obj_set_style_radius(_trackpadArea, 12, 0);
  lv_obj_add_event_cb(_trackpadArea, onTrackpadEvent, LV_EVENT_ALL, this);
  lv_obj_clear_flag(_trackpadArea, LV_OBJ_FLAG_SCROLLABLE);

  // Biểu tượng mờ chỉ báo di chuyển ở giữa trackpad
  lv_obj_t *hintIcon = lv_label_create(_trackpadArea);
  lv_obj_set_style_text_font(hintIcon, &vi_icon_font_24, 0);
  lv_obj_set_style_text_color(hintIcon, lv_color_hex(0x272E3F), 0);
  lv_label_set_text(hintIcon, ICON_ARROWS);
  lv_obj_center(hintIcon);

  // Cụm 4 nút bấm ở đáy: Left Click (86px), Scroll Up (58px), Scroll Down (58px), Right Click (86px)
  // Tổng chiều rộng: 86 + 58 + 58 + 86 + 3*6 = 306px (vừa khít 308px)

  // 1. Chuột trái (Left Click)
  lv_obj_t *btnLeft = lv_btn_create(_tabTouchpad);
  lv_obj_set_size(btnLeft, 86, 54);
  lv_obj_align(btnLeft, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_obj_set_style_bg_color(btnLeft, lv_color_hex(0x1F2636), 0);
  lv_obj_set_style_border_color(btnLeft, COLOR_PRIMARY_BLUE, 0);
  lv_obj_set_style_border_width(btnLeft, 1, 0);
  lv_obj_set_style_radius(btnLeft, 10, 0);
  lv_obj_add_event_cb(btnLeft, onLeftClickEvent, LV_EVENT_CLICKED, this);

  lv_obj_t *lblLeft = lv_label_create(btnLeft);
  lv_obj_set_style_text_font(lblLeft, &vi_icon_font_24, 0);
  lv_obj_set_style_text_color(lblLeft, COLOR_ACTIVE_BLUE, 0);
  lv_label_set_text(lblLeft, ICON_MOUSE);
  lv_obj_center(lblLeft);

  // 2. Cuộn Lên (Scroll Up)
  lv_obj_t *btnScrollUp = lv_btn_create(_tabTouchpad);
  lv_obj_set_size(btnScrollUp, 58, 54);
  lv_obj_align(btnScrollUp, LV_ALIGN_BOTTOM_MID, -34, 0);
  lv_obj_set_style_bg_color(btnScrollUp, lv_color_hex(0x1F2636), 0);
  lv_obj_set_style_border_color(btnScrollUp, COLOR_PANEL_BORDER, 0);
  lv_obj_set_style_border_width(btnScrollUp, 1, 0);
  lv_obj_set_style_radius(btnScrollUp, 10, 0);
  lv_obj_add_event_cb(btnScrollUp, onScrollUpEvent, LV_EVENT_CLICKED, this);

  lv_obj_t *lblScrollUp = lv_label_create(btnScrollUp);
  lv_obj_set_style_text_font(lblScrollUp, &vi_icon_font_24, 0);
  lv_obj_set_style_text_color(lblScrollUp, COLOR_TEXT_WHITE, 0);
  lv_label_set_text(lblScrollUp, ICON_ARROW_UP);
  lv_obj_center(lblScrollUp);

  // 3. Cuộn Xuống (Scroll Down)
  lv_obj_t *btnScrollDown = lv_btn_create(_tabTouchpad);
  lv_obj_set_size(btnScrollDown, 58, 54);
  lv_obj_align(btnScrollDown, LV_ALIGN_BOTTOM_MID, 34, 0);
  lv_obj_set_style_bg_color(btnScrollDown, lv_color_hex(0x1F2636), 0);
  lv_obj_set_style_border_color(btnScrollDown, COLOR_PANEL_BORDER, 0);
  lv_obj_set_style_border_width(btnScrollDown, 1, 0);
  lv_obj_set_style_radius(btnScrollDown, 10, 0);
  lv_obj_add_event_cb(btnScrollDown, onScrollDownEvent, LV_EVENT_CLICKED, this);

  lv_obj_t *lblScrollDown = lv_label_create(btnScrollDown);
  lv_obj_set_style_text_font(lblScrollDown, &vi_icon_font_24, 0);
  lv_obj_set_style_text_color(lblScrollDown, COLOR_TEXT_WHITE, 0);
  lv_label_set_text(lblScrollDown, ICON_ARROW_DOWN);
  lv_obj_center(lblScrollDown);

  // 4. Chuột phải (Right Click)
  lv_obj_t *btnRight = lv_btn_create(_tabTouchpad);
  lv_obj_set_size(btnRight, 86, 54);
  lv_obj_align(btnRight, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(btnRight, lv_color_hex(0x1F2636), 0);
  lv_obj_set_style_border_color(btnRight, COLOR_PANEL_BORDER, 0);
  lv_obj_set_style_border_width(btnRight, 1, 0);
  lv_obj_set_style_radius(btnRight, 10, 0);
  lv_obj_add_event_cb(btnRight, onRightClickEvent, LV_EVENT_CLICKED, this);

  lv_obj_t *lblRight = lv_label_create(btnRight);
  lv_obj_set_style_text_font(lblRight, &vi_font_montserrat_16, 0);
  lv_obj_set_style_text_color(lblRight, COLOR_TEXT_WHITE, 0);
  lv_label_set_text(lblRight, "R");
  lv_obj_center(lblRight);
}

void UiManager::setupTabShortcuts() {
  lv_obj_set_style_pad_all(_tabShortcuts, 6, 0);
  lv_obj_clear_flag(_tabShortcuts, LV_OBJ_FLAG_SCROLLABLE);

  struct ShortcutItem {
    const char *title;
    const char *hint;
    uint32_t borderColor;
    const lv_font_t *font;
  };

  const ShortcutItem items[16] = {
    // Hàng 1: Hệ thống & Tiện ích (FontAwesome 24px)
    { ICON_LOCK,               "Win+L",    0xEF4444, &vi_icon_font_24 },
    { ICON_CAMERA,             "Snip",     0xF59E0B, &vi_icon_font_24 },
    { ICON_TASKS,              "TaskMgr",  0x8B5CF6, &vi_icon_font_24 },
    { ICON_CALC,               "Calc",     0xEC4899, &vi_icon_font_24 },

    // Hàng 2: Trình duyệt Web (LVGL Symbols 20px)
    { LV_SYMBOL_LEFT,          "Alt+Left", 0x06B6D4, &vi_font_montserrat_20 },
    { LV_SYMBOL_RIGHT,         "Alt+Right",0x06B6D4, &vi_font_montserrat_20 },
    { ICON_REFRESH,            "F5",       0x0284C7, &vi_icon_font_24 },
    { LV_SYMBOL_PLUS " Tab",   "Ctrl+T",   0x0284C7, &vi_font_montserrat_16 },

    // Hàng 3: Soạn thảo (LVGL Symbols 20px)
    { LV_SYMBOL_COPY,          "Ctrl+C",   0x10B981, &vi_font_montserrat_20 },
    { LV_SYMBOL_PASTE,         "Ctrl+V",   0x10B981, &vi_font_montserrat_20 },
    { LV_SYMBOL_CUT,           "Ctrl+X",   0x059669, &vi_font_montserrat_20 },
    { LV_SYMBOL_LOOP,          "Ctrl+Z",   0x059669, &vi_font_montserrat_20 },

    // Hàng 4: Điều khiển & Hành động (LVGL Symbols 20px)
    { LV_SYMBOL_LIST,          "Ctrl+A",   0x3B82F6, &vi_font_montserrat_20 },
    { LV_SYMBOL_SAVE,          "Ctrl+S",   0x3B82F6, &vi_font_montserrat_20 },
    { LV_SYMBOL_CLOSE,         "Esc",      0x64748B, &vi_font_montserrat_20 },
    { LV_SYMBOL_OK,            "ENTER",    0x2563EB, &vi_font_montserrat_20 }
  };

  const int btnW = 70;
  const int btnH = 98;
  const int gapX = 9;
  const int gapY = 12;

  for (int i = 0; i < 16; i++) {
    int row = i / 4;
    int col = i % 4;

    lv_obj_t *btn = lv_btn_create(_tabShortcuts);
    lv_obj_set_size(btn, btnW, btnH);
    lv_obj_set_pos(btn, col * (btnW + gapX), row * (btnH + gapY));
    lv_obj_set_style_bg_color(btn, COLOR_PANEL_BG, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(items[i].borderColor), 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, 10, 0);

    lv_obj_set_user_data(btn, (void*)(intptr_t)i);
    lv_obj_add_event_cb(btn, onShortcutBtnEvent, LV_EVENT_CLICKED, this);

    // Tiêu đề / Icon trên
    lv_obj_t *lblTitle = lv_label_create(btn);
    lv_obj_set_style_text_font(lblTitle, items[i].font, 0);
    lv_obj_set_style_text_color(lblTitle, lv_color_hex(items[i].borderColor), 0);
    lv_label_set_text(lblTitle, items[i].title);
    lv_obj_align(lblTitle, LV_ALIGN_TOP_MID, 0, 14);

    // Nhãn phím tắt dưới
    lv_obj_t *lblHint = lv_label_create(btn);
    lv_obj_set_style_text_font(lblHint, &vi_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lblHint, COLOR_TEXT_MUTED, 0);
    lv_label_set_text(lblHint, items[i].hint);
    lv_obj_align(lblHint, LV_ALIGN_BOTTOM_MID, 0, -12);
  }
}

void UiManager::setupTabNumpad() {
  lv_obj_set_style_pad_all(_tabNumpad, 6, 0);
  lv_obj_clear_flag(_tabNumpad, LV_OBJ_FLAG_SCROLLABLE);

  // Bàn phím số Numpad (4 cột x 4 hàng)
  const char *numpadKeys[16] = {
    "7", "8", "9", "/",
    "4", "5", "6", "*",
    "1", "2", "3", "-",
    "0", ".", "=", "+"
  };

  for (int i = 0; i < 16; i++) {
    int row = i / 4;
    int col = i % 4;

    lv_obj_t *btn = lv_btn_create(_tabNumpad);
    lv_obj_set_size(btn, 72, 64);
    lv_obj_set_pos(btn, col * 78, row * 70);
    lv_obj_set_style_bg_color(btn, (col == 3) ? lv_color_hex(0x2A344A) : COLOR_PANEL_BG, 0);
    lv_obj_set_style_border_color(btn, COLOR_PANEL_BORDER, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, 8, 0);

    lv_obj_set_user_data(btn, (void*)numpadKeys[i]);
    lv_obj_add_event_cb(btn, onNumpadBtnEvent, LV_EVENT_CLICKED, this);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_obj_set_style_text_font(lbl, &vi_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl, COLOR_TEXT_WHITE, 0);
    lv_label_set_text(lbl, numpadKeys[i]);
    lv_obj_center(lbl);
  }

  // Hàng cuối: Phím Backspace và Enter bên dưới
  lv_obj_t *btnBs = lv_btn_create(_tabNumpad);
  lv_obj_set_size(btnBs, 148, 64);
  lv_obj_set_pos(btnBs, 0, 286);
  lv_obj_set_style_bg_color(btnBs, lv_color_hex(0xDC2626), 0);
  lv_obj_set_style_radius(btnBs, 10, 0);
  lv_obj_set_user_data(btnBs, (void*)"BACKSPACE");
  lv_obj_add_event_cb(btnBs, onNumpadBtnEvent, LV_EVENT_CLICKED, this);

  lv_obj_t *lblBs = lv_label_create(btnBs);
  lv_obj_set_style_text_font(lblBs, &vi_font_montserrat_16, 0);
  lv_obj_set_style_text_color(lblBs, COLOR_TEXT_WHITE, 0);
  lv_label_set_text(lblBs, "XÓA (BS)");
  lv_obj_center(lblBs);

  lv_obj_t *btnEnter = lv_btn_create(_tabNumpad);
  lv_obj_set_size(btnEnter, 148, 64);
  lv_obj_set_pos(btnEnter, 156, 286);
  lv_obj_set_style_bg_color(btnEnter, COLOR_PRIMARY_BLUE, 0);
  lv_obj_set_style_radius(btnEnter, 10, 0);
  lv_obj_set_user_data(btnEnter, (void*)"ENTER");
  lv_obj_add_event_cb(btnEnter, onNumpadBtnEvent, LV_EVENT_CLICKED, this);

  lv_obj_t *lblEnter = lv_label_create(btnEnter);
  lv_obj_set_style_text_font(lblEnter, &vi_font_montserrat_16, 0);
  lv_obj_set_style_text_color(lblEnter, COLOR_TEXT_WHITE, 0);
  lv_label_set_text(lblEnter, "ENTER");
  lv_obj_center(lblEnter);
}

void UiManager::setupTabMedia() {
  lv_obj_set_style_pad_all(_tabMedia, 6, 0);
  lv_obj_clear_flag(_tabMedia, LV_OBJ_FLAG_SCROLLABLE);

  // -------------------------------------------------------------
  // Card 1: Volume Control (Thanh trượt & 3 nút âm lượng)
  // -------------------------------------------------------------
  lv_obj_t *cardVol = lv_obj_create(_tabMedia);
  lv_obj_set_size(cardVol, 304, 105);
  lv_obj_align(cardVol, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(cardVol, COLOR_PANEL_BG, 0);
  lv_obj_set_style_border_color(cardVol, COLOR_PANEL_BORDER, 0);
  lv_obj_set_style_border_width(cardVol, 1, 0);
  lv_obj_set_style_radius(cardVol, 10, 0);
  lv_obj_set_style_pad_all(cardVol, 8, 0);
  lv_obj_clear_flag(cardVol, LV_OBJ_FLAG_SCROLLABLE);

  _volLabel = lv_label_create(cardVol);
  lv_obj_set_style_text_font(_volLabel, &vi_font_montserrat_12, 0);
  lv_obj_set_style_text_color(_volLabel, COLOR_ACTIVE_BLUE, 0);
  lv_label_set_text(_volLabel, "ÂM LƯỢNG HỆ THỐNG");
  lv_obj_align(_volLabel, LV_ALIGN_TOP_LEFT, 0, 0);

  // Slider âm lượng
  _volSlider = lv_slider_create(cardVol);
  lv_obj_set_size(_volSlider, 276, 12);
  lv_obj_align(_volSlider, LV_ALIGN_TOP_MID, 0, 20);
  lv_slider_set_range(_volSlider, 0, 100);
  lv_slider_set_value(_volSlider, 50, LV_ANIM_OFF);
  _lastVolumeSliderVal = 50;
  lv_obj_set_style_bg_color(_volSlider, lv_color_hex(0x272E3F), LV_PART_MAIN);
  lv_obj_set_style_bg_color(_volSlider, COLOR_ACTIVE_BLUE, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(_volSlider, COLOR_TEXT_WHITE, LV_PART_KNOB);
  lv_obj_set_style_pad_all(_volSlider, 3, LV_PART_KNOB);
  lv_obj_add_flag(_volSlider, LV_OBJ_FLAG_CLICK_FOCUSABLE);
  lv_obj_add_event_cb(_volSlider, onVolumeSliderEvent, LV_EVENT_VALUE_CHANGED, this);

  // 3 nút âm lượng bên dưới: Vol - (idx 4), Mute (idx 6), Vol + (idx 5)
  const char *volBtnLabels[3] = { ICON_VOL_DOWN, ICON_MUTE, ICON_VOL_UP };
  int volBtnIndices[3] = { 4, 6, 5 };
  for (int i = 0; i < 3; i++) {
    lv_obj_t *btn = lv_btn_create(cardVol);
    lv_obj_set_size(btn, 88, 38);
    lv_obj_set_pos(btn, i * 94, 46);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1F2636), 0);
    lv_obj_set_style_border_color(btn, COLOR_PANEL_BORDER, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_user_data(btn, (void*)(intptr_t)volBtnIndices[i]);
    lv_obj_add_event_cb(btn, onMediaBtnEvent, LV_EVENT_CLICKED, this);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_obj_set_style_text_font(lbl, &vi_icon_font_20, 0);
    lv_obj_set_style_text_color(lbl, (i == 1) ? COLOR_WARN_AMBER : COLOR_TEXT_WHITE, 0);
    lv_label_set_text(lbl, volBtnLabels[i]);
    lv_obj_center(lbl);
  }

  // -------------------------------------------------------------
  // Card 2: Media Playback (Prev, Play/Pause, Next, Stop)
  // -------------------------------------------------------------
  lv_obj_t *cardPlay = lv_obj_create(_tabMedia);
  lv_obj_set_size(cardPlay, 304, 72);
  lv_obj_align(cardPlay, LV_ALIGN_TOP_MID, 0, 112);
  lv_obj_set_style_bg_color(cardPlay, COLOR_PANEL_BG, 0);
  lv_obj_set_style_border_color(cardPlay, COLOR_PANEL_BORDER, 0);
  lv_obj_set_style_border_width(cardPlay, 1, 0);
  lv_obj_set_style_radius(cardPlay, 10, 0);
  lv_obj_set_style_pad_all(cardPlay, 8, 0);
  lv_obj_clear_flag(cardPlay, LV_OBJ_FLAG_SCROLLABLE);

  const char *mediaIcons[4] = { ICON_PREV, ICON_PLAY, ICON_NEXT, ICON_STOP };
  for (int i = 0; i < 4; i++) {
    lv_obj_t *btn = lv_btn_create(cardPlay);
    lv_obj_set_size(btn, 66, 48);
    lv_obj_set_pos(btn, i * 74, 4);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1F2636), 0);
    lv_obj_set_style_border_color(btn, (i == 1) ? COLOR_ACTIVE_BLUE : COLOR_PANEL_BORDER, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_user_data(btn, (void*)(intptr_t)i);
    lv_obj_add_event_cb(btn, onMediaBtnEvent, LV_EVENT_CLICKED, this);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_obj_set_style_text_font(lbl, &vi_icon_font_24, 0);
    lv_obj_set_style_text_color(lbl, (i == 1) ? COLOR_ACTIVE_BLUE : COLOR_TEXT_WHITE, 0);
    lv_label_set_text(lbl, mediaIcons[i]);
    lv_obj_center(lbl);
  }

  // -------------------------------------------------------------
  // Card 3: Điều hướng & Tua nhanh (D-Pad & Space)
  // -------------------------------------------------------------
  lv_obj_t *cardDpad = lv_obj_create(_tabMedia);
  lv_obj_set_size(cardDpad, 304, 230);
  lv_obj_align(cardDpad, LV_ALIGN_TOP_MID, 0, 192);
  lv_obj_set_style_bg_color(cardDpad, COLOR_PANEL_BG, 0);
  lv_obj_set_style_border_color(cardDpad, COLOR_PANEL_BORDER, 0);
  lv_obj_set_style_border_width(cardDpad, 1, 0);
  lv_obj_set_style_radius(cardDpad, 10, 0);
  lv_obj_set_style_pad_all(cardDpad, 8, 0);
  lv_obj_clear_flag(cardDpad, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *lblDpad = lv_label_create(cardDpad);
  lv_obj_set_style_text_font(lblDpad, &vi_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lblDpad, COLOR_TEXT_MUTED, 0);
  lv_label_set_text(lblDpad, "ĐIỀU HƯỚNG & TUA NHANH");
  lv_obj_align(lblDpad, LV_ALIGN_TOP_LEFT, 0, 0);

  struct DpadItem {
    int x;
    int y;
    int w;
    int h;
    const char *label;
    const lv_font_t *font;
    int keyIdx; // 0: UP, 1: DOWN, 2: LEFT, 3: RIGHT, 4: SPACE
  };

  const DpadItem dpadItems[5] = {
    { 112, 28,  64, 44, ICON_ARROW_UP,                  &vi_icon_font_24, 0 },
    { 112, 138, 64, 44, ICON_ARROW_DOWN,                &vi_icon_font_24, 1 },
    { 42,  83,  64, 44, LV_SYMBOL_LEFT " 5s",           &vi_font_montserrat_14, 2 },
    { 182, 83,  64, 44, "5s " LV_SYMBOL_RIGHT,          &vi_font_montserrat_14, 3 },
    { 112, 83,  64, 44, LV_SYMBOL_PLAY " " LV_SYMBOL_PAUSE, &vi_font_montserrat_14, 4 }
  };

  for (int i = 0; i < 5; i++) {
    lv_obj_t *btn = lv_btn_create(cardDpad);
    lv_obj_set_size(btn, dpadItems[i].w, dpadItems[i].h);
    lv_obj_set_pos(btn, dpadItems[i].x, dpadItems[i].y);
    lv_obj_set_style_bg_color(btn, (i == 4) ? COLOR_PRIMARY_BLUE : lv_color_hex(0x1F2636), 0);
    lv_obj_set_style_border_color(btn, COLOR_PANEL_BORDER, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_user_data(btn, (void*)(intptr_t)dpadItems[i].keyIdx);
    lv_obj_add_event_cb(btn, onArrowKeyBtnEvent, LV_EVENT_CLICKED, this);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_obj_set_style_text_font(lbl, dpadItems[i].font, 0);
    lv_obj_set_style_text_color(lbl, COLOR_TEXT_WHITE, 0);
    lv_label_set_text(lbl, dpadItems[i].label);
    lv_obj_center(lbl);
  }
}

void UiManager::setupTabSettings() {
  lv_obj_set_style_pad_all(_tabSettings, 8, 0);
  lv_obj_clear_flag(_tabSettings, LV_OBJ_FLAG_SCROLLABLE);

  // Card 1: Thông tin phần cứng & hệ thống (Portrait 304 x 130)
  lv_obj_t *cardInfo = lv_obj_create(_tabSettings);
  lv_obj_set_size(cardInfo, 304, 130);
  lv_obj_align(cardInfo, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(cardInfo, COLOR_PANEL_BG, 0);
  lv_obj_set_style_border_color(cardInfo, COLOR_PANEL_BORDER, 0);
  lv_obj_set_style_border_width(cardInfo, 1, 0);
  lv_obj_set_style_radius(cardInfo, 10, 0);
  lv_obj_clear_flag(cardInfo, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *lblInfoTitle = lv_label_create(cardInfo);
  lv_obj_set_style_text_font(lblInfoTitle, &vi_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lblInfoTitle, COLOR_ACTIVE_BLUE, 0);
  lv_label_set_text(lblInfoTitle, "HỆ THỐNG ESP32-S3");
  lv_obj_align(lblInfoTitle, LV_ALIGN_TOP_LEFT, 0, 0);

  lv_obj_t *lblSys = lv_label_create(cardInfo);
  lv_obj_set_style_text_font(lblSys, &vi_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lblSys, COLOR_TEXT_MUTED, 0);
  lv_label_set_text(lblSys,
    "MCU: Dual-Core @ 240MHz\n"
    "RAM: 8MB PSRAM | Flash: 16MB\n"
    "Màn hình: 3.5\" IPS 320x480\n"
    "BLE HID: ESP32-S3 TouchPad");
  lv_obj_align(lblSys, LV_ALIGN_TOP_LEFT, 0, 24);

  // Card 2: Cài đặt hệ điều hành & âm thanh (Portrait 304 x 230)
  lv_obj_t *cardCtrl = lv_obj_create(_tabSettings);
  lv_obj_set_size(cardCtrl, 304, 230);
  lv_obj_align(cardCtrl, LV_ALIGN_TOP_MID, 0, 140);
  lv_obj_set_style_bg_color(cardCtrl, COLOR_PANEL_BG, 0);
  lv_obj_set_style_border_color(cardCtrl, COLOR_PANEL_BORDER, 0);
  lv_obj_set_style_border_width(cardCtrl, 1, 0);
  lv_obj_set_style_radius(cardCtrl, 10, 0);
  lv_obj_clear_flag(cardCtrl, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *lblCtrlTitle = lv_label_create(cardCtrl);
  lv_obj_set_style_text_font(lblCtrlTitle, &vi_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lblCtrlTitle, COLOR_SUCCESS_GREEN, 0);
  lv_label_set_text(lblCtrlTitle, "CẤU HÌNH ĐIỀU KHIỂN");
  lv_obj_align(lblCtrlTitle, LV_ALIGN_TOP_LEFT, 0, 0);

  // Nút chuyển OS Mode (Windows / macOS)
  _osBtn = lv_btn_create(cardCtrl);
  lv_obj_set_size(_osBtn, 272, 48);
  lv_obj_align(_osBtn, LV_ALIGN_TOP_MID, 0, 30);
  lv_obj_set_style_bg_color(_osBtn, lv_color_hex(0x1F2636), 0);
  lv_obj_set_style_border_color(_osBtn, COLOR_PANEL_BORDER, 0);
  lv_obj_set_style_border_width(_osBtn, 1, 0);
  lv_obj_set_style_radius(_osBtn, 8, 0);
  lv_obj_add_event_cb(_osBtn, onOsToggleEvent, LV_EVENT_CLICKED, this);

  _osLabel = lv_label_create(_osBtn);
  lv_obj_set_style_text_font(_osLabel, &vi_font_montserrat_14, 0);
  lv_obj_set_style_text_color(_osLabel, COLOR_TEXT_WHITE, 0);
  lv_label_set_text(_osLabel, "Hệ điều hành: WINDOWS (Ctrl)");
  lv_obj_center(_osLabel);

  // Nút bật/tắt âm thanh phản hồi
  _soundBtn = lv_btn_create(cardCtrl);
  lv_obj_set_size(_soundBtn, 272, 48);
  lv_obj_align(_soundBtn, LV_ALIGN_TOP_MID, 0, 88);
  lv_obj_set_style_bg_color(_soundBtn, lv_color_hex(0x1F2636), 0);
  lv_obj_set_style_border_color(_soundBtn, COLOR_PANEL_BORDER, 0);
  lv_obj_set_style_border_width(_soundBtn, 1, 0);
  lv_obj_set_style_radius(_soundBtn, 8, 0);
  lv_obj_add_event_cb(_soundBtn, onSoundToggleEvent, LV_EVENT_CLICKED, this);

  _soundLabel = lv_label_create(_soundBtn);
  lv_obj_set_style_text_font(_soundLabel, &vi_font_montserrat_14, 0);
  lv_obj_set_style_text_color(_soundLabel, COLOR_SUCCESS_GREEN, 0);
  lv_label_set_text(_soundLabel, "Âm phản hồi: BẬT");
  lv_obj_center(_soundLabel);

  // Nhãn trạng thái Bluetooth
  _bleLabel = lv_label_create(cardCtrl);
  lv_obj_set_style_text_font(_bleLabel, &vi_font_montserrat_12, 0);
  lv_obj_set_style_text_color(_bleLabel, COLOR_TEXT_MUTED, 0);
  lv_label_set_text(_bleLabel, "BLE: Đang chờ kết nối...");
  lv_obj_align(_bleLabel, LV_ALIGN_BOTTOM_LEFT, 0, -5);
}

void UiManager::setBleConnected(bool connected, const String &peerName) {
  _connected = connected;
  _peerName = peerName;

  if (_connected) {
    if (lv_scr_act() != _mainScreen) {
      lv_scr_load_anim(_mainScreen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
    }
    if (_bleLabel != nullptr) {
      lv_obj_set_style_text_color(_bleLabel, COLOR_SUCCESS_GREEN, 0);
      lv_label_set_text(_bleLabel, "BLE: ĐÃ KẾT NỐI");
    }
  } else {
    if (lv_scr_act() != _pairingScreen) {
      lv_scr_load_anim(_pairingScreen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
    }
    if (_pairingStatusLabel != nullptr) {
      lv_obj_set_style_text_color(_pairingStatusLabel, COLOR_WARN_AMBER, 0);
      lv_label_set_text(_pairingStatusLabel, "Chờ ghép nối Bluetooth...");
    }
  }
}

void UiManager::setBatteryLevel(uint8_t level) {
  _batteryLevel = (level > 100) ? 100 : level;
}

void UiManager::syncVolumeFromRemote(uint8_t volumePercent) {
  if (volumePercent > 100) volumePercent = 100;
  _isSyncingVolume = true;
  _lastVolumeSliderVal = volumePercent;
  if (_volSlider != nullptr) {
    lv_slider_set_value(_volSlider, volumePercent, LV_ANIM_ON);
  }
  if (_volLabel != nullptr) {
    char buf[32];
    snprintf(buf, sizeof(buf), "ÂM LƯỢNG HỆ THỐNG: %d%%", volumePercent);
    lv_label_set_text(_volLabel, buf);
  }
  _isSyncingVolume = false;
  LOG_I(TAG, "Dong bo am luong tu Companion App thanh cong: %d%%", volumePercent);
}

void UiManager::toggleOsMode() {
  if (_osMode == OsMode::WINDOWS) {
    _osMode = OsMode::MACOS;
    if (_osLabel != nullptr) {
      lv_label_set_text(_osLabel, "Hệ điều hành: macOS (Cmd)");
    }
    LOG_I(TAG, "Chuyen che do he dieu hanh sang: macOS (Cmd)");
  } else {
    _osMode = OsMode::WINDOWS;
    if (_osLabel != nullptr) {
      lv_label_set_text(_osLabel, "Hệ điều hành: WINDOWS (Ctrl)");
    }
    LOG_I(TAG, "Chuyen che do he dieu hanh sang: Windows (Ctrl)");
  }
}

void UiManager::toggleSoundFeedback() {
  _soundEnabled = !_soundEnabled;
  if (_soundLabel != nullptr) {
    if (_soundEnabled) {
      lv_obj_set_style_text_color(_soundLabel, COLOR_SUCCESS_GREEN, 0);
      lv_label_set_text(_soundLabel, "Âm phản hồi: BẬT");
      LOG_I(TAG, "Am thanh phan hoi: BAT");
    } else {
      lv_obj_set_style_text_color(_soundLabel, COLOR_TEXT_MUTED, 0);
      lv_label_set_text(_soundLabel, "Âm phản hồi: TẮT");
      LOG_I(TAG, "Am thanh phan hoi: TAT");
    }
  }
}

// Biến hỗ trợ Trackpad Touch Gesture
static lv_point_t s_lastTouchPoint = {0, 0};
static lv_point_t s_startTouchPoint = {0, 0};
static uint32_t s_touchStartTime = 0;
static bool s_isDragging = false;

void UiManager::onTrackpadEvent(lv_event_t *e) {
  UiManager *mgr = (UiManager *)lv_event_get_user_data(e);
  if (mgr == nullptr || mgr->_hidService == nullptr || !mgr->_connected) return;

  lv_event_code_t code = lv_event_get_code(e);
  lv_indev_t *indev = lv_indev_get_act();
  if (!indev) return;

  lv_point_t p;
  lv_indev_get_point(indev, &p);

  if (code == LV_EVENT_PRESSED) {
    s_startTouchPoint = p;
    s_lastTouchPoint = p;
    s_touchStartTime = millis();
    s_isDragging = false;
  } else if (code == LV_EVENT_PRESSING) {
    int32_t dx = p.x - s_lastTouchPoint.x;
    int32_t dy = p.y - s_lastTouchPoint.y;

    if (abs(dx) > 0 || abs(dy) > 0) {
      s_isDragging = true;
      if (dx > 127) dx = 127;
      if (dx < -127) dx = -127;
      if (dy > 127) dy = 127;
      if (dy < -127) dy = -127;

      mgr->_hidService->mouseMove((int8_t)dx, (int8_t)dy, 0);
      s_lastTouchPoint = p;
    }
  } else if (code == LV_EVENT_RELEASED) {
    uint32_t pressDuration = millis() - s_touchStartTime;
    int32_t totalDistX = abs(p.x - s_startTouchPoint.x);
    int32_t totalDistY = abs(p.y - s_startTouchPoint.y);

    // Single Tap < 250ms thành Left Click
    if (!s_isDragging && pressDuration < 250 && totalDistX < 10 && totalDistY < 10) {
      mgr->_hidService->mouseClick(MOUSE_BUTTON_LEFT);
      LOG_I(TAG, "-> Trackpad Tap: Left Click");
    }
  }
}

void UiManager::onLeftClickEvent(lv_event_t *e) {
  UiManager *mgr = (UiManager *)lv_event_get_user_data(e);
  if (mgr != nullptr && mgr->_hidService != nullptr && mgr->_connected) {
    mgr->_hidService->mouseClick(MOUSE_BUTTON_LEFT);
    LOG_I(TAG, "Nut bam: Chuot Trai");
  }
}

void UiManager::onRightClickEvent(lv_event_t *e) {
  UiManager *mgr = (UiManager *)lv_event_get_user_data(e);
  if (mgr != nullptr && mgr->_hidService != nullptr && mgr->_connected) {
    mgr->_hidService->mouseClick(MOUSE_BUTTON_RIGHT);
    LOG_I(TAG, "Nut bam: Chuot Phai");
  }
}

void UiManager::onScrollUpEvent(lv_event_t *e) {
  UiManager *mgr = (UiManager *)lv_event_get_user_data(e);
  if (mgr != nullptr && mgr->_hidService != nullptr && mgr->_connected) {
    mgr->_hidService->mouseMove(0, 0, 1);
    LOG_I(TAG, "Nut bam: Cuon Len (Scroll Up)");
  }
}

void UiManager::onScrollDownEvent(lv_event_t *e) {
  UiManager *mgr = (UiManager *)lv_event_get_user_data(e);
  if (mgr != nullptr && mgr->_hidService != nullptr && mgr->_connected) {
    mgr->_hidService->mouseMove(0, 0, -1);
    LOG_I(TAG, "Nut bam: Cuon Xuong (Scroll Down)");
  }
}

void UiManager::onShortcutBtnEvent(lv_event_t *e) {
  UiManager *mgr = (UiManager *)lv_event_get_user_data(e);
  if (mgr == nullptr || mgr->_hidService == nullptr || !mgr->_connected) return;

  lv_obj_t *btn = lv_event_get_target(e);
  int idx = (int)(intptr_t)lv_obj_get_user_data(btn);

  uint8_t modCtrl = (mgr->_osMode == OsMode::MACOS) ? KEY_MOD_LMETA : KEY_MOD_LCTRL;

  switch (idx) {
    // --- Hàng 1: Hệ thống & Tiện ích ---
    case 0: {
      // Lock: Win+L / Cmd+Ctrl+Q
      if (mgr->_osMode == OsMode::MACOS) {
        mgr->_hidService->keyboardWrite(HID_KEY_Q, KEY_MOD_LMETA | KEY_MOD_LCTRL);
      } else {
        mgr->_hidService->keyboardWrite(HID_KEY_L, KEY_MOD_LMETA);
      }
      LOG_I(TAG, "Shortcut: Lock Screen");
      break;
    }
    case 1: {
      // Screenshot / Snip: Win+Shift+S / Cmd+Shift+4
      if (mgr->_osMode == OsMode::MACOS) {
        mgr->_hidService->keyboardWrite(HID_KEY_4, KEY_MOD_LMETA | KEY_MOD_LSHIFT);
      } else {
        mgr->_hidService->keyboardWrite(HID_KEY_S, KEY_MOD_LMETA | KEY_MOD_LSHIFT);
      }
      LOG_I(TAG, "Shortcut: Screenshot / Snipping Tool");
      break;
    }
    case 2: {
      // Task Manager: Ctrl+Shift+Esc / Cmd+Space
      if (mgr->_osMode == OsMode::MACOS) {
        mgr->_hidService->keyboardWrite(HID_KEY_SPACE, KEY_MOD_LMETA);
      } else {
        mgr->_hidService->keyboardWrite(HID_KEY_ESC, KEY_MOD_LCTRL | KEY_MOD_LSHIFT);
      }
      LOG_I(TAG, "Shortcut: Task Manager");
      break;
    }
    case 3: {
      // Calculator
      mgr->_hidService->mediaKeyWrite(MEDIA_KEY_CALCULATOR);
      LOG_I(TAG, "Shortcut: Calculator");
      break;
    }

    // --- Hàng 2: Trình duyệt Web ---
    case 4: {
      // Back: Alt+Left / Cmd+[
      if (mgr->_osMode == OsMode::MACOS) {
        mgr->_hidService->keyboardWrite(HID_KEY_LEFTBRACE, KEY_MOD_LMETA);
      } else {
        mgr->_hidService->keyboardWrite(HID_KEY_LEFT, KEY_MOD_LALT);
      }
      LOG_I(TAG, "Shortcut: Browser Back");
      break;
    }
    case 5: {
      // Forward: Alt+Right / Cmd+]
      if (mgr->_osMode == OsMode::MACOS) {
        mgr->_hidService->keyboardWrite(HID_KEY_RIGHTBRACE, KEY_MOD_LMETA);
      } else {
        mgr->_hidService->keyboardWrite(HID_KEY_RIGHT, KEY_MOD_LALT);
      }
      LOG_I(TAG, "Shortcut: Browser Forward");
      break;
    }
    case 6: {
      // Reload: F5 / Cmd+R
      if (mgr->_osMode == OsMode::MACOS) {
        mgr->_hidService->keyboardWrite(HID_KEY_R, KEY_MOD_LMETA);
      } else {
        mgr->_hidService->keyboardWrite(HID_KEY_F5);
      }
      LOG_I(TAG, "Shortcut: Reload");
      break;
    }
    case 7: {
      // New Tab: Ctrl+T / Cmd+T
      mgr->_hidService->keyboardWrite(HID_KEY_T, modCtrl);
      LOG_I(TAG, "Shortcut: New Tab");
      break;
    }

    // --- Hàng 3: Soạn thảo ---
    case 8:  mgr->_hidService->keyboardWrite(HID_KEY_C, modCtrl); LOG_I(TAG, "Shortcut: Copy"); break;
    case 9:  mgr->_hidService->keyboardWrite(HID_KEY_V, modCtrl); LOG_I(TAG, "Shortcut: Paste"); break;
    case 10: mgr->_hidService->keyboardWrite(HID_KEY_X, modCtrl); LOG_I(TAG, "Shortcut: Cut"); break;
    case 11: mgr->_hidService->keyboardWrite(HID_KEY_Z, modCtrl); LOG_I(TAG, "Shortcut: Undo"); break;

    // --- Hàng 4: Điều khiển & Hành động ---
    case 12: mgr->_hidService->keyboardWrite(HID_KEY_A, modCtrl); LOG_I(TAG, "Shortcut: Select All"); break;
    case 13: mgr->_hidService->keyboardWrite(HID_KEY_S, modCtrl); LOG_I(TAG, "Shortcut: Save"); break;
    case 14: {
      // Esc
      mgr->_hidService->keyboardWrite(HID_KEY_ESC);
      LOG_I(TAG, "Shortcut: Esc");
      break;
    }
    case 15: {
      // Enter
      mgr->_hidService->keyboardWrite(HID_KEY_ENTER);
      LOG_I(TAG, "Shortcut: Enter");
      break;
    }
  }
}

void UiManager::onNumpadBtnEvent(lv_event_t *e) {
  UiManager *mgr = (UiManager *)lv_event_get_user_data(e);
  if (mgr == nullptr || mgr->_hidService == nullptr || !mgr->_connected) return;

  lv_obj_t *btn = lv_event_get_target(e);
  const char *key = (const char *)lv_obj_get_user_data(btn);
  if (key == nullptr) return;

  if (strcmp(key, "ENTER") == 0) {
    mgr->_hidService->keyboardWrite(HID_KEY_ENTER);
  } else if (strcmp(key, "BACKSPACE") == 0) {
    mgr->_hidService->keyboardWrite(HID_KEY_BACKSPACE);
  } else {
    mgr->_hidService->keyboardPrint(key);
  }
}

void UiManager::onMediaBtnEvent(lv_event_t *e) {
  UiManager *mgr = (UiManager *)lv_event_get_user_data(e);
  if (mgr == nullptr || mgr->_hidService == nullptr || !mgr->_connected) return;

  lv_obj_t *btn = lv_event_get_target(e);
  int idx = (int)(intptr_t)lv_obj_get_user_data(btn);

  switch (idx) {
    case 0: mgr->_hidService->mediaKeyWrite(MEDIA_KEY_PREV);       LOG_I(TAG, "Media: Prev Track"); break;
    case 1: mgr->_hidService->mediaKeyWrite(MEDIA_KEY_PLAY_PAUSE); LOG_I(TAG, "Media: Play/Pause"); break;
    case 2: mgr->_hidService->mediaKeyWrite(MEDIA_KEY_NEXT);       LOG_I(TAG, "Media: Next Track"); break;
    case 3: mgr->_hidService->mediaKeyWrite(MEDIA_KEY_STOP);       LOG_I(TAG, "Media: Stop"); break;
    case 4: mgr->_hidService->mediaKeyWrite(MEDIA_KEY_VOL_DOWN);   LOG_I(TAG, "Media: Vol Down"); break;
    case 5: mgr->_hidService->mediaKeyWrite(MEDIA_KEY_VOL_UP);     LOG_I(TAG, "Media: Vol Up"); break;
    case 6: mgr->_hidService->mediaKeyWrite(MEDIA_KEY_MUTE);       LOG_I(TAG, "Media: Mute"); break;
  }
}

void UiManager::onVolumeSliderEvent(lv_event_t *e) {
  lv_event_stop_bubbling(e);
  UiManager *mgr = (UiManager *)lv_event_get_user_data(e);
  if (mgr == nullptr || mgr->_hidService == nullptr || !mgr->_connected) return;

  // Nếu đang trong tiến trình đồng bộ từ Companion App thì không gửi ngược lại
  if (mgr->_isSyncingVolume) return;

  lv_obj_t *slider = lv_event_get_target(e);
  int32_t val = lv_slider_get_value(slider);
  if (val > 100) val = 100;
  if (val < 0) val = 0;

  // Cập nhật nhãn % hiển thị trực quan
  if (mgr->_volLabel != nullptr) {
    char buf[32];
    snprintf(buf, sizeof(buf), "ÂM LƯỢNG HỆ THỐNG: %d%%", (int)val);
    lv_label_set_text(mgr->_volLabel, buf);
  }

  // 1. Gửi giá trị tuyệt đối qua Custom GATT Service cho Companion App
  mgr->_hidService->setRemoteVolume((uint8_t)val);

  // 2. Dự phòng: gửi chuỗi xung HID Consumer Control nếu Companion App chưa chạy
  int32_t delta = val - mgr->_lastVolumeSliderVal;
  if (abs(delta) >= 2) {
    if (delta > 0) {
      int steps = delta / 2;
      if (steps > 5) steps = 5;
      for (int i = 0; i < steps; i++) {
        mgr->_hidService->mediaKeyWrite(MEDIA_KEY_VOL_UP);
      }
    } else {
      int steps = (-delta) / 2;
      if (steps > 5) steps = 5;
      for (int i = 0; i < steps; i++) {
        mgr->_hidService->mediaKeyWrite(MEDIA_KEY_VOL_DOWN);
      }
    }
    mgr->_lastVolumeSliderVal = val;
  }
}

void UiManager::onArrowKeyBtnEvent(lv_event_t *e) {
  UiManager *mgr = (UiManager *)lv_event_get_user_data(e);
  if (mgr == nullptr || mgr->_hidService == nullptr || !mgr->_connected) return;

  lv_obj_t *btn = lv_event_get_target(e);
  int keyIdx = (int)(intptr_t)lv_obj_get_user_data(btn);

  switch (keyIdx) {
    case 0: mgr->_hidService->keyboardWrite(HID_KEY_UP);    LOG_I(TAG, "D-Pad: Arrow UP"); break;
    case 1: mgr->_hidService->keyboardWrite(HID_KEY_DOWN);  LOG_I(TAG, "D-Pad: Arrow DOWN"); break;
    case 2: mgr->_hidService->keyboardWrite(HID_KEY_LEFT);  LOG_I(TAG, "D-Pad: Arrow LEFT"); break;
    case 3: mgr->_hidService->keyboardWrite(HID_KEY_RIGHT); LOG_I(TAG, "D-Pad: Arrow RIGHT"); break;
    case 4: mgr->_hidService->keyboardWrite(HID_KEY_SPACE); LOG_I(TAG, "D-Pad: SPACE / PLAY-PAUSE"); break;
  }
}

void UiManager::onOsToggleEvent(lv_event_t *e) {
  UiManager *mgr = (UiManager *)lv_event_get_user_data(e);
  if (mgr != nullptr) {
    mgr->toggleOsMode();
  }
}

void UiManager::onSoundToggleEvent(lv_event_t *e) {
  UiManager *mgr = (UiManager *)lv_event_get_user_data(e);
  if (mgr != nullptr) {
    mgr->toggleSoundFeedback();
  }
}
