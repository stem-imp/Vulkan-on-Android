#include "event_loop.hpp"
#include "log.hpp"
#include <memory>

using std::unique_ptr;

static status OnActivate();
static void OnDeactivate();
static status OnStep();

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
static void OnConfigurationChanged(void);
static void OnLowMemory(void);

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state)
{
    Log::TAG = "vulkan";
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

static status OnActivate()
{
    Log::Info("App OnActivate");
    return OK;
}

static void OnDeactivate()
{
    Log::Info("App OnDeactivate");
}

status OnStep()
{
    Log::Info("App OnStep");
    return OK;
}

void OnStart(void)
{
    Log::Info("App OnStart");
}

void OnResume(void)
{
    Log::Info("App OnResume");
}

void OnPause(void)
{
    Log::Info("App OnPause");
}

void OnStop(void)
{
    Log::Info("App OnStop");
}

void OnDestroy(void)
{
    Log::Info("App OnDestroy");
}

void OnInitWindow(android_app* app)
{
    Log::Info("App OnInitWindow");
}

void OnTermWindow(void)
{
    Log::Info("App OnTermWindow");
}

void OnGainFocus(void)
{
    Log::Info("App OnGainFocus");
}

void OnLostFocus(void)
{
    Log::Info("App OnLostFocus");
}

void OnSaveInstanceState(void**, size_t*)
{
    Log::Info("App OnSaveInstanceState");
}

void OnConfigurationChanged(void)
{
    Log::Info("App OnConfigurationChanged");
}

void OnLowMemory(void)
{
    Log::Info("App OnLowMemory");
}