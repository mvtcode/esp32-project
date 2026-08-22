#include "game_tetris.h"

static const int BOARD_X = 14;
static const int BOARD_Y = 5;

static const int8_t PIECES[7][4][4][2] = {
    // 0: I piece
    {
        {{0,1}, {1,1}, {2,1}, {3,1}},
        {{2,0}, {2,1}, {2,2}, {2,3}},
        {{0,2}, {1,2}, {2,2}, {3,2}},
        {{1,0}, {1,1}, {1,2}, {1,3}}
    },
    // 1: J piece
    {
        {{0,0}, {0,1}, {1,1}, {2,1}},
        {{1,0}, {2,0}, {1,1}, {1,2}},
        {{0,1}, {1,1}, {2,1}, {2,2}},
        {{1,0}, {1,1}, {0,2}, {1,2}}
    },
    // 2: L piece
    {
        {{2,0}, {0,1}, {1,1}, {2,1}},
        {{1,0}, {1,1}, {1,2}, {2,2}},
        {{0,1}, {1,1}, {2,1}, {0,2}},
        {{0,0}, {1,0}, {1,1}, {1,2}}
    },
    // 3: O piece
    {
        {{0,0}, {1,0}, {0,1}, {1,1}},
        {{0,0}, {1,0}, {0,1}, {1,1}},
        {{0,0}, {1,0}, {0,1}, {1,1}},
        {{0,0}, {1,0}, {0,1}, {1,1}}
    },
    // 4: S piece
    {
        {{1,0}, {2,0}, {0,1}, {1,1}},
        {{1,0}, {1,1}, {2,1}, {2,2}},
        {{1,1}, {2,1}, {0,2}, {1,2}},
        {{0,0}, {0,1}, {1,1}, {1,2}}
    },
    // 5: T piece
    {
        {{1,0}, {0,1}, {1,1}, {2,1}},
        {{1,0}, {1,1}, {2,1}, {1,2}},
        {{0,1}, {1,1}, {2,1}, {1,2}},
        {{1,0}, {0,1}, {1,1}, {1,2}}
    },
    // 6: Z piece
    {
        {{0,0}, {1,0}, {1,1}, {2,1}},
        {{2,0}, {1,1}, {2,1}, {1,2}},
        {{0,1}, {1,1}, {1,2}, {2,2}},
        {{1,0}, {0,1}, {1,1}, {0,2}}
    }
};

GameTetris::GameTetris() {
    m_high_score = 0;
    init();
}

void GameTetris::init() {
    for (int r = 0; r < TETRIS_ROWS; r++) {
        for (int c = 0; c < TETRIS_COLS; c++) {
            m_board[r][c] = 0;
        }
    }

    m_score = 0;
    m_lines = 0;
    m_level = 1;
    m_game_over = false;
    m_exit_requested = false;
    m_enc_accum = 0;
    m_fall_interval_ms = 450;
    m_last_fall_ts = millis();

    m_next_type = random(0, 7);
    spawnPiece();
}

void GameTetris::spawnPiece() {
    m_cur_type = m_next_type;
    m_next_type = random(0, 7);
    m_cur_rot = 0;
    m_cur_x = 3;
    m_cur_y = 0;

    if (checkCollision(m_cur_type, m_cur_rot, m_cur_x, m_cur_y)) {
        m_game_over = true;
    }
}

bool GameTetris::checkCollision(int type, int rot, int x, int y) const {
    for (int i = 0; i < 4; i++) {
        int bx = x + PIECES[type][rot][i][0];
        int by = y + PIECES[type][rot][i][1];

        if (bx < 0 || bx >= TETRIS_COLS || by >= TETRIS_ROWS) {
            return true;
        }
        if (by >= 0 && m_board[by][bx]) {
            return true;
        }
    }
    return false;
}

void GameTetris::lockPiece() {
    for (int i = 0; i < 4; i++) {
        int bx = m_cur_x + PIECES[m_cur_type][m_cur_rot][i][0];
        int by = m_cur_y + PIECES[m_cur_type][m_cur_rot][i][1];
        if (by >= 0 && by < TETRIS_ROWS && bx >= 0 && bx < TETRIS_COLS) {
            m_board[by][bx] = 1;
        }
    }
    clearLines();
    spawnPiece();
}

void GameTetris::clearLines() {
    int cleared = 0;
    for (int r = TETRIS_ROWS - 1; r >= 0; r--) {
        bool full = true;
        for (int c = 0; c < TETRIS_COLS; c++) {
            if (!m_board[r][c]) {
                full = false;
                break;
            }
        }

        if (full) {
            cleared++;
            // Shift all rows above down by 1
            for (int nr = r; nr > 0; nr--) {
                for (int c = 0; c < TETRIS_COLS; c++) {
                    m_board[nr][c] = m_board[nr - 1][c];
                }
            }
            for (int c = 0; c < TETRIS_COLS; c++) {
                m_board[0][c] = 0;
            }
            r++; // re-check current row
        }
    }

    if (cleared > 0) {
        static const int SCORE_TABLE[5] = {0, 100, 300, 500, 800};
        m_score += SCORE_TABLE[cleared] * m_level;
        if (m_score > m_high_score) m_high_score = m_score;
        m_lines += cleared;
        m_level = 1 + (m_lines / 10);
        m_fall_interval_ms = (m_level < 10) ? (450 - (m_level - 1) * 35) : 120;
    }
}

