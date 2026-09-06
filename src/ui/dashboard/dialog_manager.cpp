#include "dialog_manager.h"
#include "cyd_theme.h"
#include "log.h"

lv_obj_t* DialogManager::modalBackdrop = nullptr;
lv_obj_t* DialogManager::lockBackdrop = nullptr;
lv_obj_t* DialogManager::lockCard = nullptr;
lv_obj_t* DialogManager::lockLblTitle = nullptr;
lv_obj_t* DialogManager::lockLblSub = nullptr;
lv_obj_t* DialogManager::lockLblHint = nullptr;
lv_obj_t* DialogManager::lockBar = nullptr;
lv_obj_t* DialogManager::toastContainer = nullptr;
lv_timer_t* DialogManager::toastTimer = nullptr;

void DialogManager::showToast(const char* message, uint32_t durationMs) {
    if (toastTimer) {
        lv_timer_del(toastTimer);
        toastTimer = nullptr;
    }
    if (toastContainer) {
        lv_obj_del(toastContainer);
        toastContainer = nullptr;
    }

    toastContainer = lv_obj_create(lv_layer_top());
    lv_obj_set_size(toastContainer, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(toastContainer, lv_color_make(18, 26, 44), 0);
    lv_obj_set_style_bg_opa(toastContainer, LV_OPA_90, 0);
    lv_obj_set_style_border_color(toastContainer, CydTheme::getAccentColor(), 0);
    lv_obj_set_style_border_width(toastContainer, 1, 0);
    lv_obj_set_style_radius(toastContainer, 8, 0);
    lv_obj_set_style_pad_hor(toastContainer, 16, 0);
    lv_obj_set_style_pad_ver(toastContainer, 8, 0);
    lv_obj_set_style_shadow_width(toastContainer, 14, 0);
    lv_obj_set_style_shadow_color(toastContainer, lv_color_black(), 0);
    lv_obj_set_style_shadow_opa(toastContainer, LV_OPA_70, 0);
    lv_obj_clear_flag(toastContainer, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_align(toastContainer, LV_ALIGN_TOP_MID, 0, 42);

    lv_obj_t* lbl = lv_label_create(toastContainer);
    lv_label_set_text(lbl, message);
    CydTheme::applyTextFont(lbl, CydTheme::getFont12(), CydTheme::getWhiteColor());
    lv_obj_center(lbl);

    toastTimer = lv_timer_create(toast_timer_cb, durationMs, nullptr);
    lv_timer_set_repeat_count(toastTimer, 1);
}

void DialogManager::toast_timer_cb(lv_timer_t* t) {
    if (toastContainer) {
        lv_obj_del(toastContainer);
        toastContainer = nullptr;
    }
    toastTimer = nullptr;
}

void DialogManager::showAlert(const char* title, const char* message,
                             const char* btnText, lv_event_cb_t btnCb,
                             void* userData, lv_color_t themeColor) {
    dismissModal();

    modalBackdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_size(modalBackdrop, 480, 320);
    lv_obj_align(modalBackdrop, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(modalBackdrop, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(modalBackdrop, LV_OPA_70, 0);
    lv_obj_set_style_border_width(modalBackdrop, 0, 0);
    lv_obj_set_style_pad_all(modalBackdrop, 0, 0);
    lv_obj_clear_flag(modalBackdrop, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(modalBackdrop, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t* card = lv_obj_create(modalBackdrop);
    lv_obj_set_size(card, 380, 190);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(card, lv_color_make(18, 24, 40), 0);
    lv_obj_set_style_border_color(card, themeColor, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t* lblTitle = lv_label_create(card);
    lv_label_set_text(lblTitle, title);
    CydTheme::applyTextFont(lblTitle, CydTheme::getFont14(), themeColor);
    lv_obj_align(lblTitle, LV_ALIGN_TOP_MID, 0, 4);

    // Message
    lv_obj_t* lblMsg = lv_label_create(card);
    lv_label_set_text(lblMsg, message);
    CydTheme::applyTextFont(lblMsg, CydTheme::getFont12(), CydTheme::getWhiteColor());
    lv_label_set_long_mode(lblMsg, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lblMsg, 350);
    lv_obj_set_style_text_align(lblMsg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lblMsg, LV_ALIGN_TOP_MID, 0, 36);

    // Action Button
    lv_obj_t* btn = lv_btn_create(card);
    lv_obj_set_size(btn, 200, 38);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_style_bg_color(btn, CydTheme::getAccentColor(), 0);
    lv_obj_set_style_radius(btn, 6, 0);
    if (btnCb) {
        lv_obj_add_event_cb(btn, btnCb, LV_EVENT_CLICKED, userData);
    } else {
        lv_obj_add_event_cb(btn, default_dismiss_cb, LV_EVENT_CLICKED, nullptr);
    }

    lv_obj_t* lblBtn = lv_label_create(btn);
    lv_label_set_text(lblBtn, btnText ? btnText : "Đóng");
    CydTheme::applyTextFont(lblBtn, CydTheme::getFont12(), CydTheme::getWhiteColor());
    lv_obj_center(lblBtn);
}

void DialogManager::showConfirm(const char* title, const char* message,
                               const char* confirmText, lv_event_cb_t confirmCb, void* userData,
                               const char* cancelText, lv_event_cb_t cancelCb,
                               lv_color_t confirmColor) {
    dismissModal();

    modalBackdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_size(modalBackdrop, 480, 320);
    lv_obj_align(modalBackdrop, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(modalBackdrop, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(modalBackdrop, LV_OPA_70, 0);
    lv_obj_set_style_border_width(modalBackdrop, 0, 0);
    lv_obj_set_style_pad_all(modalBackdrop, 0, 0);
    lv_obj_clear_flag(modalBackdrop, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(modalBackdrop, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t* card = lv_obj_create(modalBackdrop);
    lv_obj_set_size(card, 360, 165);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(card, lv_color_make(18, 24, 40), 0);
    lv_obj_set_style_border_color(card, confirmColor, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_pad_all(card, 14, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lblTitle = lv_label_create(card);
    lv_label_set_text(lblTitle, title);
    CydTheme::applyTextFont(lblTitle, CydTheme::getFont14(), confirmColor);
    lv_obj_align(lblTitle, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t* lblMsg = lv_label_create(card);
    lv_label_set_text(lblMsg, message);
    CydTheme::applyTextFont(lblMsg, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblMsg, LV_ALIGN_TOP_LEFT, 0, 28);

    // Cancel Button
    lv_obj_t* btnCancel = lv_btn_create(card);
    lv_obj_set_size(btnCancel, 140, 36);
    lv_obj_align(btnCancel, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_color(btnCancel, lv_color_make(45, 55, 75), 0);
    lv_obj_set_style_radius(btnCancel, 6, 0);
    if (cancelCb) {
        lv_obj_add_event_cb(btnCancel, cancelCb, LV_EVENT_CLICKED, userData);
    } else {
        lv_obj_add_event_cb(btnCancel, default_dismiss_cb, LV_EVENT_CLICKED, nullptr);
    }
    lv_obj_t* lblCancel = lv_label_create(btnCancel);
    lv_label_set_text(lblCancel, cancelText ? cancelText : (LV_SYMBOL_CLOSE " Hủy Bỏ"));
    CydTheme::applyTextFont(lblCancel, CydTheme::getFont12(), CydTheme::getWhiteColor());
    lv_obj_center(lblCancel);

    // Confirm Button
    lv_obj_t* btnConfirm = lv_btn_create(card);
    lv_obj_set_size(btnConfirm, 160, 36);
    lv_obj_align(btnConfirm, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(btnConfirm, confirmColor, 0);
    lv_obj_set_style_radius(btnConfirm, 6, 0);
    if (confirmCb) {
        lv_obj_add_event_cb(btnConfirm, confirmCb, LV_EVENT_CLICKED, userData);
    }
    lv_obj_t* lblConfirm = lv_label_create(btnConfirm);
    lv_label_set_text(lblConfirm, confirmText ? confirmText : (LV_SYMBOL_OK " Xác Nhận"));
    CydTheme::applyTextFont(lblConfirm, CydTheme::getFont12(), CydTheme::getWhiteColor());
    lv_obj_center(lblConfirm);
}

void DialogManager::showLockOverlay(const char* title, const char* message,
                                   const char* hint, const char* symbol,
                                   lv_color_t badgeColor) {
    hideLockOverlay();

    lockBackdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_size(lockBackdrop, 480, 320);
    lv_obj_align(lockBackdrop, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(lockBackdrop, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(lockBackdrop, LV_OPA_80, 0);
    lv_obj_set_style_border_width(lockBackdrop, 0, 0);
    lv_obj_set_style_pad_all(lockBackdrop, 0, 0);
    lv_obj_add_flag(lockBackdrop, LV_OBJ_FLAG_CLICKABLE); // Chặn hoàn toàn tất cả sự kiện chạm!
    lv_obj_clear_flag(lockBackdrop, LV_OBJ_FLAG_SCROLLABLE);

    lockCard = lv_obj_create(lockBackdrop);
    lv_obj_set_size(lockCard, 370, 115);
    lv_obj_align(lockCard, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(lockCard, lv_color_make(18, 24, 40), 0);
    lv_obj_set_style_border_color(lockCard, badgeColor, 0);
    lv_obj_set_style_border_width(lockCard, 2, 0);
    lv_obj_set_style_radius(lockCard, 10, 0);
    lv_obj_set_style_pad_all(lockCard, 12, 0);
    lv_obj_clear_flag(lockCard, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* iconBox = lv_obj_create(lockCard);
    lv_obj_set_size(iconBox, 42, 42);
    lv_obj_align(iconBox, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_set_style_bg_color(iconBox, badgeColor, 0);
    lv_obj_set_style_border_width(iconBox, 0, 0);
    lv_obj_set_style_radius(iconBox, LV_RADIUS_CIRCLE, 0);
    lv_obj_clear_flag(iconBox, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* icon = lv_label_create(iconBox);
    lv_label_set_text(icon, symbol ? symbol : LV_SYMBOL_TRASH);
    CydTheme::applyTextFont(icon, CydTheme::getFont14(), CydTheme::getWhiteColor());
    lv_obj_center(icon);

    lockLblTitle = lv_label_create(lockCard);
    lv_label_set_text(lockLblTitle, title);
    CydTheme::applyTextFont(lockLblTitle, CydTheme::getFont14(), badgeColor);
    lv_obj_align(lockLblTitle, LV_ALIGN_TOP_LEFT, 56, 4);

    lockLblSub = lv_label_create(lockCard);
    lv_label_set_text(lockLblSub, message);
    CydTheme::applyTextFont(lockLblSub, CydTheme::getFont12(), CydTheme::getWhiteColor());
    lv_obj_align(lockLblSub, LV_ALIGN_TOP_LEFT, 56, 26);

    lockBar = lv_bar_create(lockCard);
    lv_obj_set_size(lockBar, 280, 8);
    lv_obj_align(lockBar, LV_ALIGN_TOP_LEFT, 56, 50);
    lv_obj_set_style_bg_color(lockBar, lv_color_make(35, 45, 65), 0);
    lv_obj_set_style_bg_color(lockBar, badgeColor, LV_PART_INDICATOR);
    lv_obj_set_style_radius(lockBar, 4, 0);
    lv_obj_set_style_radius(lockBar, 4, LV_PART_INDICATOR);
    lv_bar_set_range(lockBar, 0, 100);
    lv_bar_set_value(lockBar, 0, LV_ANIM_OFF);

    lockLblHint = lv_label_create(lockCard);
    lv_label_set_text(lockLblHint, hint ? hint : "");
    CydTheme::applyTextFont(lockLblHint, CydTheme::getFont12(), CydTheme::getTextMuted());
    lv_obj_align(lockLblHint, LV_ALIGN_TOP_LEFT, 56, 66);

    // Refresh ngay màn hình không đệ quy timer handler
    lv_refr_now(NULL);
}

void DialogManager::updateLockProgress(int percent, const char* message, const char* hint) {
    if (!lockBackdrop) return;
    if (message && lockLblSub) {
        lv_label_set_text(lockLblSub, message);
    }
    if (hint && lockLblHint) {
        lv_label_set_text(lockLblHint, hint);
    }
    if (lockBar && percent >= 0) {
        if (percent > 100) percent = 100;
        lv_bar_set_value(lockBar, percent, LV_ANIM_OFF);
    }
}

void DialogManager::hideLockOverlay() {
    if (lockBackdrop) {
        lv_obj_del(lockBackdrop);
        lockBackdrop = nullptr;
        lockCard = nullptr;
        lockLblTitle = nullptr;
        lockLblSub = nullptr;
        lockLblHint = nullptr;
        lockBar = nullptr;
    }
}

void DialogManager::dismissModal() {
    if (modalBackdrop) {
        lv_obj_del(modalBackdrop);
        modalBackdrop = nullptr;
    }
}

bool DialogManager::isModalActive() {
    return modalBackdrop != nullptr;
}

bool DialogManager::isLocked() {
    return lockBackdrop != nullptr;
}

void DialogManager::cleanupAll() {
    dismissModal();
    hideLockOverlay();
    if (toastTimer) {
        lv_timer_del(toastTimer);
        toastTimer = nullptr;
    }
    if (toastContainer) {
        lv_obj_del(toastContainer);
        toastContainer = nullptr;
    }
}

void DialogManager::default_dismiss_cb(lv_event_t* e) {
    dismissModal();
}
