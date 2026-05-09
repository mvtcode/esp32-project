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
    // Note: This implementation now ASSUMES that v1 and v2 are ALREADY NORMALIZED
    // (i.e. vector_magnitude = 1.0f).
    // The Cosine Similarity of two normalized vectors is exactly their dot product.
    // This optimization saves 66% of processing time during real-time inference.
    return vector_dot_product(v1, v2, dim);
}
