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
