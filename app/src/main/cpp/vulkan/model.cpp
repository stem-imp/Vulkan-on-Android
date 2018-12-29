#include "model.h"
#include "../log/log.h"
#include "command.h"
#include "vulkan_utility.h"

using Utility::Log;

static void ProcessNode(aiNode* node, const aiScene* scene);
static void ProcessMesh(aiMesh* mesh, const aiScene* scene);
static void LoadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName);

namespace Vulkan {

    const int Model::DEFAULT_READ_FILE_FLAGS = aiProcess_Triangulate |
                                               aiProcess_ValidateDataStructure |
                                               aiProcess_RemoveRedundantMaterials |
                                               aiProcess_FixInfacingNormals |
                                               aiProcess_FindDegenerates |
                                               aiProcess_FindInvalidData |
                                               aiProcess_OptimizeMeshes |
                                               aiProcess_OptimizeGraph |
                                               aiProcess_FlipUVs;

    Model::Model(const Device& device) : vertices(device), indices(device)
    {
        DebugLog("Model()");
        this->device = device.LogicalDevice();
    }

    Model::Model(Model&& other) : vertices(std::move(other.vertices)), indices(std::move(other.indices))
    {
        DebugLog("Model(Model&&)");
        subMeshes   = std::move(other.subMeshes);
        device      = other.device               , other.device         = nullptr;
        indexCount  = other.indexCount           , other.indexCount     = 0;
        vertexCount = other.vertexCount          , other.vertexCount    = 0;
        dimension   = other.dimension            , other.dimension.size = vec3(0)
                                                 , other.dimension.min  = vec3(0)
                                                 , other.dimension.max  = vec3(0);
    }

    Model::~Model()
    {
        DebugLog("~Model()");
    }

    bool Model::RealFile(const string&       filename,
                         const VertexLayout& vertexLayout,
                         const Device&       device,
                         Command&            command,

                         ModelCreateInfo*    modelInfo,
                         int                 readFileFlags)
    {
        Assimp::Importer importer;
        if ((readFileFlags & aiProcess_Triangulate) != aiProcess_Triangulate) {
            readFileFlags |= aiProcess_Triangulate;
        }
        const aiScene* scene = importer.ReadFile(filename, readFileFlags);
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            Log::Error("%s", importer.GetErrorString());
            return false;
        }
        ProcessNode(scene->mRootNode, scene);

        subMeshes.clear();
        subMeshes.resize(scene->mNumMeshes);

        vec3 scale(1.0f);
        vec2 uvScale(1.0f);
        if (modelInfo)
        {
            scale = modelInfo->scale;
            uvScale = modelInfo->uvScale;
        }

        std::vector<float> vertexBuffer;
        std::vector<uint32_t> indexBuffer;

        vertexCount = 0;
        indexCount = 0;

