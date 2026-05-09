/**
 * vector_math.h — Vector Similarity Utilities for ESP32-S3
 * IoT Voice Command System
 */
#ifndef VECTOR_MATH_H
#define VECTOR_MATH_H

#include <stdint.h>

#define VECTOR_DIM 1024

/**
 * Calculate Cosine Similarity between two vectors.
 * Uses ESP-DSP optimizations on ESP32-S3 if available.
 */
float vector_cosine_similarity(const float* v1, const float* v2, int dim);

/**
 * Helper to calculate dot product (numerator of cosine similarity)
 */
float vector_dot_product(const float* v1, const float* v2, int dim);

/**
 * Helper to calculate magnitude (used for normalization)
 */
float vector_magnitude(const float* v, int dim);

#endif // VECTOR_MATH_H
