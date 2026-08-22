#pragma once
#include "game_base.h"

class GamePong : public GameBase {
public:
    GamePong();
    virtual const char* getName() const override { return "RETRO PONG (AI)"; }
    virtual void init() override;
    virtual void update(int32_t enc_delta, bool btn_confirm, bool btn_back, bool btn_confirm_held = false) override;
    virtual void render() override;
    virtual bool isExitRequested() const override { return m_exit_requested; }

private:
    float m_p1_y;
    float m_cpu_y;
    float m_ball_x;
    float m_ball_y;
    float m_ball_vx;
    float m_ball_vy;
    int   m_score_p1;
    int   m_score_cpu;
    int   m_rally_count;
    bool  m_serving;
    bool  m_game_over;
    bool  m_p1_won;
    bool  m_exit_requested;
    uint32_t m_last_update_ts;
    int   m_flash_frame;

    void resetBall(bool serve_to_player);
};
