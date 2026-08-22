#include "game_brick_breaker.h"

static const int PADDLE_Y = 58;
static const int BRICK_W = 14;
static const int BRICK_H = 4;
static const int BRICK_START_X = 8;
static const int BRICK_START_Y = 14;

GameBrickBreaker::GameBrickBreaker() {
    m_high_score = 0;
    init();
}

void GameBrickBreaker::init() {
    m_paddle_w = 20;
    m_paddle_x = (SCREEN_W - m_paddle_w) / 2.0f;
    m_lives = 3;
    m_score = 0;
    m_game_over = false;
    m_game_won = false;
    m_exit_requested = false;
    resetBricks();
    resetBall();
}

void GameBrickBreaker::resetBricks() {
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            m_bricks[r][c] = 1; // active
        }
    }
}

void GameBrickBreaker::resetBall() {
    m_ball_attached = true;
    m_ball_x = m_paddle_x + m_paddle_w / 2.0f;
    m_ball_y = PADDLE_Y - 3;
    m_ball_vx = 1.2f;
    m_ball_vy = -1.4f;
}

void GameBrickBreaker::update(int32_t enc_delta, bool btn_confirm, bool btn_back, bool /*btn_confirm_held*/) {
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

    // 1. Paddle movement via EC11
    if (enc_delta != 0) {
        m_paddle_x += (float)enc_delta * 2.2f;
        if (m_paddle_x < 2.0f) m_paddle_x = 2.0f;
        if (m_paddle_x > SCREEN_W - m_paddle_w - 2.0f) m_paddle_x = SCREEN_W - m_paddle_w - 2.0f;
    }

    // 2. Launch ball
    if (m_ball_attached) {
        m_ball_x = m_paddle_x + m_paddle_w / 2.0f;
        m_ball_y = PADDLE_Y - 3;
        if (btn_confirm) {
            m_ball_attached = false;
            m_ball_vx = ((float)random(-80, 80) / 100.0f) * 1.5f;
            if (fabs(m_ball_vx) < 0.4f) m_ball_vx = (m_ball_vx < 0) ? -0.8f : 0.8f;
            m_ball_vy = -1.5f;
        }
        return;
    }

    // 3. Ball movement & Wall bounce
    m_ball_x += m_ball_vx;
    m_ball_y += m_ball_vy;

    if (m_ball_x <= 2.0f) {
        m_ball_x = 2.0f;
        m_ball_vx = -m_ball_vx;
    } else if (m_ball_x >= SCREEN_W - 3.0f) {
        m_ball_x = SCREEN_W - 3.0f;
        m_ball_vx = -m_ball_vx;
    }

    if (m_ball_y <= 10.0f) {
        m_ball_y = 10.0f;
        m_ball_vy = -m_ball_vy;
    }

    // 4. Collision with Paddle
    if (m_ball_y >= PADDLE_Y - 2 && m_ball_y <= PADDLE_Y + 2) {
        if (m_ball_x >= m_paddle_x - 1.0f && m_ball_x <= m_paddle_x + m_paddle_w + 1.0f) {
            m_ball_y = PADDLE_Y - 2;
            float hit_offset = (m_ball_x - (m_paddle_x + m_paddle_w / 2.0f)) / (m_paddle_w / 2.0f);
            m_ball_vy = -fabs(m_ball_vy);
            m_ball_vx = hit_offset * 2.0f;
        }
    }

    // 5. Collision with Bricks
    bool all_cleared = true;
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            if (m_bricks[r][c] == 0) continue;
            all_cleared = false;

            int bx = BRICK_START_X + c * (BRICK_W + 1);
            int by = BRICK_START_Y + r * (BRICK_H + 1);

            if (m_ball_x >= bx - 1 && m_ball_x <= bx + BRICK_W + 1 &&
                m_ball_y >= by - 1 && m_ball_y <= by + BRICK_H + 1) {
                m_bricks[r][c] = 0; // destroy brick
                m_score += 10 * (BRICK_ROWS - r);
                if (m_score > m_high_score) m_high_score = m_score;
                m_ball_vy = -m_ball_vy;
                break;
            }
        }
    }

    if (all_cleared) {
        m_game_won = true;
    }

    // 6. Ball falls below bottom
    if (m_ball_y > SCREEN_H) {
        m_lives--;
        if (m_lives <= 0) {
            m_game_over = true;
        } else {
            resetBall();
        }
    }
}

void GameBrickBreaker::render() {
    SafeDraw::clearBuffer();

    // 1. Top Header: Score & Lives
    SafeDraw::setFont(u8g2_font_4x6_tr);
    char sc_str[32], lv_str[16];
    snprintf(sc_str, sizeof(sc_str), "SCORE: %d", m_score);
    snprintf(lv_str, sizeof(lv_str), "HEARTS: %d", m_lives);
    SafeDraw::drawStr(2, 7, sc_str);
    SafeDraw::drawStr(SCREEN_W - SafeDraw::getStrWidth(lv_str) - 2, 7, lv_str);
    SafeDraw::drawHLine(0, 9, SCREEN_W);

    // 2. Bricks
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            if (m_bricks[r][c] == 0) continue;
            int bx = BRICK_START_X + c * (BRICK_W + 1);
            int by = BRICK_START_Y + r * (BRICK_H + 1);
            SafeDraw::drawBox(bx, by, BRICK_W, BRICK_H);
        }
    }

    // 3. Paddle
    SafeDraw::drawRBox((int)m_paddle_x, PADDLE_Y, m_paddle_w, 3, 1);

    // 4. Ball
    SafeDraw::drawBox((int)m_ball_x - 1, (int)m_ball_y - 1, 2, 2);

    // 5. Attached Ball Launch Prompt
    if (m_ball_attached && !m_game_over && !m_game_won) {
        SafeDraw::setFont(u8g2_font_4x6_tr);
        const char *hint = "CONFIRM TO LAUNCH BALL";
        SafeDraw::drawStr((SCREEN_W - SafeDraw::getStrWidth(hint)) / 2, PADDLE_Y - 6, hint);
    }

    // 6. Game Over / Won Popup
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
        const char *t = m_game_won ? "VICTORY! ALL CLEAR" : "GAME OVER";
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
