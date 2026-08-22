#pragma once
#include "game_base.h"

static const int MAX_PLAYER_BULLETS = 12;
static const int MAX_ENEMIES = 5;
static const int MAX_ENEMY_BULLETS = 6;
static const int MAX_EXPLOSIONS = 6;

struct PlaneBullet {
    float x;
    float y;
    float vx;
    float vy;
    bool  active;
};

struct PlaneEnemy {
    float x;
    float y;
    float vx;
    float vy;
    int   type; // 0=Scout, 1=Fighter, 2=Heavy Bomber
    int   hp;
    bool  active;
};

struct PlaneParticle {
    float x;
    float y;
    float vx;
    float vy;
    int   life;
};

struct PowerUpItem {
    float x;
    float y;
    int   type; // 0=PowerGun, 1=Bomb
    bool  active;
};

class GameSkyFighter : public GameBase {
public:
    GameSkyFighter();
    virtual const char* getName() const override { return "SKY FIGHTER 1942"; }
    virtual void init() override;
    virtual void update(int32_t enc_delta, bool btn_confirm, bool btn_back, bool btn_confirm_held = false) override;
    virtual void render() override;
    virtual bool isExitRequested() const override { return m_exit_requested; }

private:
    float m_player_x;
    int   m_lives;
    int   m_score;
    int   m_high_score;
    int   m_weapon_lvl; // 1, 2, 3
    int   m_bombs;
    bool  m_game_over;
    bool  m_exit_requested;

    uint32_t m_last_fire_ts;
    uint32_t m_last_spawn_ts;
    uint32_t m_bomb_flash_ts;

    PlaneBullet   m_bullets[MAX_PLAYER_BULLETS];
    PlaneEnemy    m_enemies[MAX_ENEMIES];
    PlaneBullet   m_enemy_bullets[MAX_ENEMY_BULLETS];
    PlaneParticle m_particles[MAX_EXPLOSIONS * 4];
    PowerUpItem   m_powerup;

    float m_cloud_y[4];
    float m_cloud_x[4];

    void shoot();
    void triggerBomb();
    void spawnEnemy();
    void spawnExplosion(float x, float y);
};
