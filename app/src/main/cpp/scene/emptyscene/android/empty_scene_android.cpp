#ifdef __ANDROID__

#include "../empty_scene.h"
#include "../empty_scene_renderer_vulkan.h"
#include <android_native_app_glue.h>
#include "../../../log/log.h"
//#include <array>

using Utility::Log;
//using std::array;

static bool OnActivate();
static void OnDeactivate();

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
static void OnLowMemory(void);

//static void OnGetRotationVector(array<float, 4> rotationVector);

EmptyScene::EmptyScene(void* state) : Scene(state)
{
    eventLoop.onActivate = OnActivate;
    eventLoop.onDeactivate = OnDeactivate;
    eventLoop.onStep = [this]() -> bool {
        return Update();
    };

    eventLoop.onStart = OnStart;
    eventLoop.onResume = OnResume;
    eventLoop.onPause = OnPause;
    eventLoop.onStop = OnStop;
    eventLoop.onDestroy = OnDestroy;

    eventLoop.onInitWindow = [this](android_app* app) -> void {
        int32_t w = ANativeWindow_getWidth(app->window);
        int32_t h = ANativeWindow_getHeight(app->window);
        renderer = new EmptySceneRenderer(app->window, w, h);
        OnInitWindow(app);
    };
    eventLoop.onTermWindow = OnTermWindow;
    eventLoop.onGainFocus = OnGainFocus;
    eventLoop.onLostFocus = OnLostFocus;

    eventLoop.onSaveInstanceState = OnSaveInstanceState;
    eventLoop.onConfigurationChanged = [this](android_app* app) -> void {
        DebugLog("App OnConfigurationChanged");
        int32_t width = ANativeWindow_getWidth(app->window);
        int32_t height = ANativeWindow_getHeight(app->window);
        DebugLog("Old (width, height) = (%d, %d)", width, height);
        renderer->SetScreenSize(height, width);
        renderer->ChangeOrientation();
    };
    eventLoop.onLowMemory = OnLowMemory;

//    shared_ptr<SensorManager> sensorManager = std::make_shared<SensorManager>(state);
//    sensorManager->onGetRotationVector = OnGetRotationVector;
//    sensorManager->EnableRotationVector(16667);
//    eventLoop->SetRotationVectorSensor(sensorManager);

    eventLoop.Run();
}

EmptyScene::~EmptyScene()
{
    delete renderer;
    renderer = nullptr;
}

bool EmptyScene::UpdateImpl()
{
    return true;
}

bool OnActivate()
{
    DebugLog("App OnActivate");
    return true;
}

void OnDeactivate()
{
    DebugLog("App OnDeactivate");
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

void OnLowMemory(void)
{
    DebugLog("App OnLowMemory");
}

//void OnGetRotationVector(array<float, 4> rotationVector)
//{
//    //DebugLog("Sensor OnGetRotationVector");
//}

#endif