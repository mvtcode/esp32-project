#include "game_pong.h"

static const int PADDLE_H = 14;
static const int PADDLE_W = 3;
static const int P1_X = 4;
static const int CPU_X = SCREEN_W - 4 - PADDLE_W;
static const int WIN_SCORE = 5;

GamePong::GamePong() {
    init();
}

void GamePong::init() {
    m_p1_y = (SCREEN_H - PADDLE_H) / 2.0f;
    m_cpu_y = (SCREEN_H - PADDLE_H) / 2.0f;
    m_score_p1 = 0;
    m_score_cpu = 0;
    m_game_over = false;
    m_p1_won = false;
    m_exit_requested = false;
    m_flash_frame = 0;
    m_last_update_ts = millis();
    resetBall(true);
}

void GamePong::resetBall(bool serve_to_player) {
    m_ball_x = SCREEN_W / 2.0f;
    m_ball_y = SCREEN_H / 2.0f;
    m_rally_count = 0;
    float speed = 1.4f;
    m_ball_vx = serve_to_player ? -speed : speed;
    m_ball_vy = ((float)(random(-100, 100)) / 100.0f) * 0.9f;
    if (fabs(m_ball_vy) < 0.2f) m_ball_vy = (m_ball_vy < 0) ? -0.4f : 0.4f;
    m_serving = true;
}

void GamePong::update(int32_t enc_delta, bool btn_confirm, bool btn_back, bool /*btn_confirm_held*/) {
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

    // 1. Player paddle movement via EC11
    if (enc_delta != 0) {
        m_p1_y += (float)enc_delta * 1.5f;
        if (m_p1_y < 1.0f) m_p1_y = 1.0f;
        if (m_p1_y > SCREEN_H - PADDLE_H - 1.0f) m_p1_y = SCREEN_H - PADDLE_H - 1.0f;
    }

    if (m_serving) {
        if (btn_confirm || enc_delta != 0) {
            m_serving = false;
        }
        return;
    }

    // 2. CPU Paddle AI tracking with slight delay/imperfection
    float cpu_target = m_ball_y - (PADDLE_H / 2.0f);
    // Add small error if ball is moving away
    if (m_ball_vx < 0) {
        cpu_target = (SCREEN_H - PADDLE_H) / 2.0f;
    }
    float cpu_diff = cpu_target - m_cpu_y;
    float cpu_speed = 0.95f + (float)m_rally_count * 0.04f;
    if (cpu_speed > 1.8f) cpu_speed = 1.8f;
    if (fabs(cpu_diff) > 1.0f) {
        m_cpu_y += (cpu_diff > 0 ? 1.0f : -1.0f) * fminf(fabs(cpu_diff) * 0.2f, cpu_speed);
    }
    if (m_cpu_y < 1.0f) m_cpu_y = 1.0f;
    if (m_cpu_y > SCREEN_H - PADDLE_H - 1.0f) m_cpu_y = SCREEN_H - PADDLE_H - 1.0f;

    // 3. Ball Physics & Movement
    m_ball_x += m_ball_vx;
    m_ball_y += m_ball_vy;

    // Top / Bottom wall bounce
    if (m_ball_y <= 2.0f) {
        m_ball_y = 2.0f;
        m_ball_vy = -m_ball_vy;
    } else if (m_ball_y >= SCREEN_H - 3.0f) {
        m_ball_y = SCREEN_H - 3.0f;
        m_ball_vy = -m_ball_vy;
    }

    // 4. Collision with Player Paddle (Left)
    if (m_ball_x <= P1_X + PADDLE_W + 1.0f && m_ball_x >= P1_X - 1.0f) {
        if (m_ball_y >= m_p1_y - 2.0f && m_ball_y <= m_p1_y + PADDLE_H + 2.0f) {
            m_ball_x = P1_X + PADDLE_W + 1.0f;
            float hit_offset = (m_ball_y - (m_p1_y + PADDLE_H / 2.0f)) / (PADDLE_H / 2.0f);
            m_rally_count++;
            float speed = 1.4f + fminf((float)m_rally_count * 0.08f, 1.8f);
            m_ball_vx = speed;
            m_ball_vy = hit_offset * 1.5f;
            m_flash_frame = 2;
        }
    }

    // 5. Collision with CPU Paddle (Right)
    if (m_ball_x >= CPU_X - 1.0f && m_ball_x <= CPU_X + PADDLE_W + 1.0f) {
        if (m_ball_y >= m_cpu_y - 2.0f && m_ball_y <= m_cpu_y + PADDLE_H + 2.0f) {
            m_ball_x = CPU_X - 1.0f;
            float hit_offset = (m_ball_y - (m_cpu_y + PADDLE_H / 2.0f)) / (PADDLE_H / 2.0f);
            m_rally_count++;
            float speed = 1.4f + fminf((float)m_rally_count * 0.08f, 1.8f);
            m_ball_vx = -speed;
            m_ball_vy = hit_offset * 1.5f;
            m_flash_frame = 2;
        }
    }

    // 6. Score check (Out of bounds)
    if (m_ball_x < 0) {
        m_score_cpu++;
        if (m_score_cpu >= WIN_SCORE) {
            m_game_over = true;
            m_p1_won = false;
        } else {
            resetBall(true);
        }
    } else if (m_ball_x > SCREEN_W) {
        m_score_p1++;
        if (m_score_p1 >= WIN_SCORE) {
            m_game_over = true;
            m_p1_won = true;
        } else {
            resetBall(false);
        }
    }
}

