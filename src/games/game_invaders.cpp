#include "game_invaders.h"

static const int SHIP_Y = 56;
static const int ALIEN_W = 10;
static const int ALIEN_H = 6;
static const int ALIEN_GAP_X = 6;
static const int ALIEN_GAP_Y = 4;

GameInvaders::GameInvaders() {
    m_high_score = 0;
    init();
}

void GameInvaders::init() {
    m_ship_x = SCREEN_W / 2.0f;
    m_lives = 3;
    m_score = 0;
    m_game_over = false;
    m_game_won = false;
    m_exit_requested = false;

    for (int i = 0; i < MAX_BULLETS; i++) m_bullets[i].active = false;
    for (int i = 0; i < MAX_BOMBS; i++) m_bombs[i].active = false;

    resetFleet();
}

void GameInvaders::resetFleet() {
    m_fleet_x = 10.0f;
    m_fleet_y = 14.0f;
    m_fleet_vx = 3.0f;
    m_fleet_step_ms = 400;
    m_last_fleet_move_ts = millis();

    for (int r = 0; r < ALIEN_ROWS; r++) {
        for (int c = 0; c < ALIEN_COLS; c++) {
            m_aliens[r][c] = 1;
        }
    }
}

void GameInvaders::shootBullet() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!m_bullets[i].active) {
            m_bullets[i].active = true;
            m_bullets[i].x = m_ship_x;
            m_bullets[i].y = SHIP_Y - 3;
            break;
        }
    }
}

void GameInvaders::spawnBomb() {
    // Find a bottom-most living alien to drop bomb
    int active_cols[ALIEN_COLS];
    int count = 0;
    for (int c = 0; c < ALIEN_COLS; c++) {
        for (int r = ALIEN_ROWS - 1; r >= 0; r--) {
            if (m_aliens[r][c]) {
                active_cols[count++] = c;
                break;
            }
        }
    }
    if (count == 0) return;

    int col = active_cols[random(0, count)];
    int row = 0;
    for (int r = ALIEN_ROWS - 1; r >= 0; r--) {
        if (m_aliens[r][col]) {
            row = r;
            break;
        }
    }

    for (int i = 0; i < MAX_BOMBS; i++) {
        if (!m_bombs[i].active) {
            m_bombs[i].active = true;
            m_bombs[i].x = m_fleet_x + col * (ALIEN_W + ALIEN_GAP_X) + ALIEN_W / 2;
            m_bombs[i].y = m_fleet_y + row * (ALIEN_H + ALIEN_GAP_Y) + ALIEN_H;
            break;
        }
    }
}

void GameInvaders::update(int32_t enc_delta, bool btn_confirm, bool btn_back, bool /*btn_confirm_held*/) {
    if (btn_back) {
        m_exit_requested = true;
        return;
    }

    if (m_game_over || m_game_won) {
        if (btn_confirm) {
            init();
        }
        return;
    }

    // 1. Move player ship via EC11
    if (enc_delta != 0) {
        m_ship_x += (float)enc_delta * 2.2f;
        if (m_ship_x < 8.0f) m_ship_x = 8.0f;
        if (m_ship_x > SCREEN_W - 8.0f) m_ship_x = SCREEN_W - 8.0f;
    }

    // 2. Shoot laser
    if (btn_confirm) {
        shootBullet();
    }

    // 3. Move bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!m_bullets[i].active) continue;
        m_bullets[i].y -= 2.5f;
        if (m_bullets[i].y < 10.0f) {
            m_bullets[i].active = false;
            continue;
        }

        // Bullet hit alien check
        for (int r = 0; r < ALIEN_ROWS; r++) {
            for (int c = 0; c < ALIEN_COLS; c++) {
                if (!m_aliens[r][c]) continue;
                float ax = m_fleet_x + c * (ALIEN_W + ALIEN_GAP_X);
                float ay = m_fleet_y + r * (ALIEN_H + ALIEN_GAP_Y);

                if (m_bullets[i].x >= ax && m_bullets[i].x <= ax + ALIEN_W &&
                    m_bullets[i].y >= ay && m_bullets[i].y <= ay + ALIEN_H) {
                    m_aliens[r][c] = 0; // destroyed!
                    m_bullets[i].active = false;
                    m_score += (ALIEN_ROWS - r) * 20;
                    if (m_score > m_high_score) m_high_score = m_score;
                    if (m_fleet_step_ms > 120) m_fleet_step_ms -= 10;
                    break;
                }
            }
            if (!m_bullets[i].active) break;
        }
    }

    // 4. Move bombs
    for (int i = 0; i < MAX_BOMBS; i++) {
        if (!m_bombs[i].active) continue;
        m_bombs[i].y += 1.4f;
        if (m_bombs[i].y > SCREEN_H) {
            m_bombs[i].active = false;
            continue;
        }

        // Bomb hit player check
        if (m_bombs[i].y >= SHIP_Y - 2 && m_bombs[i].y <= SHIP_Y + 4 &&
            fabs(m_bombs[i].x - m_ship_x) < 7.0f) {
            m_bombs[i].active = false;
            m_lives--;
            if (m_lives <= 0) {
                m_game_over = true;
            }
        }
    }

    // Random bomb drop
    if (random(0, 100) < 4) {
        spawnBomb();
    }

    // 5. Move Alien fleet
    uint32_t now = millis();
    if (now - m_last_fleet_move_ts >= m_fleet_step_ms) {
        m_last_fleet_move_ts = now;

        float min_x = 999.0f, max_x = -999.0f;
        float max_y = 0.0f;
        bool any_alive = false;

        for (int r = 0; r < ALIEN_ROWS; r++) {
            for (int c = 0; c < ALIEN_COLS; c++) {
                if (!m_aliens[r][c]) continue;
                any_alive = true;
                float ax = m_fleet_x + c * (ALIEN_W + ALIEN_GAP_X);
                float ay = m_fleet_y + r * (ALIEN_H + ALIEN_GAP_Y);
                if (ax < min_x) min_x = ax;
                if (ax + ALIEN_W > max_x) max_x = ax + ALIEN_W;
                if (ay + ALIEN_H > max_y) max_y = ay + ALIEN_H;
            }
        }

        if (!any_alive) {
            m_game_won = true;
            return;
        }

        if (max_y >= SHIP_Y - 2) {
            m_game_over = true;
            return;
        }

        if ((m_fleet_vx > 0 && max_x >= SCREEN_W - 4) || (m_fleet_vx < 0 && min_x <= 4)) {
            m_fleet_vx = -m_fleet_vx;
            m_fleet_y += 3.0f;
        } else {
            m_fleet_x += m_fleet_vx;
        }
    }
}

