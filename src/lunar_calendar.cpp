#include "lunar_calendar.h"
#include <math.h>
#include <stdio.h>

// ========== ÂM LỊCH VIỆT NAM - ESP32 OPTIMIZED ==========
#define LUNAR_PI 3.14159265f
#define DR (LUNAR_PI / 180.0f)
#define LUNAR_TZ 7.0f

/* ---------- Julian Day ---------- */
static int jdFromDate(int d, int m, int y) {
    int a = (14 - m) / 12;
    y += 4800 - a;
    m += 12 * a - 3;

    int jd = d + (153 * m + 2) / 5 + 365 * y
           + y / 4 - y / 100 + y / 400 - 32045;

    if (jd < 2299161)
        jd = d + (153 * m + 2) / 5 + 365 * y + y / 4 - 32083;

    return jd;
}

/* ---------- New Moon ---------- */
static int getNewMoonDay(int k) {
    float T  = (float)k / 1236.85f;
    float T2 = T * T;
    float T3 = T2 * T;

    float jd = 2415020.75933f + 29.53058868f * (float)k + 0.0001178f * T2 - 0.000000155f * T3;
    jd += 0.00033f * sinf((166.56f + 132.87f * T - 0.009173f * T2) * DR);

    float M  = 359.2242f + 29.10535608f * (float)k;
    float Mp = 306.0253f + 385.81691806f * (float)k;
    float F  = 21.2964f  + 390.67050646f * (float)k;

    float C = (0.1734f - 0.000393f * T) * sinf(M * DR) - 0.4068f * sinf(Mp * DR) + 0.0161f * sinf(2 * Mp * DR) + 0.0104f * sinf(2 * F * DR);
    float deltaT = (T < -11.0f) ? (0.001f + 0.000839f * T) : (-0.000278f + 0.000265f * T);

    return (int)(jd + C - deltaT + 0.5f + LUNAR_TZ / 24.0f);
}

/* ---------- Sun Longitude ---------- */
static int getSunLongitude(int jdn) {
    float T = (jdn - 2451545.5f - LUNAR_TZ / 24.0f) / 36525.0f;
    float M = 357.52910f + 35999.05030f * T;
    float L = 280.46645f + 36000.76983f * T;

    float DL =
        (1.9146f - 0.004817f * T) * sinf(M * DR)
        + 0.019993f * sinf(2 * M * DR);

    L = (L + DL) * DR;
    L -= 2 * LUNAR_PI * floorf(L / (2 * LUNAR_PI));

    return (int)(L / LUNAR_PI * 6); // 0..11
}

/* ---------- Lunar Month 11 ---------- */
static int getLunarMonth11(int y) {
    int off = jdFromDate(31, 12, y) - 2415021;
    int k = off / 29.530588f;
    int nm = getNewMoonDay(k);
    if (getSunLongitude(nm) >= 9)
        nm = getNewMoonDay(k - 1);
    return nm;
}

/* ---------- Leap Month ---------- */
static int getLeapMonthOffset(int a11) {
    int k = (a11 - 2415021) / 29.530588f;
    int last = -1;
    for (int i = 1; i < 14; i++) {
        int arc = getSunLongitude(getNewMoonDay(k + i));
        if (arc == last) return i - 1;
        last = arc;
    }
    return 0;
}

/* ---------- Solar to Lunar Conversion ---------- */
void solarToLunar(int solarYear, int solarMonth, int solarDay, 
                  int &lunarDay, int &lunarMonth, int &lunarYear) {
    int jd = jdFromDate(solarDay, solarMonth, solarYear);
    int k = (jd - 2415021) / 29.530588f;
    int nm = getNewMoonDay(k);
    if (nm > jd) nm = getNewMoonDay(k - 1);

    int a11 = getLunarMonth11(solarYear);
    int b11 = a11;
    int ly;

    if (a11 >= nm) {
        ly = solarYear;
        a11 = getLunarMonth11(solarYear - 1);
    } else {
        ly = solarYear + 1;
        b11 = getLunarMonth11(solarYear + 1);
    }

    int diff = (nm - a11) / 29;
    int lm = diff + 11;

    if (b11 - a11 > 365) {
        int leapDiff = getLeapMonthOffset(a11);
        if (diff >= leapDiff) {
            lm--;
        }
    }

    if (lm > 12) lm -= 12;
    if (lm >= 11 && diff < 4) ly--;

    lunarDay = jd - nm + 1;
    lunarMonth = lm;
    lunarYear = ly;
}

static const char* CAN_NAMES[] = {
    "Giap", "At", "Binh", "Dinh", "Mau", "Ky", "Canh", "Tan", "Nham", "Quy"
};

static const char* CHI_NAMES[] = {
    "Ty", "Suu", "Dan", "Mao", "Thin", "Ty", "Ngo", "Mui", "Than", "Dau", "Tuat", "Hoi"
};

const char* getLunarYearName(int lunarYear) {
    static char buf[32];
    int can_idx = (lunarYear + 6) % 10;
    int chi_idx = (lunarYear + 8) % 12;
    if (can_idx < 0) can_idx += 10;
    if (chi_idx < 0) chi_idx += 12;
    snprintf(buf, sizeof(buf), "%s %s", CAN_NAMES[can_idx], CHI_NAMES[chi_idx]);
    return buf;
}
