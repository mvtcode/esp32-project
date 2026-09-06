#ifndef DIALOG_MANAGER_H
#define DIALOG_MANAGER_H

#include <lvgl.h>
#include <Arduino.h>

class DialogManager {
public:
    // 1. Toast Notification (tự động biến mất sau durationMs)
    static void showToast(const char* message, uint32_t durationMs = 2500);

    // 2. Alert Dialog (1 nút hành động)
    static void showAlert(const char* title, const char* message,
                          const char* btnText = "Đóng",
                          lv_event_cb_t btnCb = nullptr,
                          void* userData = nullptr,
                          lv_color_t themeColor = lv_color_make(255, 170, 40));

    // 3. Confirm Dialog (2 nút: Hủy & Xác nhận)
    static void showConfirm(const char* title, const char* message,
                            const char* confirmText = "Xác Nhận",
                            lv_event_cb_t confirmCb = nullptr,
                            void* userData = nullptr,
                            const char* cancelText = "Hủy Bỏ",
                            lv_event_cb_t cancelCb = nullptr,
                            lv_color_t confirmColor = lv_color_make(180, 40, 50));

    // 4. Lock Overlay (Khóa màn hình, chặn hoàn toàn cảm ứng)
    static void showLockOverlay(const char* title, const char* message,
                                const char* hint = "Màn hình tạm khóa cho đến khi xong...",
                                const char* symbol = LV_SYMBOL_TRASH,
                                lv_color_t badgeColor = lv_color_make(180, 40, 50));
    static void hideLockOverlay();

    // 5. Quản lý trạng thái & Dọn dẹp
    static void dismissModal();
    static bool isModalActive();
    static bool isLocked();
    static void cleanupAll();

private:
    static lv_obj_t* modalBackdrop;
    static lv_obj_t* lockBackdrop;
    static lv_obj_t* toastContainer;
    static lv_timer_t* toastTimer;

    static void toast_timer_cb(lv_timer_t* t);
    static void default_dismiss_cb(lv_event_t* e);
};

#endif // DIALOG_MANAGER_H