        for (uint32_t i = 0; i < scene->mNumMeshes; i++) {
            const aiMesh* mesh = scene->mMeshes[i];

            subMeshes[i] = {};
            subMeshes[i].vertexBase = vertexCount;
            subMeshes[i].indexBase = indexCount;

            vertexCount += scene->mMeshes[i]->mNumVertices;

            aiColor3D pColor(0.f, 0.f, 0.f);
            scene->mMaterials[mesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);

            const aiVector3D ZERO3(0.0f, 0.0f, 0.0f);

            for (uint32_t j = 0; j < mesh->mNumVertices; j++) {
                const aiVector3D* pos = &(mesh->mVertices[j]);
                const aiVector3D* normal = &(mesh->mNormals[j]);
                const aiVector3D* texCoord = (mesh->HasTextureCoords(0)) ? &(mesh->mTextureCoords[0][j]) : &ZERO3;
                const aiVector3D* tangent = (mesh->HasTangentsAndBitangents()) ? &(mesh->mTangents[j]) : &ZERO3;
                const aiVector3D* biTangent = (mesh->HasTangentsAndBitangents()) ? &(mesh->mBitangents[j]) : &ZERO3;

                for (const auto& component : vertexLayout.components)
                {
                    switch (component) {
                    case VERTEX_COMPONENT_POSITION:
                        vertexBuffer.push_back(pos->x * scale.x);
                        vertexBuffer.push_back(pos->y * scale.y);
                        vertexBuffer.push_back(pos->z * scale.z);
                        break;
                    case VERTEX_COMPONENT_NORMAL:
                        vertexBuffer.push_back(normal->x);
                        vertexBuffer.push_back(normal->y);
                        vertexBuffer.push_back(normal->z);
                        break;
                    case VERTEX_COMPONENT_UV:
                        vertexBuffer.push_back(texCoord->x * uvScale.s);
                        vertexBuffer.push_back(texCoord->y * uvScale.t);
                        break;
                    case VERTEX_COMPONENT_COLOR:
                        vertexBuffer.push_back(pColor.r);
                        vertexBuffer.push_back(pColor.g);
                        vertexBuffer.push_back(pColor.b);
                        break;
                    case VERTEX_COMPONENT_TANGENT:
                        vertexBuffer.push_back(tangent->x);
                        vertexBuffer.push_back(tangent->y);
                        vertexBuffer.push_back(tangent->z);
                        break;
                    case VERTEX_COMPONENT_BITANGENT:
                        vertexBuffer.push_back(biTangent->x);
                        vertexBuffer.push_back(biTangent->y);
                        vertexBuffer.push_back(biTangent->z);
                        break;
                    };
                }

                dimension.max.x = fmax(pos->x, dimension.max.x);
                dimension.max.y = fmax(pos->y, dimension.max.y);
                dimension.max.z = fmax(pos->z, dimension.max.z);
                dimension.min.x = fmin(pos->x, dimension.min.x);
                dimension.min.y = fmin(pos->y, dimension.min.y);
                dimension.min.z = fmin(pos->z, dimension.min.z);
            }

            dimension.size = dimension.max - dimension.min;

            subMeshes[i].vertexCount = mesh->mNumVertices;

            uint32_t indexBase = static_cast<uint32_t>(indexBuffer.size());
            for (uint32_t j = 0; j < mesh->mNumFaces; j++)
            {
                const aiFace& Face = mesh->mFaces[j];
                if (Face.mNumIndices != 3) {
                    vertexBuffer.clear();
                    indexBuffer.clear();
                    subMeshes.clear();
                    return  false;
                }
                indexBuffer.push_back(indexBase + Face.mIndices[0]);
                indexBuffer.push_back(indexBase + Face.mIndices[1]);
                indexBuffer.push_back(indexBase + Face.mIndices[2]);
                subMeshes[i].indexCount += 3;
                indexCount += 3;
            }
        }

        uint32_t vBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(float);
        uint32_t iBufferSize = static_cast<uint32_t>(indexBuffer.size()) * sizeof(uint32_t);

        Buffer vertexStaging(device), indexStaging(device);

        vertexStaging.BuildDefaultBuffer(vBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        indexStaging.BuildDefaultBuffer(iBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vertices.BuildDefaultBuffer(vBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        indices.BuildDefaultBuffer(iBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VkDevice d = device.LogicalDevice();
        vector<VkCommandBuffer> copyCmds = Command::CreateAndBeginCommandBuffers(command.ShortLivedTransferPool(),
                                                                                 VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                                                 1,
                                                                                 VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                                                                                 d);
        VkCommandBuffer copyCmd = copyCmds[0];
        VkBufferCopy copyRegion = {};

        copyRegion.size = VK_WHOLE_SIZE;
        vkCmdCopyBuffer(copyCmd, vertexStaging.GetBuffer(), vertices.GetBuffer(), 1, &copyRegion);

        copyRegion.size = VK_WHOLE_SIZE;
        vkCmdCopyBuffer(copyCmd, indexStaging.GetBuffer(), indices.GetBuffer(), 1, &copyRegion);

        Command::EndAndSubmitCommandBuffer(copyCmd, device.FamilyQueues().transfer.queue, command.ShortLivedTransferPool(), d);

        return true;
    }
}

void ProcessNode(aiNode* node, const aiScene* scene)
{
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // the node object only contains indices to index the actual objects in the scene.
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        DebugLog("mesh index %d %s", node->mMeshes[i], mesh->mName.C_Str());
        ProcessMesh(mesh, scene);
    }

    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene);
    }

}

void ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    DebugLog("  vertices num %d", mesh->mNumVertices);
    int tmp = 0;
    for (int j = 0; j < mesh->mNumFaces; j++) {
        tmp += mesh->mFaces[j].mNumIndices;
    }
    DebugLog("  indices num %d", tmp);

    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    aiReturn res;
    aiColor3D color3;
    int mode;
    float valuef;
    res = material->Get(AI_MATKEY_COLOR_DIFFUSE, color3);
    DebugLog("  %d diffuse (%f, %f, %f)", res, color3.r, color3.g, color3.b);
    res = material->Get(AI_MATKEY_COLOR_SPECULAR, color3);
    DebugLog("  %d specular (%f, %f, %f)", res, color3.r, color3.g, color3.b);
    res = material->Get(AI_MATKEY_COLOR_AMBIENT, color3);
    DebugLog("  %d ambient (%f, %f, %f)", res, color3.r, color3.g, color3.b);
    res = material->Get(AI_MATKEY_COLOR_EMISSIVE, color3);
    DebugLog("  %d emissive (%f, %f, %f)", res, color3.r, color3.g, color3.b);
    res = material->Get(AI_MATKEY_COLOR_TRANSPARENT, color3);
    DebugLog("  %d transparent (%f, %f, %f)", res, color3.r, color3.g, color3.b);
    res = material->Get(AI_MATKEY_ENABLE_WIREFRAME, mode);
    DebugLog("  %d wireframe %d", res, mode);
    res = material->Get(AI_MATKEY_TWOSIDED, mode);
    DebugLog("  %d twosided %d", res, mode);
    res = material->Get(AI_MATKEY_SHADING_MODEL, mode);
    DebugLog("  %d shading model %d", res, mode);
    res = material->Get(AI_MATKEY_BLEND_FUNC, mode);
    DebugLog("  %d blend func %d", res, mode);
    res = material->Get(AI_MATKEY_OPACITY, valuef);
    DebugLog("  %d opacity %f", res, valuef);
    res = material->Get(AI_MATKEY_SHININESS, valuef);
    DebugLog("  %d shininess %f", res, valuef);
    res = material->Get(AI_MATKEY_SHININESS_STRENGTH, valuef);
    DebugLog("  %d shininess strength %f", res, valuef);
    res = material->Get(AI_MATKEY_REFRACTI, valuef);
    DebugLog("  %d refracti %f", res, valuef);

    LoadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    LoadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    LoadMaterialTextures(material, aiTextureType_AMBIENT, "texture_ambient");
    LoadMaterialTextures(material, aiTextureType_EMISSIVE, "texture_emissive");
    LoadMaterialTextures(material, aiTextureType_HEIGHT, "texture_height");
    LoadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal");
    LoadMaterialTextures(material, aiTextureType_SHININESS, "texture_shininess");
    LoadMaterialTextures(material, aiTextureType_OPACITY, "texture_opacity");
    LoadMaterialTextures(material, aiTextureType_DISPLACEMENT, "texture_displacement");
    LoadMaterialTextures(material, aiTextureType_LIGHTMAP, "texture_lightmap");
    LoadMaterialTextures(material, aiTextureType_REFLECTION, "texture_reflection");
}

void LoadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
{
    int textureCount = mat->GetTextureCount(type);
    DebugLog("  %d textures of type %s", textureCount, typeName.c_str());
    for(unsigned int i = 0; i < textureCount; i++)
    {
        aiString str1, str2;
        aiReturn res1, res2, res3, res4, res5, res6, res7, res8, res9, res10;
        float f3;
        int int4, int5, int6, int7, int8, int10;
        aiVector3D v3d9;
        res1 = mat->GetTexture(type, i, &str1);
        res2 = mat->Get(AI_MATKEY_TEXTURE(type, i), str2);
        res3 = mat->Get(AI_MATKEY_TEXBLEND(type, i), f3);
        res4 = mat->Get(AI_MATKEY_TEXOP(type, i), int4);
        res5 = mat->Get(AI_MATKEY_MAPPING(type, i), int5);
        res6 = mat->Get(AI_MATKEY_UVWSRC(type, i), int6);
        res7 = mat->Get(AI_MATKEY_MAPPINGMODE_U(type, i), int7);
        res8 = mat->Get(AI_MATKEY_MAPPINGMODE_V(type, i), int8);
        res9 = mat->Get(AI_MATKEY_TEXMAP_AXIS(type, i), v3d9);
        res10 = mat->Get(AI_MATKEY_TEXFLAGS(type, i), int10);
        // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
        DebugLog("  %d, %d, %s, PATH, %s",                  i, res1, typeName.c_str(), str1.C_Str());
        DebugLog("  %d, %d, %s, TEXTURE, %s",               i, res2, typeName.c_str(), str2.C_Str());
        DebugLog("  %d, %d, %s, TEXBLEND, %f",              i, res3, typeName.c_str(), f3);
        DebugLog("  %d, %d, %s, TEXOP, %d",                 i, res4, typeName.c_str(), int4);
        DebugLog("  %d, %d, %s, MAPPING, %d",               i, res5, typeName.c_str(), int5);
        DebugLog("  %d, %d, %s, UVWSRC, %d",                i, res6, typeName.c_str(), int6);
        DebugLog("  %d, %d, %s, MAPPINGMODE_U, %d",         i, res7, typeName.c_str(), int7);
        DebugLog("  %d, %d, %s, MAPPINGMODE_V, %d",         i, res8, typeName.c_str(), int8);
        DebugLog("  %d, %d, %s, TEXMAP_AXIS, (%f, %f, %f)", i, res9, typeName.c_str(), v3d9.x, v3d9.y, v3d9.z);
        DebugLog("  %d, %d, %s, TEXFLAGS, %d",              i, res10, typeName.c_str(), int10);
    }
}