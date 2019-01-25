#ifdef __ANDROID__

#include "../stereo_viewing_scene.h"
#include "../stereo_viewing_scene_renderer.h"
#include <android_native_app_glue.h>
#include "../../../log/log.h"
#include "glm/gtc/matrix_transform.hpp"

using Vulkan::ModelCreateInfo;
using std::chrono::duration;
using std::chrono::seconds;

using std::max;

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

StereoViewingScene::StereoViewingScene(void* state) : Scene(state),
                                                      _worldSpaceCameraPos(0.0f, 0.0f, 2.0f),
                                                      _fov(45.0f),
                                                      _zNear(0.125f), _zFar(128.0f),
                                                      _focalLength(0.5f),
                                                      _eyeSeparation(0.08f)
{
    eventLoop.onActivate = OnActivate;
    eventLoop.onDeactivate = OnDeactivate;

    eventLoop.onStart = OnStart;
    eventLoop.onResume = OnResume;
    eventLoop.onPause = OnPause;
    eventLoop.onStop = OnStop;
    eventLoop.onDestroy = OnDestroy;

    eventLoop.onInitWindow = [this](android_app* app) -> void {

        int32_t w = ANativeWindow_getWidth(app->window);
        int32_t h = ANativeWindow_getHeight(app->window);
        _screenWidth = w;
        _screenHeight = h;
        renderer = new StereoViewingSceneRenderer(app, w, h);

        StereoViewingSceneRenderer* concreteRenderer = (StereoViewingSceneRenderer*)renderer;
        concreteRenderer->UploadModels(_models);
        concreteRenderer->BuildTextureSamplers();
        concreteRenderer->BuildMSAAImage(concreteRenderer->SampleCount(), 0);
        concreteRenderer->BuildMSAAImage(concreteRenderer->SampleCount(), 1);
        concreteRenderer->BuildMSAADepthImage(concreteRenderer->RenderPasses()[0],
                                              concreteRenderer->SampleCount(),
                                              0);
        concreteRenderer->BuildMSAADepthImage(concreteRenderer->RenderPasses()[0],
                                              concreteRenderer->SampleCount(),
                                              1);
        concreteRenderer->BuildMSAAResolvedImages(0);
        concreteRenderer->BuildMSAAResolvedImages(1);
        concreteRenderer->BuildMSAAResolvedResultSampler();
        concreteRenderer->BuildMSAADescriptorSetLayout();
        concreteRenderer->BuildMultiviewDescriptorSetLayout();
        concreteRenderer->BuildDescriptorPool();
        concreteRenderer->BuildMSAADescriptorSet();
        concreteRenderer->BuildMultiViewDescriptorSet(0);
        concreteRenderer->BuildMultiViewDescriptorSet(1);
        concreteRenderer->BuildMSAAPipeline(app, _models[0].VertexLayouts()[0], concreteRenderer->SampleCount());
        VertexLayout vertexLayout;
        vertexLayout.offsets.push_back(0);
        vertexLayout.components.push_back(Vulkan::VertexComponent::VERTEX_COMPONENT_POSITION);
        concreteRenderer->BuildMultiviewPipeline(app, vertexLayout);
        concreteRenderer->SetVertexLayouts({ _models[0].VertexLayouts()[0], vertexLayout });

        OnInitWindow(app);

        if (!eventLoop.onStep) {
            eventLoop.onStep = [this]() -> bool {
                return Update();
            };
        }
    };

    eventLoop.onTermWindow = OnTermWindow;
    eventLoop.onGainFocus = OnGainFocus;
    eventLoop.onLostFocus = OnLostFocus;

    eventLoop.onSaveInstanceState = OnSaveInstanceState;
    eventLoop.onConfigurationChanged = [this](android_app* app) -> void {
        if (!app->window) {
            Log::Error("Window handle is null.");
            return;
        }
        DebugLog("App OnConfigurationChanged");
        _screenHeight = ANativeWindow_getWidth(app->window);
        _screenWidth = ANativeWindow_getHeight(app->window);
        DebugLog("Old (width, height) = (%d, %d)", _screenHeight, _screenWidth);
        renderer->SetScreenSize(_screenWidth, _screenHeight);
        renderer->ChangeOrientation();
    };
    eventLoop.onLowMemory = OnLowMemory;

    android_app* app = (android_app*)state;
    _models.emplace_back(Model());
    _modelTransforms.emplace_back(mat4(1.0));
    Model& model = _models[0];
    string filePath = string(app->activity->externalDataPath) + string("/earth/");
    //Texture::TextureAttribs textureAttribs;
    ModelCreateInfo modelCreateInfo = { 0.001953125f, 1.0f, true, false };
    if (model.ReadFile(filePath, string("earth.obj"), Model::DEFAULT_READ_FILE_FLAGS, &modelCreateInfo)) {
        //_modelResources.emplace_back(*device);
        //_modelResources[_modelResources.size() - 1].UploadToGPU(model, *command);
    } else {
        Log::Error("%s: file not found.", (filePath + string("earth.obj")).c_str());
        _models.pop_back();
    }

    // square plane
    vector<Model::Mesh> subMeshes(1, Model::Mesh());
    Model::Dimension dimension;
    int rowNodes = 32;
    int colNodes = 32;
    // vertices
    for (int r = 0; r <= rowNodes; r++) {
        for (int c = 0; c <= colNodes; c++) {
            float x = 1.0f * c / colNodes * 2 - 1;
            float y = 1.0f * r / rowNodes * 2 - 1;
            subMeshes[0].vertexBuffer.push_back(x);
            subMeshes[0].vertexBuffer.push_back(y);
            subMeshes[0].vertexBuffer.push_back(0);

            dimension.max.x = fmax(x, dimension.max.x);
            dimension.max.y = fmax(y, dimension.max.y);
            dimension.min.x = fmin(x, dimension.min.x);
            dimension.min.y = fmin(y, dimension.min.y);
        }
    }
    dimension.max.z = dimension.min.z = 0.0f;
    dimension.size = dimension.max - dimension.min;
    // indices
    subMeshes[0].indexBuffer.resize(rowNodes * colNodes * 6);
    for (uint32_t ti = 0, vi = 0, y = 0; y < rowNodes; y++, vi++) {
        for (uint32_t x = 0; x < colNodes; x++, ti += 6, vi++) {
            subMeshes[0].indexBuffer[ti] = vi;
            subMeshes[0].indexBuffer[ti + 3] = subMeshes[0].indexBuffer[ti + 2] = vi + 1;
            subMeshes[0].indexBuffer[ti + 4] = subMeshes[0].indexBuffer[ti + 1] = vi + colNodes + 1;
            subMeshes[0].indexBuffer[ti + 5] = vi + colNodes + 2;
        }
    }
    vector<VertexLayout> vertexLayouts(1, Vulkan::VertexLayout());
    vertexLayouts[0].components.push_back(Vulkan::VertexComponent::VERTEX_COMPONENT_POSITION);
    vertexLayouts[0].offsets.push_back(0);
    _models.emplace_back(Model(std::move(subMeshes), std::move(vertexLayouts), dimension, {}, {}));

    eventLoop.Run();
}

