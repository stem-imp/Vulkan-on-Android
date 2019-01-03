#ifndef VULKAN_MODEL_H
#define VULKAN_MODEL_H

#include "../../log/log.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include <vector>
#include <string>
#include <unordered_map>

using Utility::Log;
using glm::vec2;
using glm::vec3;
using glm::mat4;
using std::vector;
using std::string;
using std::unordered_map;

namespace Vulkan
{
    typedef enum Component {
        VERTEX_COMPONENT_POSITION,
        VERTEX_COMPONENT_NORMAL,
        VERTEX_COMPONENT_COLOR,
        VERTEX_COMPONENT_UV,
        VERTEX_COMPONENT_TANGENT,
        VERTEX_COMPONENT_BITANGENT,
    } VertexComponent;

    struct VertexLayout {
    public:
        vector<Component> components;

        VertexLayout() { DebugLog("VertexLayout"); }
        VertexLayout(vector<Component> other)
        {
            components = std::move(other);
        }

        template <typename T>
        uint32_t Stride()
        {
            uint32_t res = 0;
            for (auto& component : components)
            {
                switch (component)
                {
                    case VERTEX_COMPONENT_UV:
                        res += 2 * sizeof(T);
                        break;
                    case VERTEX_COMPONENT_COLOR:
                        res += 4 * sizeof(T);
                    default:
                        res += 3 * sizeof(T);
                        break;
                }
            }
            return res;
        }
    };

    typedef struct ModelCreateInfo {
        vec3 scale;
        vec2 uvScale;

        ModelCreateInfo() {};

        ModelCreateInfo(vec3 scale, vec2 uvScale)
        {
            this->scale = scale;
            this->uvScale = uvScale;
        }

        ModelCreateInfo(float scale, float uvScale)
        {
            this->scale = vec3(scale);
            this->uvScale = vec2(uvScale);
        }

    } ModelCreateInfo;

    class Model
    {
    public:
        Model() { DebugLog("Model()"); }
        ~Model() { DebugLog("~Model()"); }
        static const unsigned int DEFAULT_READ_FILE_FLAGS;

        bool ReadFile(const string& filePath, const string& filename, unsigned int readFileFlags = DEFAULT_READ_FILE_FLAGS, ModelCreateInfo* modelInfo = nullptr);

        const aiScene* scene;

        typedef struct Mesh {
            vector<float> vertexBuffer;
            vector<uint32_t> indexBuffer;
        } Mesh;

        typedef struct Dimension
        {
            vec3 min = vec3(FLT_MAX);
            vec3 max = vec3(-FLT_MAX);
            vec3 size;
        } Dimension;

        const vector<Mesh>& Submeshes() const { return _subMeshes; }
        const vector<VertexLayout>& VertexLayouts() const { return _vertexLayouts; }

        typedef struct Material
        {
            Material() {}
            Material(Material&& other)
            {
                textures = std::move(other.textures);
            }

            unordered_map<int, vector<string>> textures;
        } Material;
        const vector<Material>& Materials() const { return _materials; }
    private:
        void LoadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName);

        void ProcessNode(aiNode* node, const aiScene* scene);
        void ProcessMesh(aiMesh* mesh, const aiScene* scene);
        void LoadMaterialTextures(const aiMaterial* mat, aiTextureType type, int storeIndex);

        vector<Mesh>         _subMeshes;
        vector<VertexLayout> _vertexLayouts;
        Dimension            _dimension = {};

        vector<int>      _materialIndices;
        vector<Material> _materials;

        mat4 _model      = mat4(1);
        mat4 _view       = mat4(1);
        mat4 _projection = mat4(1);
    };
}

#endif // VULKAN_MODEL_H