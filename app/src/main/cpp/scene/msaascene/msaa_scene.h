#ifndef MSAA_SCENE_H
#define MSAA_SCENE_H

#include "../scene.h"

class MSAAScene : public Scene
{
public:
    MSAAScene(void* state);
    ~MSAAScene();

    bool UpdateImpl() override;
};

#endif