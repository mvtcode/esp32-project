#pragma once
#include <Arduino.h>
#include "game_base.h"

enum GameManagerMode {
    GAME_MGR_MENU = 0,
    GAME_MGR_PLAYING = 1
};

/** Initialize game manager and game list */
void game_manager_init();

/**
 * Process inputs and update state
 * @param enc_delta EC11 rotary delta
 * @param btn_confirm Short press on Confirm button (BTN_PLUS / GPIO 14)
 * @param btn_back Short press on Back button (BTN_BACK / GPIO 13)
 * @param btn_confirm_held Confirm button is currently held down
 */
void game_manager_update(int32_t enc_delta, bool btn_confirm, bool btn_back, bool btn_confirm_held = false);

/** Render current game frame or games menu */
void game_manager_render();

/** Check if currently inside games selection menu */
bool game_manager_is_in_menu();
