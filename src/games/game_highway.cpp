#include "game_highway.h"

static const int HORIZON_Y = 18;

GameHighway::GameHighway() {
    m_high_score = 0.0f;
    init();
}

void GameHighway::init() {
    m_car_x = 64.0f;
    m_speed = 130.0f; // km/h
    m_dash_pos = 0.0f;
    m_distance = 0.0f;
    m_turbo = false;
    m_game_over = false;
    m_exit_requested = false;
    m_crash_anim = 0;

    for (int i = 0; i < MAX_TRAFFIC; i++) {
        m_traffic[i].active = false;
        m_traffic[i].dist = 0.0f;
    }

    // Initial setup: Car 0 ahead, Car 1 & 2 as first dual gate behind
    m_traffic[0].active = true;
    m_traffic[0].dist = 0.15f;
    m_traffic[0].lane = 0;
    m_traffic[0].speed = 0.45f;

    spawnDualGate(1, 2, -0.45f);
}

void GameHighway::spawnDualGate(int idx1, int idx2, float at_dist) {
    int open_lane = random(-1, 2); // -1 (Left open), 0 (Center open), 1 (Right open)
    int l1 = -1, l2 = 1;
    if (open_lane == -1) {
        l1 = 0; l2 = 1;
    } else if (open_lane == 0) {
        l1 = -1; l2 = 1;
    } else {
        l1 = -1; l2 = 0;
    }

    m_traffic[idx1].active = true;
    m_traffic[idx1].dist = at_dist;
    m_traffic[idx1].lane = l1;
    m_traffic[idx1].speed = 0.45f;

    m_traffic[idx2].active = true;
    m_traffic[idx2].dist = at_dist;
    m_traffic[idx2].lane = l2;
    m_traffic[idx2].speed = 0.45f;
}

void GameHighway::spawnTraffic(int index) {
    // Find the furthest car behind horizon
    float min_d = 0.0f;
    int last_lane = 0;
    for (int i = 0; i < MAX_TRAFFIC; i++) {
        if (i != index && m_traffic[i].active) {
            if (m_traffic[i].dist < min_d) {
                min_d = m_traffic[i].dist;
                last_lane = m_traffic[i].lane;
            }
        }
    }

    float next_dist = min_d - 0.48f;
    if (next_dist > -0.25f) next_dist = -0.25f;

    // Check if we should spawn a Dual Gate (narrow gap)
    int dual_chance = (m_distance < 200.0f) ? 30 : ((m_distance < 500.0f) ? 55 : 75);
    if (random(0, 100) < dual_chance) {
        // Find another inactive or passed car
        for (int j = 0; j < MAX_TRAFFIC; j++) {
            if (j != index && (!m_traffic[j].active || m_traffic[j].dist > 1.15f)) {
                spawnDualGate(index, j, next_dist);
                return;
            }
        }
    }

    // Single Car Spawn
    m_traffic[index].active = true;
    m_traffic[index].dist = next_dist;
    int new_lane = random(-1, 2);
    if (new_lane == last_lane) {
        new_lane = (last_lane == 1) ? -1 : (last_lane + 1);
    }
    m_traffic[index].lane = new_lane;
    m_traffic[index].speed = 0.45f + ((float)random(-10, 15) / 100.0f);
}

