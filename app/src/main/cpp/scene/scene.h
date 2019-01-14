#ifndef APP_SCENE_H
#define APP_SCENE_H

#ifdef __ANDROID__
#include <android_native_app_glue.h>
#include "../androidutility/eventloop/event_loop.h"

using AndroidNative::EventLoop;
#endif

#include "../renderer.h"
#include <chrono>

using std::chrono::high_resolution_clock;

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

    high_resolution_clock::time_point startTime;
    high_resolution_clock::time_point intervalBaseTime;
    high_resolution_clock::time_point currentTime;
    float                             deltaTime;
};

#endif // APP_SCENE_H
