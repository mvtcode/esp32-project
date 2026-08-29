#ifndef FACE_DETECTOR_H
#define FACE_DETECTOR_H

#include <Arduino.h>
#include <vector>
#include <list>
#include "camera_service.h"
#include "human_face_detect_msr01.hpp"

// Cấu trúc mô tả một khuôn mặt được phát hiện
struct FaceBox {
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
    float score = 0.0f;
    std::vector<int> keypoints;

    int width() const { return max(0, x2 - x1); }
    int height() const { return max(0, y2 - y1); }
};

// Cấu trúc kết quả phát hiện khuôn mặt (Stream Output của khối AI Detector)
struct FaceDetectionResult {
    std::vector<FaceBox> faces;
    float detect_fps = 0.0f;
    uint32_t timestamp = 0;
};

class FaceDetectorService {
private:
    static HumanFaceDetectMSR01 *detector;
    static SemaphoreHandle_t xMutex;
    static uint16_t *detect_buffer;
    static volatile bool new_frame_ready;
    static bool is_running;
    static FaceDetectionResult current_result;
    static uint32_t detect_count;
    static uint32_t last_detect_time;

    static void taskWorker(void *param);

public:
    static bool init();
    static void feedFrame(const CameraFrame &frame);
    static FaceDetectionResult getLatestResult();
};

#endif // FACE_DETECTOR_H
