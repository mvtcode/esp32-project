#include "dev_hud.h"
#include "cyd_theme.h"
#include <stdio.h>

DevHud::DevHud() : visible(false) {
    // Tạo HUD trên lv_layer_top() để nổi trên tất cả các màn hình
    hudContainer = lv_obj_create(lv_layer_top());
    lv_obj_set_size(hudContainer, 380, 24);
    lv_obj_align(hudContainer, LV_ALIGN_TOP_MID, 0, 2);
    
    lv_obj_set_style_bg_color(hudContainer, lv_color_make(8, 12, 22), 0);
    lv_obj_set_style_bg_opa(hudContainer, LV_OPA_90, 0);
    lv_obj_set_style_border_color(hudContainer, lv_color_make(0, 210, 255), 0);
    lv_obj_set_style_border_width(hudContainer, 1, 0);
    lv_obj_set_style_radius(hudContainer, 12, 0);
    lv_obj_set_style_pad_hor(hudContainer, 10, 0);
    lv_obj_set_style_pad_ver(hudContainer, 2, 0);
    lv_obj_clear_flag(hudContainer, LV_OBJ_FLAG_SCROLLABLE);

    // Flex layout row
    lv_obj_set_layout(hudContainer, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hudContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hudContainer, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // FPS
    lblFps = lv_label_create(hudContainer);
    lv_label_set_text(lblFps, "FPS: 30");
    CydTheme::applyTextFont(lblFps, CydTheme::getFont12(), lv_color_make(0, 255, 170));

    // RAM
    lblRam = lv_label_create(hudContainer);
    lv_label_set_text(lblRam, "RAM: 150KB");
    CydTheme::applyTextFont(lblRam, CydTheme::getFont12(), lv_color_make(0, 210, 255));

    // CPU
    lblCpu = lv_label_create(hudContainer);
    lv_label_set_text(lblCpu, "CPU: 20%");
    CydTheme::applyTextFont(lblCpu, CydTheme::getFont12(), lv_color_make(255, 190, 40));

    // WiFi
    lblWifi = lv_label_create(hudContainer);
    lv_label_set_text(lblWifi, "WiFi: --");
    CydTheme::applyTextFont(lblWifi, CydTheme::getFont12(), lv_color_make(255, 100, 150));

    // Mặc định ẩn
    lv_obj_add_flag(hudContainer, LV_OBJ_FLAG_HIDDEN);
}

DevHud::~DevHud() {
    if (hudContainer) {
        lv_obj_del(hudContainer);
        hudContainer = nullptr;
    }
}

void DevHud::setVisible(bool isVis) {
    visible = isVis;
    if (hudContainer) {
        if (visible) {
            lv_obj_clear_flag(hudContainer, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(hudContainer, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void DevHud::updateStats(float fps, uint32_t freeHeapKb, uint8_t cpuPercent, int32_t rssi, const char* ip) {
    if (!visible || !hudContainer) return;

    char buf[48];

    // FPS
    snprintf(buf, sizeof(buf), "FPS: %.0f", fps);
    lv_label_set_text(lblFps, buf);

    // RAM
    snprintf(buf, sizeof(buf), "RAM: %uK", freeHeapKb);
    lv_label_set_text(lblRam, buf);

    // CPU
    snprintf(buf, sizeof(buf), "CPU: %u%%", cpuPercent);
    lv_label_set_text(lblCpu, buf);

    // WiFi
    if (rssi > -95) {
        snprintf(buf, sizeof(buf), "WF: %ddBm", rssi);
    } else {
        snprintf(buf, sizeof(buf), "WF: Off");
    }
    lv_label_set_text(lblWifi, buf);
}