void GameTetris::update(int32_t enc_delta, bool btn_confirm, bool btn_back, bool btn_confirm_held) {
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

    // 1. Shift left/right with EC11 rotary knob (1/2 sensitivity = 2 pulses per column)
    if (enc_delta != 0) {
        m_enc_accum += enc_delta;
        int step = m_enc_accum / 2;
        if (step != 0) {
            m_enc_accum %= 2;
            int new_x = m_cur_x + step;
            if (!checkCollision(m_cur_type, m_cur_rot, new_x, m_cur_y)) {
                m_cur_x = new_x;
            }
        }
    }

    // 2. Rotate piece with Confirm button (Dedicated 100% to Rotate only)
    if (btn_confirm) {
        int next_rot = (m_cur_rot + 1) % 4;
        if (!checkCollision(m_cur_type, next_rot, m_cur_x, m_cur_y)) {
            m_cur_rot = next_rot;
        } else if (!checkCollision(m_cur_type, next_rot, m_cur_x - 1, m_cur_y)) {
            // Wall kick left
            m_cur_x -= 1;
            m_cur_rot = next_rot;
        } else if (!checkCollision(m_cur_type, next_rot, m_cur_x + 1, m_cur_y)) {
            // Wall kick right
            m_cur_x += 1;
            m_cur_rot = next_rot;
        }
    }

    // 3. Gravity fall step (Steady gravity fall based on current Level)
    uint32_t now = millis();
    if (now - m_last_fall_ts >= m_fall_interval_ms) {
        m_last_fall_ts = now;

        if (!checkCollision(m_cur_type, m_cur_rot, m_cur_x, m_cur_y + 1)) {
            m_cur_y += 1;
        } else {
            lockPiece();
        }
    }
}

void GameTetris::render() {
    SafeDraw::clearBuffer();

    // 1. Board Frame
    int fw = TETRIS_COLS * TETRIS_CELL + 2;
    int fh = TETRIS_ROWS * TETRIS_CELL + 2;
    SafeDraw::drawFrame(BOARD_X - 1, BOARD_Y - 1, fw, fh);

    // 2. Locked Blocks on Board
    for (int r = 0; r < TETRIS_ROWS; r++) {
        for (int c = 0; c < TETRIS_COLS; c++) {
            if (m_board[r][c]) {
                int bx = BOARD_X + c * TETRIS_CELL;
                int by = BOARD_Y + r * TETRIS_CELL;
                SafeDraw::drawBox(bx, by, TETRIS_CELL - 1, TETRIS_CELL - 1);
            }
        }
    }

    // 3. Active Falling Piece
    if (!m_game_over) {
        for (int i = 0; i < 4; i++) {
            int bx = BOARD_X + (m_cur_x + PIECES[m_cur_type][m_cur_rot][i][0]) * TETRIS_CELL;
            int by = BOARD_Y + (m_cur_y + PIECES[m_cur_type][m_cur_rot][i][1]) * TETRIS_CELL;
            if (by >= BOARD_Y) {
                SafeDraw::drawBox(bx, by, TETRIS_CELL - 1, TETRIS_CELL - 1);
            }
        }
    }

    // 4. Side Info Panel (Right side)
    int px = 54;
    SafeDraw::setFont(u8g2_font_5x8_tr);
    SafeDraw::drawStr(px, 12, "TETRIS");

    SafeDraw::setFont(u8g2_font_4x6_tr);
    char sc_str[24], ln_str[24], lv_str[24];
    snprintf(sc_str, sizeof(sc_str), "SCORE: %d", m_score);
    snprintf(ln_str, sizeof(ln_str), "LINES: %d", m_lines);
    snprintf(lv_str, sizeof(lv_str), "LEVEL: %d", m_level);
    SafeDraw::drawStr(px, 22, sc_str);
    SafeDraw::drawStr(px, 30, ln_str);
    SafeDraw::drawStr(px, 38, lv_str);

    // 5. Next Piece Preview Box
    SafeDraw::drawStr(px, 48, "NEXT:");
    SafeDraw::drawFrame(px + 24, 42, 18, 16);
    for (int i = 0; i < 4; i++) {
        int nx = px + 27 + PIECES[m_next_type][0][i][0] * 3;
        int ny = 45 + PIECES[m_next_type][0][i][1] * 3;
        SafeDraw::drawBox(nx, ny, 2, 2);
    }

    // 6. Game Over Dialog
    if (m_game_over) {
        int bw = 104;
        int bh = 36;
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
        SafeDraw::drawStr(bx + (bw - SafeDraw::getStrWidth(sub2)) / 2, by + 31, sub2);
    }

    SafeDraw::sendBuffer();
}
