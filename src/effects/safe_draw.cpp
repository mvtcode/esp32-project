#include "safe_draw.h"
#include "effect_common.h"

// Instantiate the global helper instance
SafeDraw gfx;

// -----------------------------------------------------------------------
// Cohen-Sutherland Line Clipping Algorithm
// -----------------------------------------------------------------------
static const uint8_t CS_INSIDE = 0; // 0000
static const uint8_t CS_LEFT   = 1; // 0001
static const uint8_t CS_RIGHT  = 2; // 0010
static const uint8_t CS_TOP    = 4; // 0100 (y < 0)
static const uint8_t CS_BOTTOM = 8; // 1000 (y > 63)

static inline uint8_t compute_outcode(int x, int y, int xmin, int ymin, int xmax, int ymax) {
    uint8_t code = CS_INSIDE;
    if (x < xmin)      code |= CS_LEFT;
    else if (x > xmax) code |= CS_RIGHT;
    if (y < ymin)      code |= CS_TOP;
    else if (y > ymax) code |= CS_BOTTOM;
    return code;
}

void SafeDraw::drawPixel(int x, int y) {
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        u8g2.drawPixel(x, y);
    }
}

void SafeDraw::drawLine(int x0, int y0, int x1, int y1) {
    const int xmin = 0;
    const int ymin = 0;
    const int xmax = SCREEN_W - 1; // 127
    const int ymax = SCREEN_H - 1; // 63

    uint8_t code0 = compute_outcode(x0, y0, xmin, ymin, xmax, ymax);
    uint8_t code1 = compute_outcode(x1, y1, xmin, ymin, xmax, ymax);

    while (true) {
        if (!(code0 | code1)) {
            // Both endpoints within viewport -> draw directly
            u8g2.drawLine(x0, y0, x1, y1);
            break;
        } else if (code0 & code1) {
            // Both endpoints share an outside zone -> trivially reject
            break;
        } else {
            // At least one endpoint is outside
            uint8_t code_out = code0 ? code0 : code1;
            int x = 0;
            int y = 0;

            // Find intersection point using 64-bit math to prevent overflow
            if (code_out & CS_BOTTOM) {
                x = x0 + (int)((int64_t)(x1 - x0) * (ymax - y0) / (y1 - y0));
                y = ymax;
            } else if (code_out & CS_TOP) {
                x = x0 + (int)((int64_t)(x1 - x0) * (ymin - y0) / (y1 - y0));
                y = ymin;
            } else if (code_out & CS_RIGHT) {
                y = y0 + (int)((int64_t)(y1 - y0) * (xmax - x0) / (x1 - x0));
                x = xmax;
            } else if (code_out & CS_LEFT) {
                y = y0 + (int)((int64_t)(y1 - y0) * (xmin - x0) / (x1 - x0));
                x = xmin;
            }

            if (code_out == code0) {
                x0 = x;
                y0 = y;
                code0 = compute_outcode(x0, y0, xmin, ymin, xmax, ymax);
            } else {
                x1 = x;
                y1 = y;
                code1 = compute_outcode(x1, y1, xmin, ymin, xmax, ymax);
            }
        }
    }
}

void SafeDraw::drawHLine(int x, int y, int w) {
    if (y < 0 || y >= SCREEN_H || w == 0) return;
    if (w < 0) {
        x += w;
        w = -w;
    }
    int x2 = x + w;
    if (x < 0) x = 0;
    if (x2 > SCREEN_W) x2 = SCREEN_W;
    if (x2 > x) {
        u8g2.drawHLine(x, y, x2 - x);
    }
}

void SafeDraw::drawVLine(int x, int y, int h) {
    if (x < 0 || x >= SCREEN_W || h == 0) return;
    if (h < 0) {
        y += h;
        h = -h;
    }
    int y2 = y + h;
    if (y < 0) y = 0;
    if (y2 > SCREEN_H) y2 = SCREEN_H;
    if (y2 > y) {
        u8g2.drawVLine(x, y, y2 - y);
    }
}