void GamePong::render() {
    SafeDraw::clearBuffer();

    // 1. Dashed center court line
    for (int y = 0; y < SCREEN_H; y += 6) {
        SafeDraw::drawVLine(SCREEN_W / 2, y, 3);
    }

    // 2. Scoreboard at top
    SafeDraw::setFont(u8g2_font_6x10_tf);
    char s1[8], s2[8];
    snprintf(s1, sizeof(s1), "%d", m_score_p1);
    snprintf(s2, sizeof(s2), "%d", m_score_cpu);
    SafeDraw::drawStr(SCREEN_W / 2 - 18, 10, s1);
    SafeDraw::drawStr(SCREEN_W / 2 + 12, 10, s2);

    // 3. Paddles
    SafeDraw::drawBox(P1_X, (int)m_p1_y, PADDLE_W, PADDLE_H);
    SafeDraw::drawBox(CPU_X, (int)m_cpu_y, PADDLE_W, PADDLE_H);

    // 4. Ball (2x2 pixel square)
    SafeDraw::drawBox((int)m_ball_x - 1, (int)m_ball_y - 1, 3, 3);

    // 5. Serving hint
    if (m_serving && !m_game_over) {
        SafeDraw::setFont(u8g2_font_4x6_tr);
        const char *hint = "ROTATE / CONFIRM TO SERVE";
        SafeDraw::drawStr((SCREEN_W - SafeDraw::getStrWidth(hint)) / 2, SCREEN_H - 4, hint);
    }

    // 6. Game Over / Victory Screen
    if (m_game_over) {
        int bw = 100;
        int bh = 34;
        int bx = (SCREEN_W - bw) / 2;
        int by = (SCREEN_H - bh) / 2;

        SafeDraw::setDrawColor(0);
        SafeDraw::drawBox(bx, by, bw, bh);
        SafeDraw::setDrawColor(1);
        SafeDraw::drawRFrame(bx, by, bw, bh, 2);

        SafeDraw::setFont(u8g2_font_6x10_tf);
        const char *res = m_p1_won ? "YOU WIN! :)" : "CPU WINS! :(";
        SafeDraw::drawStr(bx + (bw - SafeDraw::getStrWidth(res)) / 2, by + 13, res);

        SafeDraw::setFont(u8g2_font_4x6_tr);
        const char *sub = "PRESS CONFIRM TO REPLAY";
        SafeDraw::drawStr(bx + (bw - SafeDraw::getStrWidth(sub)) / 2, by + 23, sub);
        const char *sub2 = "PRESS BACK FOR MENU";
        SafeDraw::drawStr(bx + (bw - SafeDraw::getStrWidth(sub2)) / 2, by + 30, sub2);
    }

    SafeDraw::sendBuffer();
}
