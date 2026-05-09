/**
 * vector_math.cpp — Vector Similarity Utilities for ESP32-S3
 * IoT Voice Command System
 */
#include "vector_math.h"
#include <math.h>

// If ESP-DSP is available, we would include it here:
// #include "dsps_dotprod.h"

float vector_dot_product(const float* v1, const float* v2, int dim) {
    float dot = 0.0f;
    // Standard loop — ESP32-S3 compiler with -O2 will try to use PIE instructions
    // but dsps_dotprod_f32 would be even faster.
    for (int i = 0; i < dim; i++) {
        dot += v1[i] * v2[i];
    }
    return dot;
}

float vector_magnitude(const float* v, int dim) {
    float sum_sq = 0.0f;
    for (int i = 0; i < dim; i++) {
        sum_sq += v[i] * v[i];
    }
    return sqrtf(sum_sq);
}

float vector_cosine_similarity(const float* v1, const float* v2, int dim) {
    float dot = vector_dot_product(v1, v2, dim);
    float mag1 = vector_magnitude(v1, dim);
    float mag2 = vector_magnitude(v2, dim);
    
    if (mag1 < 0.00001f || mag2 < 0.00001f) return 0.0f;
    
    return dot / (mag1 * mag2);
}