void SafeDraw::drawBox(int x, int y, int w, int h) {
    if (w == 0 || h == 0) return;
    if (w < 0) { x += w; w = -w; }
    if (h < 0) { y += h; h = -h; }
    int x2 = x + w;
    int y2 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x2 > SCREEN_W) x2 = SCREEN_W;
    if (y2 > SCREEN_H) y2 = SCREEN_H;
    if (x2 > x && y2 > y) {
        u8g2.drawBox(x, y, x2 - x, y2 - y);
    }
}

void SafeDraw::drawFrame(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) return;
    drawHLine(x, y, w);
    drawHLine(x, y + h - 1, w);
    drawVLine(x, y, h);
    drawVLine(x + w - 1, y, h);
}

void SafeDraw::drawCircle(int x0, int y0, int r, uint8_t opt) {
    if (r <= 0) return;
    // Fast rejection if circle is completely outside screen
    if (x0 + r < 0 || x0 - r >= SCREEN_W || y0 + r < 0 || y0 - r >= SCREEN_H) return;

    // If completely inside screen, call u8g2 directly
    if (x0 - r >= 0 && x0 + r < SCREEN_W && y0 - r >= 0 && y0 + r < SCREEN_H) {
        u8g2.drawCircle(x0, y0, r, opt);
    } else {
        // Clipped midpoint circle algorithm for boundary-crossing circles
        int x = 0;
        int y = r;
        int d = 1 - r;

        auto plot_safe_points = [x0, y0, opt](int cx, int cy) {
            if (opt & U8G2_DRAW_UPPER_RIGHT) {
                drawPixel(x0 + cx, y0 - cy);
                drawPixel(x0 + cy, y0 - cx);
            }
            if (opt & U8G2_DRAW_UPPER_LEFT) {
                drawPixel(x0 - cx, y0 - cy);
                drawPixel(x0 - cy, y0 - cx);
            }
            if (opt & U8G2_DRAW_LOWER_RIGHT) {
                drawPixel(x0 + cx, y0 + cy);
                drawPixel(x0 + cy, y0 + cx);
            }
            if (opt & U8G2_DRAW_LOWER_LEFT) {
                drawPixel(x0 - cx, y0 + cy);
                drawPixel(x0 - cy, y0 + cx);
            }
        };

        plot_safe_points(x, y);
        while (y > x) {
            if (d < 0) {
                d += 2 * x + 3;
            } else {
                d += 2 * (x - y) + 5;
                y--;
            }
            x++;
            plot_safe_points(x, y);
        }
    }
}

void SafeDraw::drawEllipse(int x0, int y0, int rx, int ry, uint8_t opt) {
    if (rx <= 0 || ry <= 0) return;
    if (x0 + rx < 0 || x0 - rx >= SCREEN_W || y0 + ry < 0 || y0 - ry >= SCREEN_H) return;

    if (x0 - rx >= 0 && x0 + rx < SCREEN_W && y0 - ry >= 0 && y0 + ry < SCREEN_H) {
        u8g2.drawEllipse(x0, y0, rx, ry, opt);
    } else {
        int steps = (rx > ry ? rx : ry) * 2;
        if (steps < 16) steps = 16;
        if (steps > 64) steps = 64;
        for (int i = 0; i < steps; i++) {
            float ang = (float)i * (6.28318f / (float)steps);
            int px = x0 + (int)(cosf(ang) * (float)rx);
            int py = y0 + (int)(sinf(ang) * (float)ry);
            drawPixel(px, py);
        }
    }
}

