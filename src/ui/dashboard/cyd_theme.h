#ifndef CYD_THEME_H
#define CYD_THEME_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif
LV_FONT_DECLARE(vi_font_montserrat_12);
LV_FONT_DECLARE(vi_font_montserrat_14);
LV_FONT_DECLARE(vi_font_montserrat_16);
LV_FONT_DECLARE(vi_font_montserrat_20);
LV_FONT_DECLARE(vi_font_montserrat_24);
LV_FONT_DECLARE(vi_weather_font_24);
#ifdef __cplusplus
}
#endif

class CydTheme {
public:
    // Font Getters (Vietnamese-compatible Montserrat fonts)
    static inline const lv_font_t* getFont12() { return &vi_font_montserrat_12; }
    static inline const lv_font_t* getFont14() { return &vi_font_montserrat_14; }
    static inline const lv_font_t* getFont16() { return &vi_font_montserrat_16; }
    static inline const lv_font_t* getFont20() { return &vi_font_montserrat_20; }
    static inline const lv_font_t* getFont24() { return &vi_font_montserrat_24; }
    static inline const lv_font_t* getWeatherFont24() { return &vi_weather_font_24; }
    static inline const lv_font_t* getFont32() { return &lv_font_montserrat_32; }
    static inline const lv_font_t* getFont40() { return &lv_font_montserrat_40; }
    static inline const lv_font_t* getFont48() { return &lv_font_montserrat_48; }
    // Color Tokens
    static inline lv_color_t getBgColor() { return lv_color_make(7, 10, 19); }          // #070A13 (Extremely deep dark navy)
    static inline lv_color_t getCardColor() { return lv_color_make(17, 22, 37); }        // #111625 (Card dark blue)
    static inline lv_color_t getCardBorderColor() { return lv_color_make(30, 37, 56); }  // #1E2538 (Card border grey/blue)
    static inline lv_color_t getAccentColor() { return lv_color_make(94, 53, 177); }     // #5E35B1 (Deep purple primary)
    static inline lv_color_t getAccentGlowColor() { return lv_color_make(155, 93, 229); } // #9B5DE5 (Vibrant glow violet)
    static inline lv_color_t getWhiteColor() { return lv_color_make(255, 255, 255); }
    static inline lv_color_t getTextPrimary() { return lv_color_make(255, 255, 255); }
    static inline lv_color_t getTextSecondary() { return lv_color_make(160, 170, 191); } // #A0AABF (Muted text)
    static inline lv_color_t getTextMuted() { return lv_color_make(78, 85, 102); }       // #4E5566 (Dark text)
    static inline lv_color_t getSuccessColor() { return lv_color_make(76, 175, 80); }    // #4CAF50 (Up trend green)
    static inline lv_color_t getDangerColor() { return lv_color_make(244, 67, 54); }     // #F44336 (Down trend red)
    static inline lv_color_t getGoldColor() { return lv_color_make(255, 183, 3); }       // #FFB703 (Gold warnings)
    static inline lv_color_t getBlueColor() { return lv_color_make(33, 150, 243); }      // #2196F3 (Gas/Fuel blue)

    // Layout Helpers
    static void applyCardStyle(lv_obj_t* obj) {
        lv_obj_set_style_bg_color(obj, getCardColor(), 0);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(obj, getCardBorderColor(), 0);
        lv_obj_set_style_border_width(obj, 1, 0);
        lv_obj_set_style_border_opa(obj, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(obj, 12, 0);
        lv_obj_set_style_pad_all(obj, 10, 0);
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    }

    static void applyTextFont(lv_obj_t* label, const lv_font_t* font, lv_color_t color) {
        lv_obj_set_style_text_font(label, font, 0);
        lv_obj_set_style_text_color(label, color, 0);
    }

    static void applyButtonStyle(lv_obj_t* btn, lv_color_t bgColor, lv_color_t textColor) {
        lv_obj_set_style_bg_color(btn, bgColor, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_style_pad_hor(btn, 12, 0);
        lv_obj_set_style_pad_ver(btn, 6, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_text_color(btn, textColor, 0);
    }

    static void applySliderStyle(lv_obj_t* slider, lv_color_t activeColor) {
        lv_obj_set_style_bg_color(slider, lv_color_make(35, 38, 55), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(slider, lv_color_make(60, 65, 90), LV_PART_MAIN);
        lv_obj_set_style_border_width(slider, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_MAIN);

        lv_obj_set_style_bg_color(slider, activeColor, LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_INDICATOR);
        lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);

        lv_obj_set_style_bg_color(slider, getWhiteColor(), LV_PART_KNOB);
        lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_KNOB);
        lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_KNOB);
        lv_obj_set_style_pad_all(slider, 3, LV_PART_KNOB);
    }
};

#endif // CYD_THEME_H
