#ifndef VULKAN_MODEL_H
#define VULKAN_MODEL_H

#include "device.h"
#include "buffer.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include <vector>
#include <string>

using glm::vec2;
using glm::vec3;
using glm::vec4;
using std::vector;
using std::string;
using Vulkan::Device;
using Vulkan::Buffer;

namespace Vulkan {

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

    class Model {
    public:
        struct Mesh {
            uint32_t vertexBase;
            uint32_t vertexCount;
            uint32_t indexBase;
            uint32_t indexCount;
        };

        typedef struct Dimension
        {
            vec3 min = vec3(FLT_MAX);
            vec3 max = vec3(-FLT_MAX);
            vec3 size;
        } Dimension;

        Model(const Device& device);
        Model(Model&& other);
        virtual ~Model();

        static const int DEFAULT_READ_FILE_FLAGS;

        bool RealFile(const string&       filename,
                      const VertexLayout& vertexLayout,
                      const Device&       device,
                      Command&            command,

                      ModelCreateInfo*    modelInfo = nullptr,
                      int                 readFileFlags = DEFAULT_READ_FILE_FLAGS);
    protected:
        vector<Mesh> subMeshes;
        VkDevice     device = nullptr;
        Buffer       vertices;
        Buffer       indices;
        uint32_t     indexCount  = 0;
        uint32_t     vertexCount = 0;
        Dimension    dimension   = {};
    };
}

#endif // VULKAN_MODEL_H