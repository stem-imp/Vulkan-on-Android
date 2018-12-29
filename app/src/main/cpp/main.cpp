//#include "event_loop.hpp"
//#include "log.hpp"
//#include "layer_extension.hpp"
//#include "vulkan/vulkan.h"
//#include "vulkan_main.hpp"
//#include <memory>
//#include <cassert>
//
//using std::unique_ptr;
//using AndroidNative::EventLoop;
//using AndroidNative::Log;
//
//InstanceInfo instanceInfo;
//SwapchainInfo swapchainInfo;
//VkRenderPass renderPass;
//set<VkCommandPool> commandPools;
//vector<CommandInfo> commandInfos;
//vector<DrawSyncPrimitives> drawSyncPrimitives;
//vector<VertexV1> vertices = {
//    {{0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
//    {{-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
//    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
//};
//vector<uint16_t> indices = { 0, 1, 2 };
//BufferInfo bufferInfo;
//PipelineInfo pipelineInfo;
//
//static status OnActivate();
//static void OnDeactivate();
//static status OnStep(android_app* app);
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
//static status OnActivate()
//{
//    DebugLog("App OnActivate");
//    return OK;
//}
//
//static void OnDeactivate()
//{
//    DebugLog("App OnDeactivate");
//}
//
//status OnStep(android_app* app)
//{
//    //DebugLog("App OnStep");
//
//    VkResult result = DrawFrame(app, instanceInfo, swapchainInfo, renderPass, commandInfos, pipelineInfo, drawSyncPrimitives, bufferInfo, indices);
//    if (result == VK_SUCCESS) {
//        return OK;
//    } else {
//        return ERROR;
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
//    InitVulkan(app, instanceInfo, swapchainInfo, renderPass, commandPools, commandInfos, drawSyncPrimitives, vertices, indices, bufferInfo, pipelineInfo);
//}
//
//void OnTermWindow(void)
//{
//    DebugLog("App OnTermWindow");
//
//    DeleteVulkan(instanceInfo, commandPools, commandInfos, pipelineInfo, renderPass, swapchainInfo, drawSyncPrimitives, bufferInfo);
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
#include "scene/earthscene/earth_scene.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

using Utility::Log;

//void ProcessNode(aiNode* node, const aiScene* scene);
//void ProcessMesh(aiMesh* mesh, const aiScene* scene);
//void LoadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName);

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue. It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state)
{
    Log::Tag = "vulkan";

//    Assimp::Importer importer;
//    DebugLog("externalDataPath %s", state->activity->externalDataPath);
//    DebugLog("internalDataPath %s", state->activity->internalDataPath);
//    const aiScene* scene = importer.ReadFile(string(state->activity->externalDataPath) + string("/earth/earth.obj"),
//            aiProcess_Triangulate |
//            aiProcess_ValidateDataStructure |
//            aiProcess_RemoveRedundantMaterials |
//            aiProcess_FixInfacingNormals |
//            aiProcess_FindDegenerates |
//            aiProcess_FindInvalidData |
//            aiProcess_OptimizeMeshes |
//            aiProcess_OptimizeGraph |
//            aiProcess_FlipUVs);
//    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
//    {
//        Log::Error("%s", importer.GetErrorString());
//        ANativeActivity_finish(state->activity);
//        return;
//    }
//    ProcessNode(scene->mRootNode, scene);

    //EmptyScene emptyScene((void*)state);
    EarthScene earthScene((void*)state);
}

