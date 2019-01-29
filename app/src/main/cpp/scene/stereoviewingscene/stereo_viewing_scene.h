#ifndef STEREO_VIEWING_SCENE_H
#define STEREO_VIEWING_SCENE_H

#include "../scene.h"
#include "../../vulkan/model/model.h"
#include "../../vulkan/texture/texture.h"
#include "transformation.hpp"
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/quaternion_float.hpp>

using Vulkan::Model;

class StereoViewingScene : public Scene
{
public:
    StereoViewingScene(void* state);
    ~StereoViewingScene();

    bool UpdateImpl() override;

//    typedef struct ViewProjectionTransform {
//        mat4 view;
//        mat4 projection;
//    } ViewProjectionTransform;

//    typedef struct ModelTransform {
//        mat4 modelTransform;
//        mat4 modelTransformInverse;
//    };

    void UpdateDeviceOrientation(const float rotationMatrix[], bool columnMajorInput);

    void OnGetRotationVector(float rotationVector[])
    {
        glm::quat q = glm::quat(rotationVector[3], rotationVector[0], rotationVector[1], rotationVector[2]);
        glm::quat n = glm::normalize(q);
        glm::quat base(0.7071068f, 0.0f, -0.7071068f, 0.0f);
        n = n * glm::inverse(base);
        rotationVector[0] = -n.y;
        rotationVector[1] = n.z;
        rotationVector[2] = -n.x;
        rotationVector[3] = n.w;
        GetRotationMatrixFromVector(rotationMatrix, 16, rotationVector, 4);
        UpdateDeviceOrientation(rotationMatrix, false);
    }

    const vector<Model>& Models() const { return _models; };
private:
    uint32_t _screenWidth, _screenHeight;

    vec3 _worldSpaceCameraPos;
    float _zNear, _zFar;
    float _fov;
    float _focalLength;
    float _eyeSeparation;
    float rotationMatrix[16];
    vector<float> cameraRotationMatrix;

    // ==== Vulkan ==== //
    vector<Model> _models;
    vector<mat4>  _modelTransforms;
    ViewProjectionTransform _lViewProjTransform;
    ViewProjectionTransform _rViewProjTransform;
};

#endif // STEREO_VIEWING_SCENE_H