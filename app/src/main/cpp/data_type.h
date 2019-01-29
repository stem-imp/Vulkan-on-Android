#ifndef DATA_TYPE_H
#define DATA_TYPE_H

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

using glm::vec3;
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

typedef struct BlinnPhongLighting {
    vec3 cameraPosInWorldSpace;
    alignas(16) vec3 lightPosInWorldSpace;
    alignas(16) vec3 ambientLight;
    alignas(16) vec3 diffuseLight;
    alignas(16) vec3 specularLight;
    alignas(16) float shininess;
} BlinnPhongLighting;

#endif // DATA_TYPE_H
