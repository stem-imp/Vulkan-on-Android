#ifndef DATA_TYPE_H
#define DATA_TYPE_H

#include <glm/mat4x4.hpp>

using glm::mat4;

typedef struct Extent2D {
    uint32_t width;
    uint32_t height;
} Extent2D;

typedef struct ModelTransform {
    mat4 modelTransform;
    mat4 modelTransformInverse;
} ModelTransform;

typedef struct ViewProjectionTransform {
    mat4 view;
    mat4 projection;
} ViewProjectionTransform;

#endif // DATA_TYPE_H
