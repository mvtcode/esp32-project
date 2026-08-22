#include "game_sky_fighter.h"

static const int PLAYER_Y = 52;

GameSkyFighter::GameSkyFighter() {
    m_high_score = 0;
    init();
}

void GameSkyFighter::init() {
    m_player_x = SCREEN_W / 2.0f;
    m_lives = 3;
    m_score = 0;
    m_weapon_lvl = 1;
    m_bombs = 1;
    m_game_over = false;
    m_exit_requested = false;
    m_last_fire_ts = 0;
    m_last_spawn_ts = millis();
    m_bomb_flash_ts = 0;

    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) m_bullets[i].active = false;
    for (int i = 0; i < MAX_ENEMIES; i++) m_enemies[i].active = false;
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) m_enemy_bullets[i].active = false;
    for (int i = 0; i < MAX_EXPLOSIONS * 4; i++) m_particles[i].life = 0;
    m_powerup.active = false;

    for (int i = 0; i < 4; i++) {
        m_cloud_x[i] = random(10, SCREEN_W - 10);
        m_cloud_y[i] = i * 16;
    }
}

void GameSkyFighter::shoot() {
    uint32_t now = millis();
    if (now - m_last_fire_ts < 150) return;
    m_last_fire_ts = now;

    if (m_weapon_lvl == 1) {
        for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
            if (!m_bullets[i].active) {
                m_bullets[i].active = true;
                m_bullets[i].x = m_player_x;
                m_bullets[i].y = PLAYER_Y - 4;
                m_bullets[i].vx = 0.0f;
                m_bullets[i].vy = -3.5f;
                break;
            }
        }
    } else if (m_weapon_lvl == 2) {
        int spawned = 0;
        for (int i = 0; i < MAX_PLAYER_BULLETS && spawned < 2; i++) {
            if (!m_bullets[i].active) {
                m_bullets[i].active = true;
                m_bullets[i].x = m_player_x + (spawned == 0 ? -4.0f : 4.0f);
                m_bullets[i].y = PLAYER_Y - 2;
                m_bullets[i].vx = 0.0f;
                m_bullets[i].vy = -3.5f;
                spawned++;
            }
        }
    } else {
        // Level 3: Triple Spread
        int spawned = 0;
        float vxs[3] = {-0.8f, 0.0f, 0.8f};
        float oxs[3] = {-5.0f, 0.0f, 5.0f};
        for (int i = 0; i < MAX_PLAYER_BULLETS && spawned < 3; i++) {
            if (!m_bullets[i].active) {
                m_bullets[i].active = true;
                m_bullets[i].x = m_player_x + oxs[spawned];
                m_bullets[i].y = PLAYER_Y - 3;
                m_bullets[i].vx = vxs[spawned];
                m_bullets[i].vy = -3.4f;
                spawned++;
            }
        }
    }
}

void GameSkyFighter::triggerBomb() {
    if (m_bombs <= 0) return;
    m_bombs--;
    m_bomb_flash_ts = millis();

    // Clear all enemy bullets and damage all enemies
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) m_enemy_bullets[i].active = false;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (m_enemies[i].active) {
            spawnExplosion(m_enemies[i].x, m_enemies[i].y);
            m_enemies[i].active = false;
            m_score += 50;
            if (m_score > m_high_score) m_high_score = m_score;
        }
    }
}

void GameSkyFighter::spawnEnemy() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!m_enemies[i].active) {
            m_enemies[i].active = true;
            m_enemies[i].x = random(12, SCREEN_W - 12);
            m_enemies[i].y = -8.0f;

            int roll = random(0, 100);
            if (roll < 55) {
                m_enemies[i].type = 0; // Scout
                m_enemies[i].hp = 1;
                m_enemies[i].vx = ((float)random(-100, 100) / 100.0f) * 0.6f;
                m_enemies[i].vy = 0.8f;
            } else if (roll < 85) {
                m_enemies[i].type = 1; // Fighter
                m_enemies[i].hp = 2;
                m_enemies[i].vx = 0.0f;
                m_enemies[i].vy = 0.65f;
            } else {
                m_enemies[i].type = 2; // Bomber
                m_enemies[i].hp = 4;
                m_enemies[i].vx = 0.0f;
                m_enemies[i].vy = 0.4f;
            }
            break;
        }
    }
}

void GameSkyFighter::spawnExplosion(float x, float y) {
    int count = 0;
    for (int i = 0; i < MAX_EXPLOSIONS * 4 && count < 6; i++) {
        if (m_particles[i].life <= 0) {
            m_particles[i].x = x;
            m_particles[i].y = y;
            m_particles[i].vx = ((float)random(-100, 100) / 100.0f) * 1.5f;
            m_particles[i].vy = ((float)random(-100, 100) / 100.0f) * 1.5f;
            m_particles[i].life = random(6, 12);
            count++;
        }
    }
}

