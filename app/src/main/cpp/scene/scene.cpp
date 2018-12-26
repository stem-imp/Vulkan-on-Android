#include "scene.h"

static timespec TimeSpecDiff(timespec start, timespec end);

Scene::Scene(void *state) : eventLoop((android_app*)state)
{
#ifdef __ANDROID__
    clock_gettime(CLOCK_MONOTONIC, &startTime);
#endif
}

bool Scene::Update()
{
    static int frames = 0;
    frames++;
    timespec endTime;
#ifdef __ANDROID__
    clock_gettime(CLOCK_MONOTONIC, &endTime);
#endif
    timespec td = TimeSpecDiff(startTime, endTime);
    if (td.tv_sec >= 2) {
        Log::Info("%d", frames / 2);
        frames = 0;
        startTime = endTime;
    }

    bool result = UpdateImpl();
    renderer->Render();
    return result;
}

timespec TimeSpecDiff(timespec start, timespec end)
{
    timespec temp;
    if (end.tv_nsec - start.tv_nsec < 0) {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000ul + end.tv_nsec - start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}