//void ProcessNode(aiNode* node, const aiScene* scene)
//{
//    for(unsigned int i = 0; i < node->mNumMeshes; i++)
//    {
//        // the node object only contains indices to index the actual objects in the scene.
//        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
//        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
//        DebugLog("mesh index %d %s", node->mMeshes[i], mesh->mName.C_Str());
//        ProcessMesh(mesh, scene);
//    }
//
//    for(unsigned int i = 0; i < node->mNumChildren; i++)
//    {
//        ProcessNode(node->mChildren[i], scene);
//    }
//
//}
//
//void ProcessMesh(aiMesh* mesh, const aiScene* scene)
//{
//    DebugLog("  vertices num %d", mesh->mNumVertices);
//    int tmp = 0;
//    for (int j = 0; j < mesh->mNumFaces; j++) {
//        tmp += mesh->mFaces[j].mNumIndices;
//    }
//    DebugLog("  indices num %d", tmp);
//
//    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
//
//    aiReturn res;
//    aiColor3D color3;
//    int mode;
//    float valuef;
//    res = material->Get(AI_MATKEY_COLOR_DIFFUSE, color3);
//    DebugLog("  %d diffuse (%f, %f, %f)", res, color3.r, color3.g, color3.b);
//    res = material->Get(AI_MATKEY_COLOR_SPECULAR, color3);
//    DebugLog("  %d specular (%f, %f, %f)", res, color3.r, color3.g, color3.b);
//    res = material->Get(AI_MATKEY_COLOR_AMBIENT, color3);
//    DebugLog("  %d ambient (%f, %f, %f)", res, color3.r, color3.g, color3.b);
//    res = material->Get(AI_MATKEY_COLOR_EMISSIVE, color3);
//    DebugLog("  %d emissive (%f, %f, %f)", res, color3.r, color3.g, color3.b);
//    res = material->Get(AI_MATKEY_COLOR_TRANSPARENT, color3);
//    DebugLog("  %d transparent (%f, %f, %f)", res, color3.r, color3.g, color3.b);
//    res = material->Get(AI_MATKEY_ENABLE_WIREFRAME, mode);
//    DebugLog("  %d wireframe %d", res, mode);
//    res = material->Get(AI_MATKEY_TWOSIDED, mode);
//    DebugLog("  %d twosided %d", res, mode);
//    res = material->Get(AI_MATKEY_SHADING_MODEL, mode);
//    DebugLog("  %d shading model %d", res, mode);
//    res = material->Get(AI_MATKEY_BLEND_FUNC, mode);
//    DebugLog("  %d blend func %d", res, mode);
//    res = material->Get(AI_MATKEY_OPACITY, valuef);
//    DebugLog("  %d opacity %f", res, valuef);
//    res = material->Get(AI_MATKEY_SHININESS, valuef);
//    DebugLog("  %d shininess %f", res, valuef);
//    res = material->Get(AI_MATKEY_SHININESS_STRENGTH, valuef);
//    DebugLog("  %d shininess strength %f", res, valuef);
//    res = material->Get(AI_MATKEY_REFRACTI, valuef);
//    DebugLog("  %d refracti %f", res, valuef);
//
//    LoadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
//    LoadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
//    LoadMaterialTextures(material, aiTextureType_AMBIENT, "texture_ambient");
//    LoadMaterialTextures(material, aiTextureType_EMISSIVE, "texture_emissive");
//    LoadMaterialTextures(material, aiTextureType_HEIGHT, "texture_height");
//    LoadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal");
//    LoadMaterialTextures(material, aiTextureType_SHININESS, "texture_shininess");
//    LoadMaterialTextures(material, aiTextureType_OPACITY, "texture_opacity");
//    LoadMaterialTextures(material, aiTextureType_DISPLACEMENT, "texture_displacement");
//    LoadMaterialTextures(material, aiTextureType_LIGHTMAP, "texture_lightmap");
//    LoadMaterialTextures(material, aiTextureType_REFLECTION, "texture_reflection");
//}
//
//void LoadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
//{
//    int textureCount = mat->GetTextureCount(type);
//    DebugLog("  %d textures of type %s", textureCount, typeName.c_str());
//    for(unsigned int i = 0; i < textureCount; i++)
//    {
//        aiString str1, str2;
//        aiReturn res1, res2, res3, res4, res5, res6, res7, res8, res9, res10;
//        float f3;
//        int int4, int5, int6, int7, int8, int10;
//        aiVector3D v3d9;
//        res1 = mat->GetTexture(type, i, &str1);
//        res2 = mat->Get(AI_MATKEY_TEXTURE(type, i), str2);
//        res3 = mat->Get(AI_MATKEY_TEXBLEND(type, i), f3);
//        res4 = mat->Get(AI_MATKEY_TEXOP(type, i), int4);
//        res5 = mat->Get(AI_MATKEY_MAPPING(type, i), int5);
//        res6 = mat->Get(AI_MATKEY_UVWSRC(type, i), int6);
//        res7 = mat->Get(AI_MATKEY_MAPPINGMODE_U(type, i), int7);
//        res8 = mat->Get(AI_MATKEY_MAPPINGMODE_V(type, i), int8);
//        res9 = mat->Get(AI_MATKEY_TEXMAP_AXIS(type, i), v3d9);
//        res10 = mat->Get(AI_MATKEY_TEXFLAGS(type, i), int10);
//        // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
//        DebugLog("  %d, %d, %s, PATH, %s",                  i, res1, typeName.c_str(), str1.C_Str());
//        DebugLog("  %d, %d, %s, TEXTURE, %s",               i, res2, typeName.c_str(), str2.C_Str());
//        DebugLog("  %d, %d, %s, TEXBLEND, %f",              i, res3, typeName.c_str(), f3);
//        DebugLog("  %d, %d, %s, TEXOP, %d",                 i, res4, typeName.c_str(), int4);
//        DebugLog("  %d, %d, %s, MAPPING, %d",               i, res5, typeName.c_str(), int5);
//        DebugLog("  %d, %d, %s, UVWSRC, %d",                i, res6, typeName.c_str(), int6);
//        DebugLog("  %d, %d, %s, MAPPINGMODE_U, %d",         i, res7, typeName.c_str(), int7);
//        DebugLog("  %d, %d, %s, MAPPINGMODE_V, %d",         i, res8, typeName.c_str(), int8);
//        DebugLog("  %d, %d, %s, TEXMAP_AXIS, (%f, %f, %f)", i, res9, typeName.c_str(), v3d9.x, v3d9.y, v3d9.z);
//        DebugLog("  %d, %d, %s, TEXFLAGS, %d",              i, res10, typeName.c_str(), int10);
//    }
//}