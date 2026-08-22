#pragma once
#include "game_base.h"

static const int ALIEN_ROWS = 3;
static const int ALIEN_COLS = 6;
static const int MAX_BULLETS = 4;
static const int MAX_BOMBS = 3;

struct Bullet {
    float x;
    float y;
    bool active;
};

class GameInvaders : public GameBase {
public:
    GameInvaders();
    virtual const char* getName() const override { return "SPACE INVADERS"; }
    virtual void init() override;
    virtual void update(int32_t enc_delta, bool btn_confirm, bool btn_back, bool btn_confirm_held = false) override;
    virtual void render() override;
    virtual bool isExitRequested() const override { return m_exit_requested; }

private:
    float  m_ship_x;
    int    m_lives;
    int    m_score;
    int    m_high_score;
    bool   m_game_over;
    bool   m_game_won;
    bool   m_exit_requested;

    uint8_t m_aliens[ALIEN_ROWS][ALIEN_COLS];
    float   m_fleet_x;
    float   m_fleet_y;
    float   m_fleet_vx;
    uint32_t m_last_fleet_move_ts;
    uint32_t m_fleet_step_ms;

    Bullet m_bullets[MAX_BULLETS];
    Bullet m_bombs[MAX_BOMBS];

    void resetFleet();
    void shootBullet();
    void spawnBomb();
};
