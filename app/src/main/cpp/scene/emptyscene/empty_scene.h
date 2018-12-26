#ifndef APP_EMPTY_SCENE_H
#define APP_EMPTY_SCENE_H

#include "../scene.h"
#include "android/empty_scene_renderer_vulkan_android.h"

class EmptyScene : public Scene
{
public:
    EmptyScene(void* state);
    ~EmptyScene();

    bool UpdateImpl() override;
};

#endif // APP_EMPTY_SCENE_H
