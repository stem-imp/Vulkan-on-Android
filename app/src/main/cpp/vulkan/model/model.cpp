#include "model.h"
#include "../../log/log.h"

using Utility::Log;

//static void ProcessNode(aiNode* node, const aiScene* scene);
//static void ProcessMesh(aiMesh* mesh, const aiScene* scene);
//static void LoadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName);

namespace Vulkan
{
    const unsigned int Model::DEFAULT_READ_FILE_FLAGS = aiProcess_Triangulate |
                                                        aiProcess_GenNormals |
                                                        aiProcess_GenUVCoords |
                                                        aiProcess_CalcTangentSpace |
                                                        aiProcess_ValidateDataStructure |
                                                        aiProcess_RemoveRedundantMaterials |
                                                        aiProcess_FixInfacingNormals |
                                                        aiProcess_FindDegenerates |
                                                        aiProcess_FindInvalidData |
                                                        aiProcess_OptimizeMeshes |
                                                        aiProcess_OptimizeGraph |
                                                        aiProcess_FlipUVs;

    bool Model::ReadFile(const string& filePath, const string& filename, unsigned int readFileFlags, ModelCreateInfo* modelInfo)
    {
        if ((readFileFlags & aiProcess_Triangulate) != aiProcess_Triangulate) {
            readFileFlags |= aiProcess_Triangulate;
        }
        Assimp::Importer importer;
        scene = importer.ReadFile(filePath + filename, readFileFlags);
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            Log::Error("%s", importer.GetErrorString());
            scene = nullptr;
            return false;
        }
#ifndef NDEBUG
        ProcessNode(scene->mRootNode, scene);
#endif

        _subMeshes.clear();
        _subMeshes.resize(scene->mNumMeshes);

        vec3 scale(1.0f);
        vec2 uvScale(1.0f);
        if (modelInfo) {
            scale = modelInfo->scale;
            uvScale = modelInfo->uvScale;
        }

        aiVector3D ZERO3(0.0f, 0.0f, 0.0f);
        _vertexLayouts.resize(scene->mNumMeshes);
        for (uint32_t i = 0; i < scene->mNumMeshes; i++) {
            const aiMesh* mesh = scene->mMeshes[i];

            _subMeshes[i] = {};
            _vertexLayouts[i].components.clear();
            _vertexLayouts[i].offsets.clear();
            bool hasPositions = mesh->HasPositions();
            bool hasNormals = mesh->HasNormals();
            bool hasUVs = mesh->HasTextureCoords(0);
            aiColor3D pColor(0.f, 0.f, 0.f);
            bool hasColors = (scene->mMaterials[mesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor) == aiReturn_SUCCESS);
            bool hasTangentsAndBitangents = mesh->HasTangentsAndBitangents();
            int baseOffset = 0;
            if (hasPositions) {
                _vertexLayouts[i].components.push_back(VERTEX_COMPONENT_POSITION);
                _vertexLayouts[i].offsets.push_back(baseOffset);
                baseOffset += 3 * sizeof(float);
            }
            if (hasNormals) {
                _vertexLayouts[i].components.push_back(VERTEX_COMPONENT_NORMAL);
                _vertexLayouts[i].offsets.push_back(baseOffset);
                baseOffset += 3 * sizeof(float);
            }
            if (hasUVs) {
                _vertexLayouts[i].components.push_back(VERTEX_COMPONENT_UV);
                _vertexLayouts[i].offsets.push_back(baseOffset);
                baseOffset += 2 * sizeof(float);
            }
            if (hasColors) {
                _vertexLayouts[i].components.push_back(VERTEX_COMPONENT_COLOR);
                _vertexLayouts[i].offsets.push_back(baseOffset);
                baseOffset += 3 * sizeof(float);
            }
            if (hasTangentsAndBitangents) {
                _vertexLayouts[i].components.push_back(VERTEX_COMPONENT_TANGENT);
                _vertexLayouts[i].components.push_back(VERTEX_COMPONENT_BITANGENT);
                _vertexLayouts[i].offsets.push_back(baseOffset);
                baseOffset += 3 * sizeof(float);
                _vertexLayouts[i].offsets.push_back(baseOffset);
            }

            for (uint32_t j = 0; j < mesh->mNumVertices; j++) {
                aiVector3D* pos;
                if (hasPositions) {
                    pos = &(mesh->mVertices[j]);
                    _subMeshes[i].vertexBuffer.push_back(pos->x * scale.x);
                    _subMeshes[i].vertexBuffer.push_back(pos->y * scale.y);
                    _subMeshes[i].vertexBuffer.push_back(pos->z * scale.z);
                }

                aiVector3D* normal;
                if (hasNormals) {
                    normal = &(mesh->mNormals[j]);
                    _subMeshes[i].vertexBuffer.push_back(normal->x);
                    _subMeshes[i].vertexBuffer.push_back(normal->y);
                    _subMeshes[i].vertexBuffer.push_back(normal->z);
                }

                aiVector3D* texCoord;
                if (hasUVs) {
                    texCoord = &(mesh->mTextureCoords[0][j]);
                    _subMeshes[i].vertexBuffer.push_back(texCoord->x * uvScale.s);
                    _subMeshes[i].vertexBuffer.push_back(texCoord->y * uvScale.t);
                }

                if (hasColors) {
                    _subMeshes[i].vertexBuffer.push_back(pColor.r);
                    _subMeshes[i].vertexBuffer.push_back(pColor.g);
                    _subMeshes[i].vertexBuffer.push_back(pColor.b);
                }

                const aiVector3D* tangent;
                const aiVector3D* biTangent;
                if (hasTangentsAndBitangents) {
                    tangent = &(mesh->mTangents[j]);
                    _subMeshes[i].vertexBuffer.push_back(tangent->x);
                    _subMeshes[i].vertexBuffer.push_back(tangent->y);
                    _subMeshes[i].vertexBuffer.push_back(tangent->z);
                    biTangent = &(mesh->mBitangents[j]);
                    _subMeshes[i].vertexBuffer.push_back(biTangent->x);
                    _subMeshes[i].vertexBuffer.push_back(biTangent->y);
                    _subMeshes[i].vertexBuffer.push_back(biTangent->z);
                }

                _dimension.max.x = fmax(pos->x, _dimension.max.x);
                _dimension.max.y = fmax(pos->y, _dimension.max.y);
                _dimension.max.z = fmax(pos->z, _dimension.max.z);
                _dimension.min.x = fmin(pos->x, _dimension.min.x);
                _dimension.min.y = fmin(pos->y, _dimension.min.y);
                _dimension.min.z = fmin(pos->z, _dimension.min.z);
            }

            _dimension.size = _dimension.max - _dimension.min;

            for (uint32_t j = 0; j < mesh->mNumFaces; j++)
            {
                const aiFace& Face = mesh->mFaces[j];
                _subMeshes[i].indexBuffer.push_back(Face.mIndices[0]);
                _subMeshes[i].indexBuffer.push_back(Face.mIndices[1]);
                _subMeshes[i].indexBuffer.push_back(Face.mIndices[2]);
            }

            _materialIndices.push_back(mesh->mMaterialIndex);
        }

