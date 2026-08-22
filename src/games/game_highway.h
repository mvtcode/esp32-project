#pragma once
#include "game_base.h"

static const int MAX_TRAFFIC = 4;

struct TrafficCar {
    float dist;     // 0.0 (horizon) to 1.0 (player)
    int   lane;     // -1 (Left), 0 (Center), 1 (Right)
    float speed;    // relative speed
    bool  active;
};

class GameHighway : public GameBase {
public:
    GameHighway();
    virtual const char* getName() const override { return "HIGHWAY RACER (3D)"; }
    virtual void init() override;
    virtual void update(int32_t enc_delta, bool btn_confirm, bool btn_back, bool btn_confirm_held = false) override;
    virtual void render() override;
    virtual bool isExitRequested() const override { return m_exit_requested; }

private:
    float m_car_x;
    float m_speed;
    float m_dash_pos;
    float m_distance;
    float m_high_score;
    bool  m_turbo;
    bool  m_game_over;
    bool  m_exit_requested;
    int   m_crash_anim;
    TrafficCar m_traffic[MAX_TRAFFIC];

    void spawnTraffic(int index);
    void spawnDualGate(int idx1, int idx2, float at_dist);
};