void SafeDraw::drawDisc(int x0, int y0, int r, uint8_t opt) {
    if (r <= 0) return;
    if (x0 + r < 0 || x0 - r >= SCREEN_W || y0 + r < 0 || y0 - r >= SCREEN_H) return;

    if (x0 - r >= 0 && x0 + r < SCREEN_W && y0 - r >= 0 && y0 + r < SCREEN_H) {
        u8g2.drawDisc(x0, y0, r, opt);
    } else {
        // Safe horizontal scanlines for filled disc
        for (int dy = -r; dy <= r; dy++) {
            int py = y0 + dy;
            if (py < 0 || py >= SCREEN_H) continue;
            int dx = (int)sqrtf((float)(r * r - dy * dy));
            
            if (dy <= 0) { // Upper half
                if ((opt & U8G2_DRAW_UPPER_LEFT) && (opt & U8G2_DRAW_UPPER_RIGHT)) {
                    drawHLine(x0 - dx, py, 2 * dx + 1);
                } else if (opt & U8G2_DRAW_UPPER_LEFT) {
                    drawHLine(x0 - dx, py, dx + 1);
                } else if (opt & U8G2_DRAW_UPPER_RIGHT) {
                    drawHLine(x0, py, dx + 1);
                }
            } else { // Lower half
                if ((opt & U8G2_DRAW_LOWER_LEFT) && (opt & U8G2_DRAW_LOWER_RIGHT)) {
                    drawHLine(x0 - dx, py, 2 * dx + 1);
                } else if (opt & U8G2_DRAW_LOWER_LEFT) {
                    drawHLine(x0 - dx, py, dx + 1);
                } else if (opt & U8G2_DRAW_LOWER_RIGHT) {
                    drawHLine(x0, py, dx + 1);
                }
            }
        }
    }
}

void SafeDraw::drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2) {
    // Fast path: if all vertices are within screen bounds, use u8g2.drawTriangle directly
    if (x0 >= 0 && x0 < SCREEN_W && x1 >= 0 && x1 < SCREEN_W && x2 >= 0 && x2 < SCREEN_W &&
        y0 >= 0 && y0 < SCREEN_H && y1 >= 0 && y1 < SCREEN_H && y2 >= 0 && y2 < SCREEN_H) {
        u8g2.drawTriangle(x0, y0, x1, y1, x2, y2);
        return;
    }

    // Sort vertices by Y ascending (y0 <= y1 <= y2)
    if (y0 > y1) { int tx = x0; x0 = x1; x1 = tx; int ty = y0; y0 = y1; y1 = ty; }
    if (y1 > y2) { int tx = x1; x1 = x2; x2 = tx; int ty = y1; y1 = y2; y2 = ty; }
    if (y0 > y1) { int tx = x0; x0 = x1; x1 = tx; int ty = y0; y0 = y1; y1 = ty; }

    int total_height = y2 - y0;
    if (total_height == 0) {
        int min_x = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
        int max_x = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
        drawHLine(min_x, y0, max_x - min_x + 1);
        return;
    }

    // Top half (y0 to y1)
    int seg0_height = y1 - y0;
    if (seg0_height > 0) {
        for (int y = y0; y <= y1; y++) {
            int xa = x0 + (int)((int64_t)(x2 - x0) * (y - y0) / total_height);
            int xb = x0 + (int)((int64_t)(x1 - x0) * (y - y0) / seg0_height);
            if (xa > xb) { int t = xa; xa = xb; xb = t; }
            drawHLine(xa, y, xb - xa + 1);
        }
    }

    // Bottom half (y1 + 1 to y2)
    int seg1_height = y2 - y1;
    if (seg1_height > 0) {
        for (int y = y1 + 1; y <= y2; y++) {
            int xa = x0 + (int)((int64_t)(x2 - x0) * (y - y0) / total_height);
            int xb = x1 + (int)((int64_t)(x2 - x1) * (y - y1) / seg1_height);
            if (xa > xb) { int t = xa; xa = xb; xb = t; }
            drawHLine(xa, y, xb - xa + 1);
        }
    }
}

void SafeDraw::drawStr(int x, int y, const char *s) {
    if (!s || !s[0]) return;
    if (x > SCREEN_W || y < 0 || y > SCREEN_H + 30) return;
    u8g2.drawStr(x, y, s);
}

void SafeDraw::drawRFrame(int x, int y, int w, int h, int r) {
    if (w <= 0 || h <= 0) return;
    if (x >= 0 && x + w <= SCREEN_W && y >= 0 && y + h <= SCREEN_H) {
        u8g2.drawRFrame(x, y, w, h, r);
    } else {
        drawFrame(x, y, w, h);
    }
}

void SafeDraw::drawRBox(int x, int y, int w, int h, int r) {
    if (w <= 0 || h <= 0) return;
    if (x >= 0 && x + w <= SCREEN_W && y >= 0 && y + h <= SCREEN_H) {
        u8g2.drawRBox(x, y, w, h, r);
    } else {
        drawBox(x, y, w, h);
    }
}
