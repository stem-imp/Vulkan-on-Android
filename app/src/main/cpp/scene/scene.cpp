#include "scene.h"

using std::chrono::duration;
using std::chrono::seconds;

Scene::Scene(void* state) : eventLoop((android_app*)state)
{
    startTime = intervalBaseTime = currentTime = high_resolution_clock::now();
}

bool Scene::Update()
{
    static int frames = 0;
    frames++;

    auto now = high_resolution_clock::now();
    float elapsedTime = duration<float, seconds::period>(now - intervalBaseTime).count();
    if (elapsedTime >= 2.0f) {
        Log::Info("%d", frames / 2);
        frames = 0;
        intervalBaseTime = now;
    }

    deltaTime = duration<float, seconds::period>(now - currentTime).count();
    currentTime = now;

    bool result = UpdateImpl();
    renderer->Render();
    return result;
}