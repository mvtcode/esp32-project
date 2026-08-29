#ifndef UPLOAD_TYPES_H
#define UPLOAD_TYPES_H

// Trạng thái Upload – tách ra file riêng để tránh circular dependency
// giữa google_drive_service.h, display_service.h và led_service.h
enum UploadStatus {
    UPLOAD_IDLE,
    UPLOAD_IN_PROGRESS,
    UPLOAD_SUCCESS,
    UPLOAD_FAILED
};

#endif // UPLOAD_TYPES_H
