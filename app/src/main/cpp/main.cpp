#include "event_loop.hpp"
#include "log.hpp"
#include "layer_extension.hpp"
#include "vulkan/vulkan.h"
#include "vulkan_main.hpp"
#include <memory>
#include <cassert>
#include <android/sensor.h>
#include <gtc/quaternion.hpp>

using std::unique_ptr;
using AndroidNative::EventLoop;
using AndroidNative::Log;

InstanceInfo instanceInfo;
SwapchainInfo swapchainInfo;
VkRenderPass renderPass;
set<VkCommandPool> commandPools;
vector<CommandInfo> commandInfos;
vector<DrawSyncPrimitives> drawSyncPrimitives;

vector<VertexV1> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},

    /*{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},*/
};
vector<uint32_t> indices = { 0, 1, 2, 0, 2, 3/*, 4, 5, 6, 6, 7, 4*/ };
BufferInfo bufferInfo;
PipelineInfo pipelineInfo;

VkDescriptorSetLayout descriptorSetLayout;
vector<VkBuffer> uniformBuffers;
vector<VkDeviceMemory> uniformBuffersMemory;
//VkDescriptorPool descriptorPool;
vector<VkDescriptorSet> descriptorSets;
set<VkDescriptorPool> descriptorPools;
vector<ResourceDescriptor> transformDescriptor;
TextureOject textureObject;

float rotationMatrix[16];

static status OnActivate();
static void OnDeactivate();
static status OnStep(android_app* app);

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

static void OnGetRotationVector(float* rotationVector);

void GetRotationMatrixFromVector(float R[], int length, float rotationVector[], int rotationVectorLength);

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

    loop->onGetRotationVector = OnGetRotationVector;

    loop->Run();
}

static status OnActivate()
{
    DebugLog("App OnActivate");
    return OK;
}

static void OnDeactivate()
{
    DebugLog("App OnDeactivate");
}

status OnStep(android_app* app)
{
    //DebugLog("App OnStep");

    VkResult result = DrawFrame(app, instanceInfo, swapchainInfo, renderPass, commandInfos, pipelineInfo, drawSyncPrimitives, bufferInfo, indices, uniformBuffersMemory, uniformBuffers, textureObject, transformDescriptor);
    if (result == VK_SUCCESS) {
        return OK;
    } else {
        return ERROR;
    }
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

    instanceInfo.width = ANativeWindow_getWidth(app->window);
    instanceInfo.height = ANativeWindow_getHeight(app->window);
    InitVulkan(app, instanceInfo, swapchainInfo, renderPass, commandPools, commandInfos, drawSyncPrimitives, vertices, indices, bufferInfo, pipelineInfo, uniformBuffers, uniformBuffersMemory, transformDescriptor, descriptorPools, textureObject);
}

void OnTermWindow(void)
{
    DebugLog("App OnTermWindow");

    DeleteVulkan(instanceInfo, commandPools, commandInfos, pipelineInfo, renderPass, swapchainInfo, drawSyncPrimitives, bufferInfo, uniformBuffers, uniformBuffersMemory, transformDescriptor, descriptorPools, textureObject);
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
    int32_t width = ANativeWindow_getWidth(app->window);
    int32_t height = ANativeWindow_getHeight(app->window);
    DebugLog("width: %d, height: %d", width, height);
    instanceInfo.width = height;
    instanceInfo.height = width;
    instanceInfo.resolutionChanged = true;
}

void OnLowMemory(void)
{
    DebugLog("App OnLowMemory");
}

void OnGetRotationVector(float rotationVector[])
{
//    mat4 transform(0.0f, -1.0f, 0.0f, 0.0f,
//                   0.0f, 0.0f, 1.0f, 0.0f,
//                   - 1.0f, 0.0f, 0.0f, 0.0f,
//                   0.0f, 0.0f, 0.0f, 1.0f);
    glm::quat q = glm::quat(rotationVector[3], rotationVector[0], rotationVector[1], rotationVector[2]);
    glm::quat n = glm::normalize(q);
//    mat4 qM = glm::mat4_cast(n);
    glm::quat base(0.7071068f, 0.0f, -0.7071068f, 0.0f);
    n = n * glm::inverse(base);
    rotationVector[0] = -n.y;
    rotationVector[1] = n.z;
    rotationVector[2] = -n.x;
    rotationVector[3] = n.w;
    glm::vec3 euler = glm::eulerAngles(n) * 180.0f / 3.141592f;
    GetRotationMatrixFromVector(rotationMatrix, 16, rotationVector, 4);
    UpdateDeviceOrientation(rotationMatrix, false);
}

// Row major
void GetRotationMatrixFromVector(float R[], int length, float rotationVector[], int rotationVectorLength)
{
    float q0;
    float q1 = rotationVector[0];
    float q2 = rotationVector[1];
    float q3 = rotationVector[2];
    if (rotationVectorLength >= 4) {
        q0 = rotationVector[3];
    } else {
        q0 = 1 - q1 * q1 - q2 * q2 - q3 * q3;
        q0 = (q0 > 0) ? sqrt(q0) : 0;
    }
    float sq_q1 = 2 * q1 * q1;
    float sq_q2 = 2 * q2 * q2;
    float sq_q3 = 2 * q3 * q3;
    float q1_q2 = 2 * q1 * q2;
    float q3_q0 = 2 * q3 * q0;
    float q1_q3 = 2 * q1 * q3;
    float q2_q0 = 2 * q2 * q0;
    float q2_q3 = 2 * q2 * q3;
    float q1_q0 = 2 * q1 * q0;
    if (length == 9) {
        R[0] = 1 - sq_q2 - sq_q3;
        R[1] = q1_q2 - q3_q0;
        R[2] = q1_q3 + q2_q0;
        R[3] = q1_q2 + q3_q0;
        R[4] = 1 - sq_q1 - sq_q3;
        R[5] = q2_q3 - q1_q0;
        R[6] = q1_q3 - q2_q0;
        R[7] = q2_q3 + q1_q0;
        R[8] = 1 - sq_q1 - sq_q2;
    } else if (length == 16) {
        R[0] = 1 - sq_q2 - sq_q3;
        R[1] = q1_q2 - q3_q0;
        R[2] = q1_q3 + q2_q0;
        R[3] = 0.0f;
        R[4] = q1_q2 + q3_q0;
        R[5] = 1 - sq_q1 - sq_q3;
        R[6] = q2_q3 - q1_q0;
        R[7] = 0.0f;
        R[8] = q1_q3 - q2_q0;
        R[9] = q2_q3 + q1_q0;
        R[10] = 1 - sq_q1 - sq_q2;
        R[11] = 0.0f;
        R[12] = R[13] = R[14] = 0.0f;
        R[15] = 1.0f;
    }
}