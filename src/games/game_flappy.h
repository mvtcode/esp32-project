#pragma once
#include "game_base.h"

struct FlappyPipe {
    float x;
    int   gap_y;
    int   gap_h;
    bool  scored;
};

class GameFlappy : public GameBase {
public:
    GameFlappy();
    virtual const char* getName() const override { return "FLAPPY BIRD"; }
    virtual void init() override;
    virtual void update(int32_t enc_delta, bool btn_confirm, bool btn_back, bool btn_confirm_held = false) override;
    virtual void render() override;
    virtual bool isExitRequested() const override { return m_exit_requested; }

private:
    float m_bird_y;
    float m_bird_vy;
    int   m_score;
    int   m_high_score;
    bool  m_game_over;
    bool  m_started;
    bool  m_exit_requested;
    FlappyPipe m_pipes[2];

    void resetPipes();
};
