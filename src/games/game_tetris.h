#pragma once
#include "game_base.h"

static const int TETRIS_COLS = 10;
static const int TETRIS_ROWS = 18;
static const int TETRIS_CELL = 3;

class GameTetris : public GameBase {
public:
    GameTetris();
    virtual const char* getName() const override { return "CLASSIC TETRIS"; }
    virtual void init() override;
    virtual void update(int32_t enc_delta, bool btn_confirm, bool btn_back, bool btn_confirm_held = false) override;
    virtual void render() override;
    virtual bool isExitRequested() const override { return m_exit_requested; }

private:
    uint8_t m_board[TETRIS_ROWS][TETRIS_COLS];
    int     m_cur_type;     // 0..6 (I, J, L, O, S, T, Z)
    int     m_cur_rot;      // 0..3
    int     m_cur_x;
    int     m_cur_y;
    int     m_next_type;
    int     m_score;
    int     m_lines;
    int     m_level;
    int     m_high_score;
    bool    m_game_over;
    bool    m_exit_requested;
    int     m_enc_accum;
    uint32_t m_last_fall_ts;
    uint32_t m_fall_interval_ms;

    void spawnPiece();
    bool checkCollision(int type, int rot, int x, int y) const;
    void lockPiece();
    void clearLines();
};
