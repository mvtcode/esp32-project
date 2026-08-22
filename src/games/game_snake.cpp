#include "game_snake.h"

static const int OFFSET_X = 6;
static const int OFFSET_Y = 12;

// Direction offsets: 0=UP, 1=RIGHT, 2=DOWN, 3=LEFT
static const int8_t DIR_DX[4] = { 0,  1,  0, -1};
static const int8_t DIR_DY[4] = {-1,  0,  1,  0};

GameSnake::GameSnake() {
    m_high_score = 0;
    init();
}

void GameSnake::init() {
    m_length = 4;
    m_dir = 1; // RIGHT
    int start_x = GRID_W / 4;
    int start_y = GRID_H / 2;
    for (int i = 0; i < m_length; i++) {
        m_body[i].x = start_x - i;
        m_body[i].y = start_y;
    }
    m_score = 0;
    m_game_over = false;
    m_exit_requested = false;
    m_step_interval_ms = 110;
    m_enc_accum = 0;
    m_last_step_ts = millis();
    spawnFood();
}

void GameSnake::spawnFood() {
    bool valid = false;
    while (!valid) {
        m_food.x = random(0, GRID_W);
        m_food.y = random(0, GRID_H);
        valid = true;
        for (int i = 0; i < m_length; i++) {
            if (m_body[i].x == m_food.x && m_body[i].y == m_food.y) {
                valid = false;
                break;
            }
        }
    }
}

void GameSnake::update(int32_t enc_delta, bool btn_confirm, bool btn_back, bool btn_confirm_held) {
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

    // 1. Direction turning with EC11 rotary knob (1/2 sensitivity = 2 pulses per 90-degree turn)
    if (enc_delta != 0) {
        m_enc_accum += enc_delta;
        int step = m_enc_accum / 2;
        if (step != 0) {
            m_enc_accum %= 2;
            if (step > 0) {
                // Clockwise turns
                for (int s = 0; s < step; s++) {
                    m_dir = (m_dir + 1) % 4;
                }
            } else {
                // Counter-clockwise turns
                for (int s = 0; s < -step; s++) {
                    m_dir = (m_dir + 3) % 4;
                }
            }
        }
    }

    // 2. Speed boost when holding confirm button
    uint32_t interval = (btn_confirm_held || btn_confirm) ? 45 : m_step_interval_ms;

    // 3. Move snake head on step timer
    uint32_t now = millis();
    if (now - m_last_step_ts < interval) return;
    m_last_step_ts = now;

    SnakePoint next_head;
    next_head.x = m_body[0].x + DIR_DX[m_dir];
    next_head.y = m_body[0].y + DIR_DY[m_dir];

    // Wall collision
    if (next_head.x < 0 || next_head.x >= GRID_W || next_head.y < 0 || next_head.y >= GRID_H) {
        m_game_over = true;
        return;
    }

    // Self collision
    for (int i = 0; i < m_length; i++) {
        if (m_body[i].x == next_head.x && m_body[i].y == next_head.y) {
            m_game_over = true;
            return;
        }
    }

    // Check food
    bool ate_food = (next_head.x == m_food.x && next_head.y == m_food.y);

    // Shift body
    if (ate_food) {
        if (m_length < SNAKE_MAX_LEN) m_length++;
        m_score += 10;
        if (m_score > m_high_score) m_high_score = m_score;
        if (m_step_interval_ms > 50) m_step_interval_ms -= 2;
        spawnFood();
    }

    for (int i = m_length - 1; i > 0; i--) {
        m_body[i] = m_body[i - 1];
    }
    m_body[0] = next_head;
}

void GameSnake::render() {
    SafeDraw::clearBuffer();

    // 1. Header
    SafeDraw::setFont(u8g2_font_4x6_tr);
    char s_str[32];
    snprintf(s_str, sizeof(s_str), "SCORE: %d | BEST: %d", m_score, m_high_score);
    SafeDraw::drawStr(2, 7, s_str);
    SafeDraw::drawHLine(0, 9, SCREEN_W);

    // 2. Playfield boundary frame
    SafeDraw::drawFrame(OFFSET_X - 2, OFFSET_Y - 2, GRID_W * CELL_SZ + 4, GRID_H * CELL_SZ + 4);

    // 3. Food (pulsing diamond / box)
    int fx = OFFSET_X + m_food.x * CELL_SZ;
    int fy = OFFSET_Y + m_food.y * CELL_SZ;
    SafeDraw::drawFrame(fx, fy, CELL_SZ, CELL_SZ);
    SafeDraw::drawPixel(fx + 1, fy + 1);

    // 4. Snake Body
    for (int i = 0; i < m_length; i++) {
        int sx = OFFSET_X + m_body[i].x * CELL_SZ;
        int sy = OFFSET_Y + m_body[i].y * CELL_SZ;
        if (i == 0) {
            // Head (solid)
            SafeDraw::drawBox(sx, sy, CELL_SZ, CELL_SZ);
        } else {
            // Body segment
            SafeDraw::drawRBox(sx, sy, CELL_SZ, CELL_SZ, 1);
        }
    }

    // 5. Game Over Dialog
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
        snprintf(s_sub, sizeof(s_sub), "FINAL SCORE: %d", m_score);
        SafeDraw::drawStr(bx + (bw - SafeDraw::getStrWidth(s_sub)) / 2, by + 22, s_sub);

        const char *sub2 = "CONFIRM: REPLAY | BACK: MENU";
        SafeDraw::drawStr(bx + (bw - SafeDraw::getStrWidth(sub2)) / 2, by + 30, sub2);
    }

    SafeDraw::sendBuffer();
}