        _materials.resize(scene->mNumMaterials);
        for (int i = 0; i < scene->mNumMaterials; i++) {
            aiMaterial* material = scene->mMaterials[i];
            LoadMaterialTextures(material, aiTextureType_DIFFUSE     , i);
            LoadMaterialTextures(material, aiTextureType_SPECULAR    , i);
            LoadMaterialTextures(material, aiTextureType_AMBIENT     , i);
            LoadMaterialTextures(material, aiTextureType_EMISSIVE    , i);
            LoadMaterialTextures(material, aiTextureType_HEIGHT      , i);
            LoadMaterialTextures(material, aiTextureType_NORMALS     , i);
            LoadMaterialTextures(material, aiTextureType_SHININESS   , i);
            LoadMaterialTextures(material, aiTextureType_OPACITY     , i);
            LoadMaterialTextures(material, aiTextureType_DISPLACEMENT, i);
            LoadMaterialTextures(material, aiTextureType_LIGHTMAP    , i);
            LoadMaterialTextures(material, aiTextureType_REFLECTION  , i);
        }
        return true;
    }

    void Model::LoadMaterialTextures(const aiMaterial* mat, aiTextureType type, int storeIndex)
    {
        int textureCount = mat->GetTextureCount(type);
        if (textureCount > 0) {
            _materials[storeIndex].textures.insert({ (int)type, {} });
        }
        for (int i = 0; i < textureCount; i++) {
            aiString path;
            if (mat->GetTexture(type, i, &path) == aiReturn_SUCCESS) {
                _materials[storeIndex].textures[(int) type].emplace_back(path.C_Str());
            }
        }
    }

    void Model::ProcessNode(aiNode* node, const aiScene* scene)
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

    void Model::ProcessMesh(aiMesh* mesh, const aiScene* scene)
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

    void Model::LoadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
    {
        int textureCount = mat->GetTextureCount(type);
        DebugLog("  %d textures of type %s", textureCount, typeName.c_str());
        for (int i = 0; i < textureCount; i++) {
            aiString str1;
            aiReturn res1;
            res1 = mat->GetTexture(type, i, &str1);
            DebugLog("  %d, %d, %s, PATH, %s", i, res1, typeName.c_str(), str1.C_Str());
            aiString str2;
            aiReturn res2, res3, res4, res5, res6, res7, res8, res9, res10;
            float f3;
            int int4, int5, int6, int7, int8, int10;
            aiVector3D v3d9;
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
}