#include "game_manager.h"
#include "game_pong.h"
#include "game_highway.h"
#include "game_brick_breaker.h"
#include "game_snake.h"
#include "game_invaders.h"
#include "game_flappy.h"
#include "game_tetris.h"
#include "game_sky_fighter.h"
#include "../effects/safe_draw.h"
#include "../log.h"

static GameTetris       s_game_tetris;
static GameSkyFighter   s_game_sky_fighter;
static GameHighway      s_game_highway;
static GamePong         s_game_pong;
static GameBrickBreaker s_game_brick;
static GameSnake        s_game_snake;
static GameInvaders     s_game_invaders;
static GameFlappy       s_game_flappy;

static GameBase* const s_games[] = {
    &s_game_tetris,
    &s_game_sky_fighter,
    &s_game_highway,
    &s_game_pong,
    &s_game_brick,
    &s_game_snake,
    &s_game_invaders,
    &s_game_flappy
};

static const int TOTAL_GAMES = sizeof(s_games) / sizeof(s_games[0]);

static GameManagerMode s_mgr_mode = GAME_MGR_MENU;
static int s_selected_idx = 0;
static int s_active_game_idx = 0;
static int s_scroll_accum = 0;

void game_manager_init() {
    s_mgr_mode = GAME_MGR_MENU;
    s_selected_idx = 0;
    s_active_game_idx = 0;
    s_scroll_accum = 0;
    LOG_I("GameMgr", "Initialized with %d retro games", TOTAL_GAMES);
}

bool game_manager_is_in_menu() {
    return (s_mgr_mode == GAME_MGR_MENU);
}

static void draw_games_menu_screen() {
    // 1. Header Banner (matching MP3 playlist style)
    SafeDraw::drawBox(0, 0, SCREEN_W, 11);
    SafeDraw::setDrawColor(0);
    SafeDraw::setFont(u8g2_font_5x8_tr);
    char hdr[32];
    snprintf(hdr, sizeof(hdr), "GAMES MENU [%d/%d]", s_selected_idx + 1, TOTAL_GAMES);
    SafeDraw::drawStr(3, 9, hdr);
    SafeDraw::setDrawColor(1);

    // 2. Viewport calculation (4 visible lines)
    const int VISIBLE_ITEMS = 4;
    const int LINE_H = 12;
    const int START_Y = 22;

    int top_idx = s_selected_idx - (VISIBLE_ITEMS / 2);
    if (top_idx < 0) top_idx = 0;
    if (top_idx > TOTAL_GAMES - VISIBLE_ITEMS) top_idx = TOTAL_GAMES - VISIBLE_ITEMS;
    if (top_idx < 0) top_idx = 0;

    for (int i = 0; i < VISIBLE_ITEMS && (top_idx + i) < TOTAL_GAMES; i++) {
        int item_idx = top_idx + i;
        GameBase *game = s_games[item_idx];
        if (!game) continue;

        int y = START_Y + i * LINE_H;
        bool is_selected = (item_idx == s_selected_idx);

        if (is_selected) {
            SafeDraw::drawRBox(0, y - 9, SCREEN_W - 6, LINE_H - 1, 1);
            SafeDraw::setDrawColor(0);
        }

        char line_str[64];
        const char *icon = is_selected ? "> " : "  ";
        snprintf(line_str, sizeof(line_str), "%s%d. %s", icon, item_idx + 1, game->getName());
        SafeDraw::setFont(u8g2_font_5x8_tr);
        SafeDraw::drawStr(3, y - 1, line_str);

        if (is_selected) {
            SafeDraw::setDrawColor(1);
        }
    }

    // 3. Right vertical scrollbar
    int sb_x = SCREEN_W - 3;
    SafeDraw::drawVLine(sb_x, 13, SCREEN_H - 14);
    if (TOTAL_GAMES > VISIBLE_ITEMS) {
        int avail_h = SCREEN_H - 14;
        int bar_h = (VISIBLE_ITEMS * avail_h) / TOTAL_GAMES;
        if (bar_h < 4) bar_h = 4;
        int bar_y = 13 + (s_selected_idx * (avail_h - bar_h)) / (TOTAL_GAMES - 1);
        SafeDraw::drawBox(sb_x - 1, bar_y, 3, bar_h);
    }
}

void game_manager_update(int32_t enc_delta, bool btn_confirm, bool btn_back, bool btn_confirm_held) {
    if (s_mgr_mode == GAME_MGR_MENU) {
        // Scroll through menu with EC11 knob
        if (enc_delta != 0) {
            s_scroll_accum += enc_delta;
            int step = s_scroll_accum / 4;
            if (step != 0) {
                s_scroll_accum %= 4;
                s_selected_idx += step;
                if (s_selected_idx < 0) s_selected_idx = 0;
                if (s_selected_idx >= TOTAL_GAMES) s_selected_idx = TOTAL_GAMES - 1;
            }
        }

        // Confirm button: Launch selected game
        if (btn_confirm) {
            s_active_game_idx = s_selected_idx;
            s_games[s_active_game_idx]->init();
            s_mgr_mode = GAME_MGR_PLAYING;
            LOG_I("GameMgr", "Launched game: %s", s_games[s_active_game_idx]->getName());
        }
    } else {
        // Playing active game
        GameBase *active_game = s_games[s_active_game_idx];
        if (active_game) {
            active_game->update(enc_delta, btn_confirm, btn_back, btn_confirm_held);

            // Exit back to menu if requested
            if (active_game->isExitRequested()) {
                s_mgr_mode = GAME_MGR_MENU;
                LOG_I("GameMgr", "Exited to games menu");
            }
        }
    }
}

void game_manager_render() {
    if (s_mgr_mode == GAME_MGR_MENU) {
        SafeDraw::clearBuffer();
        draw_games_menu_screen();
        SafeDraw::sendBuffer();
    } else {
        GameBase *active_game = s_games[s_active_game_idx];
        if (active_game) {
            active_game->render();
        }
    }
}