void GameHighway::update(int32_t enc_delta, bool btn_confirm, bool btn_back, bool btn_confirm_held) {
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

    // 1. Steering with EC11 knob (responsive steer)
    if (enc_delta != 0) {
        m_car_x += (float)enc_delta * 3.0f;
    }
    // Road boundaries at bottom
    if (m_car_x < 20.0f) m_car_x = 20.0f;
    if (m_car_x > 108.0f) m_car_x = 108.0f;

    // 2. Progressive Speed Scaling with distance (increases from 130 km/h up to 280+ km/h)
    m_turbo = btn_confirm_held || btn_confirm;
    float base_spd = 130.0f + fminf(m_distance * 0.08f, 150.0f);
    float target_speed = m_turbo ? (base_spd + 75.0f) : base_spd;
    m_speed += (target_speed - m_speed) * 0.08f;

    // 3. Distance & Road animation
    float speed_factor = m_speed / 100.0f;
    m_dash_pos += 0.06f * speed_factor;
    if (m_dash_pos > 1.0f) m_dash_pos -= 1.0f;

    m_distance += 0.4f * speed_factor;
    if (m_distance > m_high_score) {
        m_high_score = m_distance;
    }

    // 4. Update traffic cars with relative speed
    for (int i = 0; i < MAX_TRAFFIC; i++) {
        if (!m_traffic[i].active) continue;

        m_traffic[i].dist += 0.014f * (speed_factor - m_traffic[i].speed);

        // Check if passed player -> respawn
        if (m_traffic[i].dist > 1.18f) {
            spawnTraffic(i);
        }

        // 5. Collision detection near player (dist >= 0.84 and dist <= 1.02)
        if (m_traffic[i].dist >= 0.84f && m_traffic[i].dist <= 1.02f) {
            float enemy_x = 64.0f + (float)m_traffic[i].lane * (m_traffic[i].dist * 40.0f);
            if (fabs(m_car_x - enemy_x) < 12.0f) {
                // CRASH!
                m_game_over = true;
                m_crash_anim = 15;
            }
        }
    }
}

