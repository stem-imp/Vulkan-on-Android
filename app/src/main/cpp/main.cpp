#include "androidutility/eventloop/event_loop.h"
#include "log/log.h"
#include <memory>

using std::unique_ptr;
using AndroidNative::EventLoop;
using Utility::Log;

static bool OnActivate();
static void OnDeactivate();
static bool OnStep(android_app* app);

static void OnStart(void);
static void OnResume(void);
static void OnPause(void);
static void OnStop(void);
static void OnDestroy(void);

static void OnInitWindow(android_app* app);
static void OnTermWindow(void);
static void OnGainFocus(void);
static void OnLostFocus(void);

static void OnSaveInstanceState(void**, size_t*);
static void OnConfigurationChanged(android_app* app);
static void OnLowMemory(void);

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state)
{
    Log::Tag = "vulkan";
    unique_ptr<EventLoop> loop(new EventLoop(state));
    loop->onActivate = OnActivate;
    loop->onDeactivate = OnDeactivate;
    loop->onStep = OnStep;

    loop->onStart = OnStart;
    loop->onResume = OnResume;
    loop->onPause = OnPause;
    loop->onStop = OnStop;
    loop->onDestroy = OnDestroy;

    loop->onInitWindow = OnInitWindow;
    loop->onTermWindow = OnTermWindow;
    loop->onGainFocus = OnGainFocus;
    loop->onLostFocus = OnLostFocus;

    loop->onSaveInstanceState = OnSaveInstanceState;
    loop->onConfigurationChanged = OnConfigurationChanged;
    loop->onLowMemory = OnLowMemory;

    loop->Run();
}

static bool OnActivate()
{
    DebugLog("App OnActivate");
    return true;
}

static void OnDeactivate()
{
    DebugLog("App OnDeactivate");
}

bool OnStep(android_app*)
{
    DebugLog("App OnStep");
    return true;
}

void OnStart(void)
{
    DebugLog("App OnStart");
}

void OnResume(void)
{
    DebugLog("App OnResume");
}

void OnPause(void)
{
    DebugLog("App OnPause");
}

void OnStop(void)
{
    DebugLog("App OnStop");
}

void OnDestroy(void)
{
    DebugLog("App OnDestroy");
}

void OnInitWindow(android_app* app)
{
    DebugLog("App OnInitWindow");
}

void OnTermWindow(void)
{
    DebugLog("App OnTermWindow");
}

void OnGainFocus(void)
{
    DebugLog("App OnGainFocus");
}

void OnLostFocus(void)
{
    DebugLog("App OnLostFocus");
}

void OnSaveInstanceState(void**, size_t*)
{
    DebugLog("App OnSaveInstanceState");
}

void OnConfigurationChanged(android_app* app)
{
    DebugLog("App OnConfigurationChanged");
}

void OnLowMemory(void)
{
    DebugLog("App OnLowMemory");
}