#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include "../display.h"

// Forward extern declaration of the display instance
extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

// -----------------------------------------------------------------------
// SafeDraw: Bounded Graphics Layer for U8g2
// 
// Prevents out-of-bounds drawing bugs (horizontal/vertical screen streaks)
// caused by signed coordinate underflow/overflow when passed to U8g2's
// uint8_t arguments. Uses Cohen-Sutherland line clipping and geometric
// bounding box clipping.
// -----------------------------------------------------------------------

class SafeDraw {
public:
    // --- Clipped 2D Primitives (accepts arbitrary signed coordinates) ---
    
    /** Draw a single pixel if within [0, SCREEN_W-1] and [0, SCREEN_H-1]. */
    static void drawPixel(int x, int y);

    /**
     * Draw a line using Cohen-Sutherland line clipping.
     * Endpoints outside the viewport are clipped to screen edges before rendering.
     */
    static void drawLine(int x0, int y0, int x1, int y1);

    /** Draw a horizontal line clipped to [0, SCREEN_W]. Handles w < 0 safely. */
    static void drawHLine(int x, int y, int w);

    /** Draw a vertical line clipped to [0, SCREEN_H]. Handles h < 0 safely. */
    static void drawVLine(int x, int y, int h);

    /** Draw a solid rectangle clipped to screen bounds. */
    static void drawBox(int x, int y, int w, int h);

    /** Draw a wireframe rectangle clipped to screen bounds. */
    static void drawFrame(int x, int y, int w, int h);

    /** Draw a circle if within or intersecting screen bounds. */
    static void drawCircle(int x0, int y0, int r, uint8_t opt = U8G2_DRAW_ALL);

    /** Draw a filled disc if within or intersecting screen bounds. */
    static void drawDisc(int x0, int y0, int r, uint8_t opt = U8G2_DRAW_ALL);

    /** Draw a filled triangle clipped to screen bounds. */
    static void drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2);

    /** Draw a string with boundary check. */
    static void drawStr(int x, int y, const char *s);

    /** Draw a rounded wireframe rectangle. */
    static void drawRFrame(int x, int y, int w, int h, int r);

    /** Draw a rounded filled rectangle. */
    static void drawRBox(int x, int y, int w, int h, int r);

    // --- U8g2 Passthrough Methods ---
    static inline void setFont(const uint8_t *font) { u8g2.setFont(font); }
    static inline void setDrawColor(uint8_t color) { u8g2.setDrawColor(color); }
    static inline int  getStrWidth(const char *s) { return u8g2.getStrWidth(s); }
    static inline int  getMaxCharHeight() { return u8g2.getMaxCharHeight(); }
    static inline void clearBuffer() { u8g2.clearBuffer(); }
    static inline void sendBuffer() { u8g2.sendBuffer(); }
};

// Global instance alias for convenient syntax: gfx.drawLine(...), gfx.drawPixel(...)
extern SafeDraw gfx;