void GameSkyFighter::update(int32_t enc_delta, bool btn_confirm, bool btn_back, bool btn_confirm_held) {
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

    // 1. Player Plane Steering with EC11 knob
    if (enc_delta != 0) {
        m_player_x += (float)enc_delta * 2.2f;
        if (m_player_x < 8.0f) m_player_x = 8.0f;
        if (m_player_x > SCREEN_W - 8.0f) m_player_x = SCREEN_W - 8.0f;
    }

    // 2. Shooting
    if (btn_confirm || btn_confirm_held) {
        shoot();
    }

    // 3. Background Cloud Scroll
    for (int i = 0; i < 4; i++) {
        m_cloud_y[i] += 0.5f;
        if (m_cloud_y[i] > SCREEN_H) {
            m_cloud_y[i] = -10.0f;
            m_cloud_x[i] = random(10, SCREEN_W - 10);
        }
    }

    // 4. Update Player Bullets
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (!m_bullets[i].active) continue;
        m_bullets[i].x += m_bullets[i].vx;
        m_bullets[i].y += m_bullets[i].vy;

        if (m_bullets[i].y < 0 || m_bullets[i].x < 0 || m_bullets[i].x > SCREEN_W) {
            m_bullets[i].active = false;
            continue;
        }

        // Bullet hit enemy check
        for (int e = 0; e < MAX_ENEMIES; e++) {
            if (!m_enemies[e].active) continue;
            float hit_box = (m_enemies[e].type == 2) ? 8.0f : 5.0f;
            if (fabs(m_bullets[i].x - m_enemies[e].x) < hit_box &&
                fabs(m_bullets[i].y - m_enemies[e].y) < hit_box) {
                m_bullets[i].active = false;
                m_enemies[e].hp--;
                if (m_enemies[e].hp <= 0) {
                    spawnExplosion(m_enemies[e].x, m_enemies[e].y);
                    m_enemies[e].active = false;
                    m_score += (m_enemies[e].type + 1) * 20;
                    if (m_score > m_high_score) m_high_score = m_score;

                    // Chance to drop power-up
                    if (!m_powerup.active && random(0, 100) < 30) {
                        m_powerup.active = true;
                        m_powerup.x = m_enemies[e].x;
                        m_powerup.y = m_enemies[e].y;
                        m_powerup.type = (random(0, 100) < 70) ? 0 : 1; // 0=Gun, 1=Bomb
                    }
                }
                break;
            }
        }
    }

    // 5. Update Enemies
    uint32_t now = millis();
    if (now - m_last_spawn_ts > 900) {
        m_last_spawn_ts = now;
        spawnEnemy();
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!m_enemies[i].active) continue;
        if (m_enemies[i].type == 0) {
            m_enemies[i].x += sinf(m_enemies[i].y * 0.15f) * 0.9f;
        }
        m_enemies[i].y += m_enemies[i].vy;

        if (m_enemies[i].y > SCREEN_H + 8) {
            m_enemies[i].active = false;
            continue;
        }

        // Enemy firing
        if (m_enemies[i].type >= 1 && random(0, 100) < 2) {
            for (int b = 0; b < MAX_ENEMY_BULLETS; b++) {
                if (!m_enemy_bullets[b].active) {
                    m_enemy_bullets[b].active = true;
                    m_enemy_bullets[b].x = m_enemies[i].x;
                    m_enemy_bullets[b].y = m_enemies[i].y + 4;
                    m_enemy_bullets[b].vx = 0.0f;
                    m_enemy_bullets[b].vy = 1.6f;
                    break;
                }
            }
        }

        // Collision with player
        if (fabs(m_enemies[i].x - m_player_x) < 8.0f && fabs(m_enemies[i].y - PLAYER_Y) < 6.0f) {
            spawnExplosion(m_enemies[i].x, m_enemies[i].y);
            m_enemies[i].active = false;
            m_lives--;
            if (m_weapon_lvl > 1) m_weapon_lvl--;
            if (m_lives <= 0) m_game_over = true;
        }
    }

    // 6. Update Enemy Bullets
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!m_enemy_bullets[i].active) continue;
        m_enemy_bullets[i].y += m_enemy_bullets[i].vy;

        if (m_enemy_bullets[i].y > SCREEN_H) {
            m_enemy_bullets[i].active = false;
            continue;
        }

        if (fabs(m_enemy_bullets[i].x - m_player_x) < 6.0f &&
            fabs(m_enemy_bullets[i].y - PLAYER_Y) < 5.0f) {
            m_enemy_bullets[i].active = false;
            spawnExplosion(m_player_x, PLAYER_Y);
            m_lives--;
            if (m_weapon_lvl > 1) m_weapon_lvl--;
            if (m_lives <= 0) m_game_over = true;
        }
    }

    // 7. Update Power-up Item
    if (m_powerup.active) {
        m_powerup.y += 0.6f;
        if (m_powerup.y > SCREEN_H) m_powerup.active = false;

        if (fabs(m_powerup.x - m_player_x) < 8.0f && fabs(m_powerup.y - PLAYER_Y) < 6.0f) {
            m_powerup.active = false;
            if (m_powerup.type == 0) {
                if (m_weapon_lvl < 3) m_weapon_lvl++;
                m_score += 50;
            } else {
                if (m_bombs < 3) m_bombs++;
                m_score += 50;
            }
        }
    }

    // 8. Update Explosion Particles
    for (int i = 0; i < MAX_EXPLOSIONS * 4; i++) {
        if (m_particles[i].life > 0) {
            m_particles[i].x += m_particles[i].vx;
            m_particles[i].y += m_particles[i].vy;
            m_particles[i].life--;
        }
    }
}

