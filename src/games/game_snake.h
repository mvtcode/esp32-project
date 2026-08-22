#pragma once
#include "game_base.h"

static const int SNAKE_MAX_LEN = 120;
static const int GRID_W = 38;
static const int GRID_H = 16;
static const int CELL_SZ = 3;

struct SnakePoint {
    int8_t x;
    int8_t y;
};

class GameSnake : public GameBase {
public:
    GameSnake();
    virtual const char* getName() const override { return "RETRO SNAKE"; }
    virtual void init() override;
    virtual void update(int32_t enc_delta, bool btn_confirm, bool btn_back, bool btn_confirm_held = false) override;
    virtual void render() override;
    virtual bool isExitRequested() const override { return m_exit_requested; }

private:
    SnakePoint m_body[SNAKE_MAX_LEN];
    int        m_length;
    int        m_dir; // 0=UP, 1=RIGHT, 2=DOWN, 3=LEFT
    SnakePoint m_food;
    int        m_score;
    int        m_high_score;
    bool       m_game_over;
    bool       m_exit_requested;
    uint32_t   m_last_step_ts;
    uint32_t   m_step_interval_ms;
    int        m_enc_accum;

    void spawnFood();
};
