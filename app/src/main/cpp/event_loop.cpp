#include "event_loop.hpp"
#include "log.hpp"

using AndroidNative::EventLoop;

EventLoop::EventLoop(android_app* app):
        _application(app),
        _enabled(false),
        _quit(false)
{
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
            if (source != nullptr) {
                source->process (_application, source);
            }
            if (_application->destroyRequested) {
                Log::Info("Exiting event loop");
                return;
            }
        }
        if (_enabled && !_quit) {
            if (onStep != nullptr && onStep() != OK) {
                _quit = true;
                ANativeActivity_finish(_application->activity);
            }
        }
    }
}

void EventLoop::Activate()
{
    if (!_enabled && _application->window != nullptr) {
        _quit = false;
        _enabled = true;
        if (onActivate != nullptr && onActivate() != OK) {
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
        if (onDeactivate != nullptr) {
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
            if (onStart != nullptr) {
                onStart();
            }
            break;
        case APP_CMD_RESUME:
            if (onResume != nullptr) {
                onResume();
            }
            break;
        case APP_CMD_INIT_WINDOW:
            if (onInitWindow != nullptr) {
                onInitWindow(app);
            }
            break;
        case APP_CMD_GAINED_FOCUS:
            Activate();
            if (onGainFocus != nullptr) {
                onGainFocus();
            }
            break;
        case APP_CMD_PAUSE:
            if (onPause != nullptr) {
                onPause();
            }
            Deactivate();
            break;
        case APP_CMD_SAVE_STATE:
            if (onSaveInstanceState != nullptr) {
                onSaveInstanceState(&_application->savedState, &_application->savedStateSize);
            }
            break;
        case APP_CMD_STOP:
            if (onStop != nullptr) {
                onStop();
            }
            break;
        case APP_CMD_LOST_FOCUS:
            if (onLostFocus != nullptr) {
                onLostFocus();
            }
            Deactivate();
            break;
        case APP_CMD_TERM_WINDOW:
            if (onTermWindow != nullptr) {
                onTermWindow();
            }
            Deactivate();
            break;
        case APP_CMD_DESTROY:
            if (onDestroy != nullptr) {
                onDestroy();
            }
            break;
        case APP_CMD_CONFIG_CHANGED:
            if (onConfigurationChanged != nullptr) {
                onConfigurationChanged(app);
            }
            break;
        case APP_CMD_LOW_MEMORY:
            if (onLowMemory != nullptr) {
                onLowMemory();
            }
            break;
        default:
            break;
    }
}