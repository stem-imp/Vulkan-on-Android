//#include "androidutility/eventloop/event_loop.h"
//#include "log/log.h"
//#include "vulkan/layer_extension.h"
//#include "vulkan/vulkan.h"
//#include "vulkan_main.hpp"
//#include <memory>
//#include <cassert>
//
//using std::unique_ptr;
//using AndroidNative::EventLoop;
//using Utility::Log;
//
//InstanceInfo instanceInfo;
//SwapchainInfo swapchainInfo;
//VkRenderPass renderPass;
//set<VkCommandPool> commandPools;
//vector<CommandInfo> commandInfos;
//vector<DrawSyncPrimitives> drawSyncPrimitives;
//
//static bool OnActivate();
//static void OnDeactivate();
//static bool OnStep();
//
//static void OnStart(void);
//static void OnResume(void);
//static void OnPause(void);
//static void OnStop(void);
//static void OnDestroy(void);
//
//static void OnInitWindow(android_app* app);
//static void OnTermWindow(void);
//static void OnGainFocus(void);
//static void OnLostFocus(void);
//
//static void OnSaveInstanceState(void**, size_t*);
//static void OnConfigurationChanged(android_app* app);
//static void OnLowMemory(void);
//
///**
// * This is the main entry point of a native application that is using
// * android_native_app_glue.  It runs in its own thread, with its own
// * event loop for receiving input events and doing other things.
// */
//void android_main(struct android_app* state)
//{
//    Log::Tag = "vulkan";
//    unique_ptr<EventLoop> loop(new EventLoop(state));
//    loop->onActivate = OnActivate;
//    loop->onDeactivate = OnDeactivate;
//    loop->onStep = OnStep;
//
//    loop->onStart = OnStart;
//    loop->onResume = OnResume;
//    loop->onPause = OnPause;
//    loop->onStop = OnStop;
//    loop->onDestroy = OnDestroy;
//
//    loop->onInitWindow = OnInitWindow;
//    loop->onTermWindow = OnTermWindow;
//    loop->onGainFocus = OnGainFocus;
//    loop->onLostFocus = OnLostFocus;
//
//    loop->onSaveInstanceState = OnSaveInstanceState;
//    loop->onConfigurationChanged = OnConfigurationChanged;
//    loop->onLowMemory = OnLowMemory;
//
//    loop->Run();
//}
//
//static bool OnActivate()
//{
//    DebugLog("App OnActivate");
//    return true;
//}
//
//static void OnDeactivate()
//{
//    DebugLog("App OnDeactivate");
//}
//
//bool OnStep()
//{
//    //DebugLog("App OnStep");
//
//    VkResult result = DrawFrame(instanceInfo, swapchainInfo, renderPass, commandInfos, drawSyncPrimitives);
//    if (result == VK_SUCCESS) {
//        return true;
//    } else {
//        return false;
//    }
//}
//
//void OnStart(void)
//{
//    DebugLog("App OnStart");
//}
//
//void OnResume(void)
//{
//    DebugLog("App OnResume");
//}
//
//void OnPause(void)
//{
//    DebugLog("App OnPause");
//}
//
//void OnStop(void)
//{
//    DebugLog("App OnStop");
//}
//
//void OnDestroy(void)
//{
//    DebugLog("App OnDestroy");
//}
//
//void OnInitWindow(android_app* app)
//{
//    DebugLog("App OnInitWindow");
//
//    instanceInfo.width = ANativeWindow_getWidth(app->window);
//    instanceInfo.height = ANativeWindow_getHeight(app->window);
//    InitVulkan(app, instanceInfo, swapchainInfo, renderPass, commandPools, commandInfos, drawSyncPrimitives);
//}
//
//void OnTermWindow(void)
//{
//    DebugLog("App OnTermWindow");
//
//    DeleteVulkan(instanceInfo, commandPools, commandInfos, renderPass, swapchainInfo, drawSyncPrimitives);
//}
//
//void OnGainFocus(void)
//{
//    DebugLog("App OnGainFocus");
//}
//
//void OnLostFocus(void)
//{
//    DebugLog("App OnLostFocus");
//}
//
//void OnSaveInstanceState(void**, size_t*)
//{
//    DebugLog("App OnSaveInstanceState");
//}
//
//void OnConfigurationChanged(android_app* app)
//{
//    DebugLog("App OnConfigurationChanged");
//    int32_t width = ANativeWindow_getWidth(app->window);
//    int32_t height = ANativeWindow_getHeight(app->window);
//    DebugLog("width: %d, height: %d", width, height);
//    instanceInfo.width = height;
//    instanceInfo.height = width;
//    instanceInfo.resolutionChanged = true;
//}
//
//void OnLowMemory(void)
//{
//    DebugLog("App OnLowMemory");
//}

#include "log/log.h"
#include "scene/emptyscene/empty_scene.h"

using Utility::Log;

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue. It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state)
{
    Log::Tag = "vulkan";
    EmptyScene emptyScene((void*)state);
}