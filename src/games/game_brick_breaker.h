#pragma once
#include "game_base.h"

static const int BRICK_ROWS = 4;
static const int BRICK_COLS = 8;

class GameBrickBreaker : public GameBase {
public:
    GameBrickBreaker();
    virtual const char* getName() const override { return "BRICK BREAKER"; }
    virtual void init() override;
    virtual void update(int32_t enc_delta, bool btn_confirm, bool btn_back, bool btn_confirm_held = false) override;
    virtual void render() override;
    virtual bool isExitRequested() const override { return m_exit_requested; }

private:
    float m_paddle_x;
    int   m_paddle_w;
    float m_ball_x;
    float m_ball_y;
    float m_ball_vx;
    float m_ball_vy;
    bool  m_ball_attached;
    int   m_lives;
    int   m_score;
    int   m_high_score;
    bool  m_game_over;
    bool  m_game_won;
    bool  m_exit_requested;
    uint8_t m_bricks[BRICK_ROWS][BRICK_COLS];

    void resetBricks();
    void resetBall();
};