void GameHighway::render() {
    SafeDraw::clearBuffer();

    // 1. Distant Horizon & Equalizer Mountains
    SafeDraw::drawHLine(0, HORIZON_Y, 128);
    for (int col = 0; col < 16; col++) {
        int x = col * 8;
        int h = (int)(3.0f + sinf(col * 0.6f + m_distance * 0.02f) * 3.0f);
        if (h > 0) {
            SafeDraw::drawVLine(x, HORIZON_Y - h, h);
            SafeDraw::drawVLine(x + 1, HORIZON_Y - h, h);
        }
    }

    // 2. 3D Perspective Road Edges
    SafeDraw::drawLine(60, HORIZON_Y, 4, 63);   // Left road edge
    SafeDraw::drawLine(68, HORIZON_Y, 124, 63); // Right road edge

    // Roadside Light Poles
    for (int p = 1; p <= 3; p++) {
        float t = (float)p / 3.0f + (m_dash_pos * 0.33f);
        if (t > 1.0f) t -= 1.0f;
        int py = HORIZON_Y + (int)(t * t * 45.0f);
        int lx = 60 - (int)(t * 56.0f);
        int rx = 68 + (int)(t * 56.0f);
        int pole_h = (int)(t * 12.0f);
        if (py <= 63 && pole_h > 0) {
            SafeDraw::drawVLine(lx, py - pole_h, pole_h);
            SafeDraw::drawDisc(lx, py - pole_h, 1);
            SafeDraw::drawVLine(rx, py - pole_h, pole_h);
            SafeDraw::drawDisc(rx, py - pole_h, 1);
        }
    }

    // Moving Dashed Center Line
    for (int d = 1; d <= 4; d++) {
        float t1 = (float)d / 4.0f + (m_dash_pos * 0.25f);
        if (t1 > 1.0f) t1 -= 1.0f;
        float t2 = t1 + 0.08f;
        if (t2 > 1.0f) t2 = 1.0f;
        int y1 = HORIZON_Y + (int)(t1 * t1 * 45.0f);
        int y2 = HORIZON_Y + (int)(t2 * t2 * 45.0f);
        if (y2 <= 63) {
            SafeDraw::drawLine(64, y1, 64, y2);
        }
    }

    // 3. Traffic Enemy Cars
    for (int i = 0; i < MAX_TRAFFIC; i++) {
        if (!m_traffic[i].active || m_traffic[i].dist <= 0.05f || m_traffic[i].dist > 1.1f) continue;
        float t = m_traffic[i].dist;
        int ey = HORIZON_Y + (int)(t * t * 45.0f);
        int ex = 64 + (int)((float)m_traffic[i].lane * t * 40.0f);

        int car_w = (int)(4.0f + t * 14.0f);
        int car_h = (int)(2.0f + t * 7.0f);
        if (ey < 63 && car_w >= 2 && car_h >= 2) {
            SafeDraw::drawBox(ex - car_w / 2, ey - car_h, car_w, car_h);
            // Roof / window cut
            if (t > 0.4f) {
                SafeDraw::setDrawColor(0);
                SafeDraw::drawBox(ex - car_w / 4, ey - car_h + 1, car_w / 2, car_h / 2);
                SafeDraw::setDrawColor(1);
            }
        }
    }

    // 4. Player's Retro Sports Car (at bottom)
    int cx = (int)m_car_x;
    int cy = 52;

    if (m_game_over && m_crash_anim > 0) {
        // Crash explosion debris
        for (int p = 0; p < 8; p++) {
            int ox = (p % 4 - 2) * (18 - m_crash_anim);
            int oy = (p / 4 - 1) * (18 - m_crash_anim);
            SafeDraw::drawPixel(cx + ox, cy + oy);
            SafeDraw::drawPixel(cx - ox, cy + oy);
        }
    } else {
        // Car Body
        SafeDraw::drawBox(cx - 11, cy, 22, 8);
        // Roof Cabin
        SafeDraw::drawBox(cx - 7, cy - 5, 14, 5);
        // Rear Window (cutout)
        SafeDraw::setDrawColor(0);
        SafeDraw::drawBox(cx - 5, cy - 4, 10, 3);
        SafeDraw::setDrawColor(1);
        // Rear Spoiler
        SafeDraw::drawHLine(cx - 13, cy - 1, 26);
        SafeDraw::drawVLine(cx - 10, cy - 1, 2);
        SafeDraw::drawVLine(cx + 10, cy - 1, 2);
        // Tail lights
        SafeDraw::drawBox(cx - 9, cy + 2, 4, 2);
        SafeDraw::drawBox(cx + 5, cy + 2, 4, 2);
        if (m_turbo) {
            // Nitro flames flare!
            SafeDraw::drawLine(cx - 7, cy + 8, cx - 7, cy + 12);
            SafeDraw::drawLine(cx + 7, cy + 8, cx + 7, cy + 12);
        }
        // Wheels
        SafeDraw::drawBox(cx - 12, cy + 5, 2, 4);
        SafeDraw::drawBox(cx + 10, cy + 5, 2, 4);
    }

    // 5. HUD Display at Top
    SafeDraw::setFont(u8g2_font_4x6_tr);
    char hud_l[32], hud_r[32];
    snprintf(hud_l, sizeof(hud_l), "%s %d KM/H", m_turbo ? "TURBO" : "SPD", (int)m_speed);
    snprintf(hud_r, sizeof(hud_r), "DIST %dM", (int)m_distance);
    SafeDraw::drawStr(2, 7, hud_l);
    SafeDraw::drawStr(SCREEN_W - SafeDraw::getStrWidth(hud_r) - 2, 7, hud_r);

    // 6. Game Over Screen
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
        const char *t1 = "CRASH! GAME OVER";
        SafeDraw::drawStr(bx + (bw - SafeDraw::getStrWidth(t1)) / 2, by + 12, t1);

        SafeDraw::setFont(u8g2_font_4x6_tr);
        char sc_str[32];
        snprintf(sc_str, sizeof(sc_str), "SCORE: %dM | BEST: %dM", (int)m_distance, (int)m_high_score);
        SafeDraw::drawStr(bx + (bw - SafeDraw::getStrWidth(sc_str)) / 2, by + 22, sc_str);

        const char *sub = "CONFIRM: REPLAY | BACK: MENU";
        SafeDraw::drawStr(bx + (bw - SafeDraw::getStrWidth(sub)) / 2, by + 31, sub);
    }

    SafeDraw::sendBuffer();
}
