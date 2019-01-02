#ifndef EARTH_SCENE_H
#define EARTH_SCENE_H

#include "../scene.h"
#include "../../vulkan/model/model_resource.h"
#include <vector>

class EarthScene : public Scene
{
public:
    EarthScene(void* state);
    ~EarthScene();

    bool UpdateImpl() override;
};


#endif // EARTH_SCENE_H