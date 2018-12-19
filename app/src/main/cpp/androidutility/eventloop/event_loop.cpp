#include "event_loop.h"
#include "../../log/log.h"
#include <string>

using AndroidNative::EventLoop;
using Utility::Log;

using std::to_string;

EventLoop::EventLoop(android_app* app) : _application(app), _enabled(false), _quit(false)
{
    assert("Null android_app* parameter." && _application);
    _application->userData = this;
    _application->onAppCmd = AppEvent;
}

void EventLoop::Run()
{
    int32_t  events;
    android_poll_source* source;

    // Calls to app_dummy are no longer necessary.

    Log::Info("Starting event loop");
    while (true) {
        while (ALooper_pollAll(_enabled ? 0 : -1, nullptr, &events, (void**)&source) >= 0) {
            if (source) {
                source->process (_application, source);
            }
            if (_application->destroyRequested) {
                Log::Info("Exiting event loop");
                return;
            }
        }
        if (_enabled && !_quit) {
            if (onStep && !onStep()) {
                _quit = true;
                ANativeActivity_finish(_application->activity);
            }
        }
    }
}

void EventLoop::Activate()
{
    if (!_enabled && _application->window) {
        _quit = false;
        _enabled = true;
        if (onActivate && !onActivate()) {
            goto ERROR;
        }
    }
    return;

    ERROR:
    _quit = true;
    Deactivate();
    ANativeActivity_finish(_application->activity);
}

void EventLoop::Deactivate()
{
    if (_enabled) {
        if (onDeactivate) {
            onDeactivate();
        }
        _enabled = false;
    }
}

void EventLoop::AppEvent(android_app* app, int32_t command)
{
    EventLoop& eventLoop = *(EventLoop*)app->userData;
    eventLoop.ProcessAppEvent(app, command);
}

void EventLoop::ProcessAppEvent(android_app* app, int32_t command)
{
    switch (command) {
        case APP_CMD_START:
            if (onStart) {
                onStart();
            }
            break;
        case APP_CMD_RESUME:
            if (onResume) {
                onResume();
            }
            break;
        case APP_CMD_INIT_WINDOW:
            if (onInitWindow) {
                onInitWindow(app);
            }
            break;
        case APP_CMD_GAINED_FOCUS:
            Activate();
            if (onGainFocus) {
                onGainFocus();
            }
            break;
        case APP_CMD_PAUSE:
            if (onPause) {
                onPause();
            }
            Deactivate();
            break;
        case APP_CMD_SAVE_STATE:
            if (onSaveInstanceState) {
                onSaveInstanceState(&_application->savedState, &_application->savedStateSize);
            }
            break;
        case APP_CMD_STOP:
            if (onStop) {
                onStop();
            }
            break;
        case APP_CMD_LOST_FOCUS:
            if (onLostFocus) {
                onLostFocus();
            }
            Deactivate();
            break;
        case APP_CMD_TERM_WINDOW:
            if (onTermWindow) {
                onTermWindow();
            }
            Deactivate();
            break;
        case APP_CMD_DESTROY:
            if (onDestroy) {
                onDestroy();
            }
            break;
        case APP_CMD_CONFIG_CHANGED:
            if (onConfigurationChanged) {
                onConfigurationChanged();
            }
            break;
        case APP_CMD_LOW_MEMORY:
            if (onLowMemory) {
                onLowMemory();
            }
            break;
        default:
            break;
    }
}