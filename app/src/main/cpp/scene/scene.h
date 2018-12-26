#ifndef APP_SCENE_H
#define APP_SCENE_H

#ifdef __ANDROID__
#include <android_native_app_glue.h>
#include "../androidutility/eventloop/event_loop.h"

using AndroidNative::EventLoop;
#endif

#include "../renderer.h"

class Scene
{
public:
    bool Update();

protected:
    Scene(void* state);

    virtual bool UpdateImpl() = 0;

#ifdef __ANDROID__
    EventLoop eventLoop;
#endif
    Renderer* renderer;

    timespec startTime;
};

#endif // APP_SCENE_H
