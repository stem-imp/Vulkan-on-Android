#ifndef VULKAN_MODEL_RESOURCE_H
#define VULKAN_MODEL_RESOURCE_H

#include "../device.h"
#include "../buffer.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "model.h"
#include <vector>
#include <string>

using glm::vec2;
using glm::vec3;
using glm::vec4;
using std::vector;
using std::string;
using Vulkan::Device;
using Vulkan::Buffer;

namespace Vulkan
{
    class ModelResource {
    public:
        struct Mesh {
            uint32_t vertexBase;
            uint32_t vertexCount;
            uint32_t indexBase;
            uint32_t indexCount;
        };

        ModelResource(const Device& device);
        ModelResource(ModelResource&& other);
        virtual ~ModelResource();

        void UploadToGPU(const Model& model, Command& command);

        const Buffer& VertexBuffer() const { return vertices; }
        const Buffer& IndexBuffer() const { return indices; }
        uint32_t IndicesCount() { return indicesCount; }
    protected:
        vector<Mesh>  subMeshes;
        const Device& device;
        Buffer        vertices;
        Buffer        indices;
        uint32_t      indicesCount;
    };
}

#endif // VULKAN_MODEL_RESOURCE_H