#ifndef DEV_HUD_H
#define DEV_HUD_H

#include <lvgl.h>
#include <Arduino.h>

class DevHud {
public:
    DevHud();
    ~DevHud();

    void setVisible(bool visible);
    bool isVisible() const { return visible; }
    void updateStats(float fps, uint32_t freeHeapKb, uint8_t cpuPercent, int32_t rssi, const char* ip);

private:
    lv_obj_t* hudContainer;
    lv_obj_t* lblFps;
    lv_obj_t* lblRam;
    lv_obj_t* lblCpu;
    lv_obj_t* lblWifi;
    bool visible;
};

#endif // DEV_HUD_H