void GameInvaders::render() {
    SafeDraw::clearBuffer();

    // 1. Header
    SafeDraw::setFont(u8g2_font_4x6_tr);
    char sc_str[32], lv_str[16];
    snprintf(sc_str, sizeof(sc_str), "SCORE: %d", m_score);
    snprintf(lv_str, sizeof(lv_str), "SHIPS: %d", m_lives);
    SafeDraw::drawStr(2, 7, sc_str);
    SafeDraw::drawStr(SCREEN_W - SafeDraw::getStrWidth(lv_str) - 2, 7, lv_str);
    SafeDraw::drawHLine(0, 9, SCREEN_W);

    // 2. Aliens
    for (int r = 0; r < ALIEN_ROWS; r++) {
        for (int c = 0; c < ALIEN_COLS; c++) {
            if (!m_aliens[r][c]) continue;
            int ax = (int)(m_fleet_x + c * (ALIEN_W + ALIEN_GAP_X));
            int ay = (int)(m_fleet_y + r * (ALIEN_H + ALIEN_GAP_Y));

            // Retro alien sprite shape
            SafeDraw::drawBox(ax + 2, ay, 6, 2);
            SafeDraw::drawBox(ax, ay + 2, 10, 3);
            SafeDraw::drawPixel(ax + 1, ay + 5);
            SafeDraw::drawPixel(ax + 8, ay + 5);
            // Eye cutouts
            SafeDraw::setDrawColor(0);
            SafeDraw::drawPixel(ax + 2, ay + 3);
            SafeDraw::drawPixel(ax + 7, ay + 3);
            SafeDraw::setDrawColor(1);
        }
    }

    // 3. Player Ship
    int sx = (int)m_ship_x;
    SafeDraw::drawTriangle(sx, SHIP_Y - 4, sx - 6, SHIP_Y + 3, sx + 6, SHIP_Y + 3);
    SafeDraw::drawBox(sx - 2, SHIP_Y + 1, 5, 3);

    // 4. Bullets & Bombs
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (m_bullets[i].active) {
            SafeDraw::drawVLine((int)m_bullets[i].x, (int)m_bullets[i].y, 3);
        }
    }
    for (int i = 0; i < MAX_BOMBS; i++) {
        if (m_bombs[i].active) {
            SafeDraw::drawBox((int)m_bombs[i].x - 1, (int)m_bombs[i].y, 2, 2);
        }
    }

    // 5. Game Over / Victory
    if (m_game_over || m_game_won) {
        int bw = 104;
        int bh = 34;
        int bx = (SCREEN_W - bw) / 2;
        int by = (SCREEN_H - bh) / 2;

        SafeDraw::setDrawColor(0);
        SafeDraw::drawBox(bx, by, bw, bh);
        SafeDraw::setDrawColor(1);
        SafeDraw::drawRFrame(bx, by, bw, bh, 2);

        SafeDraw::setFont(u8g2_font_6x10_tf);
        const char *t = m_game_won ? "GALAXY SAVED!" : "GAME OVER";
        SafeDraw::drawStr(bx + (bw - SafeDraw::getStrWidth(t)) / 2, by + 12, t);

        SafeDraw::setFont(u8g2_font_4x6_tr);
        char s_sub[32];
        snprintf(s_sub, sizeof(s_sub), "SCORE: %d | BEST: %d", m_score, m_high_score);
        SafeDraw::drawStr(bx + (bw - SafeDraw::getStrWidth(s_sub)) / 2, by + 22, s_sub);

        const char *sub2 = "CONFIRM: REPLAY | BACK: MENU";
        SafeDraw::drawStr(bx + (bw - SafeDraw::getStrWidth(sub2)) / 2, by + 30, sub2);
    }

    SafeDraw::sendBuffer();
}
