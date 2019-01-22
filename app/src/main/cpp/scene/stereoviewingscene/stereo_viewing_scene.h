#ifndef STEREO_VIEWING_SCENE_H
#define STEREO_VIEWING_SCENE_H

#include "../scene.h"
#include "../../vulkan/model/model.h"
#include "../../vulkan/texture/texture.h"

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

    const vector<Model>& Models() const { return _models; };
private:
    uint32_t _screenWidth, _screenHeight;

    vec3 _worldSpaceCameraPos;
    float _zNear, _zFar;
    float _fov;
    float _focalLength;
    float _eyeSeparation;

    // ==== Vulkan ==== //
    vector<Model>           _models;
    vector<mat4>  _modelTransforms;
    ViewProjectionTransform _lViewProjTransform;
    ViewProjectionTransform _rViewProjTransform;
};

#endif // STEREO_VIEWING_SCENE_H