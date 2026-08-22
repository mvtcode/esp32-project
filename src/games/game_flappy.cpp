#include "game_flappy.h"

static const int BIRD_X = 24;
static const int PIPE_W = 10;
static const int PIPE_DIST = 70;
static const int GROUND_Y = 60;

GameFlappy::GameFlappy() {
    m_high_score = 0;
    init();
}

void GameFlappy::init() {
    m_bird_y = 30.0f;
    m_bird_vy = 0.0f;
    m_score = 0;
    m_game_over = false;
    m_started = false;
    m_exit_requested = false;
    resetPipes();
}

void GameFlappy::resetPipes() {
    for (int i = 0; i < 2; i++) {
        m_pipes[i].x = SCREEN_W + 10 + i * PIPE_DIST;
        m_pipes[i].gap_h = 24;
        m_pipes[i].gap_y = random(14, GROUND_Y - m_pipes[i].gap_h - 4);
        m_pipes[i].scored = false;
    }
}

void GameFlappy::update(int32_t enc_delta, bool btn_confirm, bool btn_back, bool /*btn_confirm_held*/) {
    if (btn_back) {
        m_exit_requested = true;
        return;
    }

    if (m_game_over) {
        if (btn_confirm) {
            init();
        }
        return;
    }

    // 1. Jump with Confirm or Encoder turn
    if (btn_confirm || enc_delta != 0) {
        m_started = true;
        m_bird_vy = -2.2f;
    }

    if (!m_started) return;

    // 2. Bird Gravity Physics
    m_bird_vy += 0.16f;
    if (m_bird_vy > 3.0f) m_bird_vy = 3.0f;
    m_bird_y += m_bird_vy;

    // Ground / Ceiling collision
    if (m_bird_y >= GROUND_Y - 4) {
        m_bird_y = GROUND_Y - 4;
        m_game_over = true;
    }
    if (m_bird_y <= 2) {
        m_bird_y = 2;
        m_bird_vy = 0.0f;
    }

    // 3. Move Pipes
    for (int i = 0; i < 2; i++) {
        m_pipes[i].x -= 1.2f;

        // Score increment when bird passes pipe center
        if (!m_pipes[i].scored && m_pipes[i].x + PIPE_W < BIRD_X) {
            m_pipes[i].scored = true;
            m_score++;
            if (m_score > m_high_score) m_high_score = m_score;
        }

        // Pipe Recycle
        if (m_pipes[i].x < -PIPE_W) {
            int other = (i == 0) ? 1 : 0;
            m_pipes[i].x = m_pipes[other].x + PIPE_DIST;
            m_pipes[i].gap_h = 22;
            m_pipes[i].gap_y = random(12, GROUND_Y - m_pipes[i].gap_h - 4);
            m_pipes[i].scored = false;
        }

        // 4. Pipe Collision
        if (BIRD_X + 4 >= m_pipes[i].x && BIRD_X - 4 <= m_pipes[i].x + PIPE_W) {
            if (m_bird_y - 3 < m_pipes[i].gap_y || m_bird_y + 3 > m_pipes[i].gap_y + m_pipes[i].gap_h) {
                m_game_over = true;
            }
        }
    }
}

void GameFlappy::render() {
    SafeDraw::clearBuffer();

    // 1. Pipes (Top & Bottom)
    for (int i = 0; i < 2; i++) {
        int px = (int)m_pipes[i].x;
        int gy = m_pipes[i].gap_y;
        int gh = m_pipes[i].gap_h;

        // Top Pipe
        if (gy > 0) {
            SafeDraw::drawBox(px, 0, PIPE_W, gy);
            SafeDraw::drawBox(px - 1, gy - 3, PIPE_W + 2, 3); // collar
        }
        // Bottom Pipe
        if (gy + gh < GROUND_Y) {
            SafeDraw::drawBox(px, gy + gh, PIPE_W, GROUND_Y - (gy + gh));
            SafeDraw::drawBox(px - 1, gy + gh, PIPE_W + 2, 3); // collar
        }
    }

    // 2. Ground line
    SafeDraw::drawHLine(0, GROUND_Y, SCREEN_W);
    for (int x = 0; x < SCREEN_W; x += 8) {
        SafeDraw::drawLine(x, GROUND_Y + 1, x + 4, 63);
    }

    // 3. Bird (Round body + eye + beak)
    int bx = BIRD_X;
    int by = (int)m_bird_y;
    SafeDraw::drawDisc(bx, by, 4);
    SafeDraw::drawTriangle(bx + 3, by - 1, bx + 7, by, bx + 3, by + 2); // beak
    // Wing
    SafeDraw::setDrawColor(0);
    SafeDraw::drawPixel(bx + 1, by - 1); // eye
    SafeDraw::drawHLine(bx - 3, by + (m_bird_vy < 0 ? 1 : -1), 4); // wing flap
    SafeDraw::setDrawColor(1);

    // 4. Score at top center
    SafeDraw::setFont(u8g2_font_6x10_tf);
    char sc_str[16];
    snprintf(sc_str, sizeof(sc_str), "%d", m_score);
    SafeDraw::drawStr((SCREEN_W - SafeDraw::getStrWidth(sc_str)) / 2, 10, sc_str);

    // 5. Start Prompt
    if (!m_started && !m_game_over) {
        SafeDraw::setFont(u8g2_font_4x6_tr);
        const char *hint = "PRESS CONFIRM OR FLAP";
        SafeDraw::drawStr((SCREEN_W - SafeDraw::getStrWidth(hint)) / 2, 45, hint);
    }

    // 6. Game Over Screen
    if (m_game_over) {
        int bw = 104;
        int bh = 34;
        int bx = (SCREEN_W - bw) / 2;
        int by = (SCREEN_H - bh) / 2;

        SafeDraw::setDrawColor(0);
        SafeDraw::drawBox(bx, by, bw, bh);
        SafeDraw::setDrawColor(1);
        SafeDraw::drawRFrame(bx, by, bw, bh, 2);

        SafeDraw::setFont(u8g2_font_6x10_tf);
        const char *t = "GAME OVER";
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
