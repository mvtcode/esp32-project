#include "face_detector.h"
#include "log.h"

HumanFaceDetectMSR01 *FaceDetectorService::detector = nullptr;
SemaphoreHandle_t FaceDetectorService::xMutex = NULL;
uint16_t *FaceDetectorService::detect_buffer = nullptr;
volatile bool FaceDetectorService::new_frame_ready = false;
bool FaceDetectorService::is_running = false;
FaceDetectionResult FaceDetectorService::current_result;
uint32_t FaceDetectorService::detect_count = 0;
uint32_t FaceDetectorService::last_detect_time = 0;

void FaceDetectorService::taskWorker(void *param) {
    LOG_I("FaceDetectorService", "Lightweight Face Detection Worker started on Core 0");
    
    // Khởi tạo Mô hình Phát hiện khuôn mặt MSR01 siêu nhẹ
    detector = new HumanFaceDetectMSR01(0.3F, 0.3F, 5, 0.25F);
    std::vector<int> input_shape = {240, 320, 3};

    while (is_running) {
        if (new_frame_ready && detect_buffer != nullptr) {
            std::list<dl::detect::result_t> &results = detector->infer(detect_buffer, input_shape);

            if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                current_result.faces.clear();

                for (auto &res : results) {
                    FaceBox box;
                    box.x1 = constrain(res.box[0], 0, 319);
                    box.y1 = constrain(res.box[1], 0, 239);
                    box.x2 = constrain(res.box[2], 0, 319);
                    box.y2 = constrain(res.box[3], 0, 239);
                    box.score = res.score;
                    box.keypoints = res.keypoint;
                    current_result.faces.push_back(box);
                }
                current_result.timestamp = millis();
                xSemaphoreGive(xMutex);
            }

            new_frame_ready = false;

            detect_count++;
            uint32_t now = millis();
            if (now - last_detect_time >= 1000) {
                current_result.detect_fps = (detect_count * 1000.0f) / (now - last_detect_time);
                detect_count = 0;
                last_detect_time = now;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}

bool FaceDetectorService::init() {
    // Cấp phát bộ đệm phân tích hình ảnh trong PSRAM 8MB
    detect_buffer = (uint16_t *)ps_malloc(320 * 240 * sizeof(uint16_t));
    if (!detect_buffer) {
        detect_buffer = (uint16_t *)malloc(320 * 240 * sizeof(uint16_t));
    }
    if (!detect_buffer) return false;

    xMutex = xSemaphoreCreateMutex();
    is_running = true;

    xTaskCreatePinnedToCore(
        taskWorker,
        "FaceDetectorWorker",
        8 * 1024, // Fix #4: 6KB không đủ cho HumanFaceDetectMSR01::infer(), khôi phục 8KB
        NULL,
        1,
        NULL,
        0 // Pin to Core 0
    );

    return true;
}

void FaceDetectorService::feedFrame(const CameraFrame &frame) {
    if (!new_frame_ready && detect_buffer != nullptr && frame.isValid()) {
        if (frame.width == 640 && frame.height == 480) {
            const uint16_t *src = (const uint16_t *)frame.buffer;
            for (int y = 0; y < 240; ++y) {
                const uint16_t *src_row = &src[y * 2 * 640];
                uint16_t *dst_row = &detect_buffer[y * 320];
                for (int x = 0; x < 320; ++x) {
                    dst_row[x] = src_row[x * 2];
                }
            }
            new_frame_ready = true;
        } else if (frame.width == 320 && frame.height == 240) {
            memcpy(detect_buffer, frame.buffer, 320 * 240 * sizeof(uint16_t));
            new_frame_ready = true;
        }
    }
}

FaceDetectionResult FaceDetectorService::getLatestResult() {
    FaceDetectionResult res;
    if (xMutex != NULL && xSemaphoreTake(xMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        res = current_result;
        xSemaphoreGive(xMutex);
    }
    return res;
}
