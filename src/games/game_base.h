#pragma once
#include <Arduino.h>
#include "../effects/safe_draw.h"

/**
 * @brief Base class for all retro games on OLED 128x64.
 */
class GameBase {
public:
    virtual ~GameBase() {}

    /** @brief Get the display name of the game for the menu */
    virtual const char* getName() const = 0;

    /** @brief Initialize or restart the game state */
    virtual void init() = 0;

    /**
     * @brief Process user input and physics update.
     * @param enc_delta Rotation delta from EC11 rotary encoder.
     * @param btn_confirm Short press on Confirm button (BTN_PLUS / GPIO 14).
     * @param btn_back Short press on Back button (BTN_BACK / GPIO 13).
     * @param btn_confirm_held True if Confirm button is currently held down (for continuous action/turbo).
     */
    virtual void update(int32_t enc_delta, bool btn_confirm, bool btn_back, bool btn_confirm_held = false) = 0;

    /** @brief Render the current game frame buffer (using SafeDraw / u8g2) */
    virtual void render() = 0;

    /** @brief Check if player requested to exit to Game Menu */
    virtual bool isExitRequested() const = 0;
};
