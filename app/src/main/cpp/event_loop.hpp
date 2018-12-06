#ifndef ANDROID_EVENT_LOOP_HPP
#define ANDROID_EVENT_LOOP_HPP

#include "types.hpp"
#include <android_native_app_glue.h>
#include <functional>
#include <android/sensor.h>

using std::function;

namespace AndroidNative {
    class EventLoop {
    public:
        EventLoop(android_app *app);

        void Run();

        function<status(void)> onActivate;
        function<void(void)> onDeactivate;
        function<status(android_app *)> onStep;

        function<void(void)> onStart;
        function<void(void)> onResume;
        function<void(android_app *)> onInitWindow;
        function<void(void)> onGainFocus;

        function<void(void)> onPause;
        function<void(void)> onLostFocus;
        function<void(void)> onTermWindow;
        function<void(void **, size_t *)> onSaveInstanceState;
        function<void(void)> onStop;
        function<void(void)> onDestroy;

        function<void(android_app *)> onConfigurationChanged;
        function<void(void)> onLowMemory;

        function<void(float rotationVector[])> onGetRotationVector;

        ASensorEventQueue* rotationVectorEventQueue;
    private:
        void Activate();
        void Deactivate();
        void ProcessAppEvent(android_app *app, int32_t command);
        static void AppEvent(android_app *app, int32_t command);

        android_app *_application;
        bool _enabled;
        bool _quit;

        ASensorManager* _sensorManager;
        const ASensor* _rotationVector;
    };
}

#endif // ANDROID_EVENT_LOOP_HPP