StereoViewingScene::~StereoViewingScene()
{
    DebugLog("~StereoViewingScene()");
    _models.clear();
    _modelTransforms.clear();
    delete renderer;
    renderer = nullptr;
}

bool StereoViewingScene::UpdateImpl()
{
    StereoViewingSceneRenderer* concreteRenderer = (StereoViewingSceneRenderer*)renderer;
    auto now = high_resolution_clock::now();
    float elapsedTime = duration<float, seconds::period>(now - startTime).count();
    _modelTransforms[0] = glm::rotate(mat4(1.0f), glm::radians(elapsedTime * 1024.0f), vec3(1.5f, 0.5f, -1.0f));

    float aspectRatio = (_screenWidth * 0.5f) / _screenHeight;
    float wd2 = _zNear * tan(glm::radians(_fov / 2.0f));
    float ndfl = _zNear / _focalLength;
    float left, right;
    float top = wd2;
    float bottom = -wd2;

    left = -aspectRatio * wd2 + 0.5f * _eyeSeparation * ndfl;
    right = aspectRatio * wd2 + 0.5f * _eyeSeparation * ndfl;
    _lViewProjTransform.view = lookAt(vec3(-0.5f * _eyeSeparation, 0.0f, 24.0f), vec3(-0.5f * _eyeSeparation, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    _lViewProjTransform.projection = glm::frustum(left, right, bottom, top, _zNear, _zFar);
    _lViewProjTransform.projection[1][1] *= -1;

    left = -aspectRatio * wd2 - 0.5f * _eyeSeparation * ndfl;
    right = aspectRatio * wd2 - 0.5f * _eyeSeparation * ndfl;
    _rViewProjTransform.view = lookAt(vec3(0.5f * _eyeSeparation, 0.0f, 24.0f), vec3(0.5f * _eyeSeparation, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    _rViewProjTransform.projection = glm::frustum(left, right, bottom, top, _zNear, _zFar);
    _rViewProjTransform.projection[1][1] *= -1;

    vector<int> modelTransformSizes = { sizeof(mat4) };
    concreteRenderer->UpdateUniformBuffers(_modelTransforms, modelTransformSizes, _lViewProjTransform, _rViewProjTransform, sizeof(ViewProjectionTransform));
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

#endif