void GameSkyFighter::render() {
    SafeDraw::clearBuffer();

    // 1. Bomb Flash effect
    if (m_bomb_flash_ts > 0 && millis() - m_bomb_flash_ts < 120) {
        SafeDraw::drawBox(0, 0, SCREEN_W, SCREEN_H);
        SafeDraw::sendBuffer();
        return;
    }

    // 2. Background Clouds
    for (int i = 0; i < 4; i++) {
        int cx = (int)m_cloud_x[i];
        int cy = (int)m_cloud_y[i];
        SafeDraw::drawCircle(cx, cy, 5);
        SafeDraw::drawCircle(cx + 6, cy - 1, 7);
        SafeDraw::drawCircle(cx + 12, cy, 5);
    }

    // 3. Power-up Drop
    if (m_powerup.active) {
        int px = (int)m_powerup.x;
        int py = (int)m_powerup.y;
        SafeDraw::drawFrame(px - 4, py - 4, 9, 9);
        SafeDraw::setFont(u8g2_font_4x6_tr);
        SafeDraw::drawStr(px - 2, py + 3, m_powerup.type == 0 ? "P" : "B");
    }

    // 4. Enemy Planes
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!m_enemies[i].active) continue;
        int ex = (int)m_enemies[i].x;
        int ey = (int)m_enemies[i].y;

        if (m_enemies[i].type == 0) {
            // Scout: small triangular jet
            SafeDraw::drawTriangle(ex, ey + 4, ex - 4, ey - 3, ex + 4, ey - 3);
        } else if (m_enemies[i].type == 1) {
            // Fighter: twin wing
            SafeDraw::drawBox(ex - 5, ey - 1, 11, 2);
            SafeDraw::drawVLine(ex, ey - 4, 8);
            SafeDraw::drawBox(ex - 3, ey + 2, 7, 2);
        } else {
            // Heavy Bomber: wide body
            SafeDraw::drawBox(ex - 8, ey - 2, 17, 3);
            SafeDraw::drawBox(ex - 3, ey - 5, 7, 10);
            SafeDraw::drawPixel(ex - 6, ey + 2);
            SafeDraw::drawPixel(ex + 6, ey + 2);
        }
    }

    // 5. Player Aircraft (Fighter)
    int px = (int)m_player_x;
    int py = PLAYER_Y;
    // Fuselage
    SafeDraw::drawVLine(px, py - 6, 12);
    // Main Wings
    SafeDraw::drawBox(px - 7, py - 1, 15, 2);
    // Wing Guns
    SafeDraw::drawVLine(px - 6, py - 4, 3);
    SafeDraw::drawVLine(px + 6, py - 4, 3);
    // Tail
    SafeDraw::drawHLine(px - 3, py + 4, 7);
    // Engine Flare
    SafeDraw::drawPixel(px, py + 6);

    // 6. Bullets
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (m_bullets[i].active) {
            SafeDraw::drawVLine((int)m_bullets[i].x, (int)m_bullets[i].y, 3);
        }
    }
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (m_enemy_bullets[i].active) {
            SafeDraw::drawBox((int)m_enemy_bullets[i].x - 1, (int)m_enemy_bullets[i].y - 1, 2, 2);
        }
    }

    // 7. Explosion Particles
    for (int i = 0; i < MAX_EXPLOSIONS * 4; i++) {
        if (m_particles[i].life > 0) {
            SafeDraw::drawPixel((int)m_particles[i].x, (int)m_particles[i].y);
        }
    }

    // 8. Top HUD
    SafeDraw::setFont(u8g2_font_4x6_tr);
    char sc_str[24], st_str[24];
    snprintf(sc_str, sizeof(sc_str), "SC:%d", m_score);
    snprintf(st_str, sizeof(st_str), "HP:%d LV:%d B:%d", m_lives, m_weapon_lvl, m_bombs);
    SafeDraw::drawStr(2, 6, sc_str);
    SafeDraw::drawStr(SCREEN_W - SafeDraw::getStrWidth(st_str) - 2, 6, st_str);

    // 9. Game Over Screen
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
        snprintf(s_sub, sizeof(s_sub), "SCORE: %d | BEST: %d", m_score, m_high_score);
        SafeDraw::drawStr(bx + (bw - SafeDraw::getStrWidth(s_sub)) / 2, by + 22, s_sub);

        const char *sub2 = "CONFIRM: REPLAY | BACK: MENU";
        SafeDraw::drawStr(bx + (bw - SafeDraw::getStrWidth(sub2)) / 2, by + 30, sub2);
    }

    SafeDraw::sendBuffer();
}
