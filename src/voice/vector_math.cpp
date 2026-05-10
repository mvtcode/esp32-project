/**
 * vector_math.cpp — Vector Similarity Utilities for ESP32-S3
 * IoT Voice Command System
 */
#include "vector_math.h"
#include <math.h>
#include <esp_dsp.h>

float vector_dot_product(const float* v1, const float* v2, int dim) {
    float dot = 0.0f;
    dsps_dotprod_f32(v1, v2, &dot, dim);
    return dot;
}

float vector_magnitude(const float* v, int dim) {
    float sum_sq = 0.0f;
    dsps_dotprod_f32(v, v, &sum_sq, dim);
    return sqrtf(sum_sq);
}

float vector_cosine_similarity(const float* v1, const float* v2, int dim) {
    float dot = vector_dot_product(v1, v2, dim);
    float mag1 = vector_magnitude(v1, dim);
    float mag2 = vector_magnitude(v2, dim);
    
    if (mag1 < 0.0001f || mag2 < 0.0001f) return 0.0f;
    return dot / (mag1 * mag2);
}
