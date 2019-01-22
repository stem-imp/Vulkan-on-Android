#include "model_resource.h"
#include "../../log/log.h"
#include "../command.h"
#include "../vulkan_utility.h"
#include "../../androidutility/assetmanager/io_asset.hpp"

using Utility::Log;

namespace Vulkan
{
    ModelResource::ModelResource(const Device& device) : device(device), vertices(device), indices(device)
    {
        DebugLog("ModelResource()");
    }

    ModelResource::ModelResource(ModelResource&& other) : subMeshes(std::move(other.subMeshes)),
                                                          device(other.device),
                                                          vertices(std::move(other.vertices)),
                                                          indices(std::move(other.indices)),
                                                          indicesCount(other.indicesCount)
    {
        DebugLog("ModelResource(ModelResource&&)");
    }

    ModelResource::~ModelResource()
    {
        DebugLog("~ModelResource()");
    }

    void ModelResource::UploadToGPU(const Model& model, Command& command)
    {
        // Upload model.
        subMeshes.clear();
        subMeshes.resize(model.Submeshes().size());

        const vector<VertexLayout>& vertexLayouts = model.VertexLayouts();

        vector<float> vertexBuffer;
        vector<uint32_t> indexBuffer;
        uint32_t vertexCount = 0;
        uint32_t indexCount  = 0;
        size_t numMeshes = model.Submeshes().size();
        for (size_t i = 0; i < numMeshes; i++) {
            subMeshes[i] = {};
            subMeshes[i].vertexBase = vertexCount;
            subMeshes[i].indexBase = indexCount;

            const Model::Mesh& m = model.Submeshes()[i];
            size_t packSize = vertexLayouts[i].Stride() / sizeof(float);
            size_t vCount = m.vertexBuffer.size() / packSize;
            subMeshes[i].vertexCount = vCount;
            vertexCount += vCount;
            for (uint32_t j = 0; j < vCount; j++) {
                int indexInsidePack = 0;
                int vIndex = j * packSize;
                for (const auto& component : vertexLayouts[i].components)
                {
                    switch (component) {
                        case VERTEX_COMPONENT_POSITION:
                        case VERTEX_COMPONENT_NORMAL:
                        case VERTEX_COMPONENT_COLOR:
                        case VERTEX_COMPONENT_TANGENT:
                        case VERTEX_COMPONENT_BITANGENT:
                            vertexBuffer.push_back(m.vertexBuffer[vIndex + indexInsidePack]);
                            vertexBuffer.push_back(m.vertexBuffer[vIndex + indexInsidePack + 1]);
                            vertexBuffer.push_back(m.vertexBuffer[vIndex + indexInsidePack + 2]);
                            indexInsidePack += 3;
                            break;
                        case VERTEX_COMPONENT_UV:
                            vertexBuffer.push_back(m.vertexBuffer[vIndex + indexInsidePack]);
                            vertexBuffer.push_back(m.vertexBuffer[vIndex + indexInsidePack + 1]);
                            indexInsidePack += 2;
                            break;
                    };
                }
            }

            subMeshes[i].indexCount = m.indexBuffer.size();
            indexBuffer.insert(indexBuffer.end(), m.indexBuffer.begin(), m.indexBuffer.end());
            indexCount += subMeshes[i].indexCount;
        }

        uint32_t vBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(float);
        uint32_t iBufferSize = static_cast<uint32_t>(indexBuffer.size()) * sizeof(uint32_t);
        indicesCount = indexBuffer.size();

        Buffer vertexStaging(device), indexStaging(device);
        vertexStaging.BuildDefaultBuffer(vBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        indexStaging.BuildDefaultBuffer(iBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vertices.BuildDefaultBuffer(vBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        indices.BuildDefaultBuffer(iBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        vertexStaging.Map();
        indexStaging.Map();
        memcpy(vertexStaging.mapped, vertexBuffer.data(), vBufferSize);
        memcpy(indexStaging.mapped, indexBuffer.data(), iBufferSize);
        vertexStaging.Unmap();
        indexStaging.Unmap();


        VkDevice d = device.LogicalDevice();
        vector<VkCommandBuffer> copyCmds = Command::CreateAndBeginCommandBuffers(command.ShortLivedTransferPool(),
                                                                                 VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                                                 1,
                                                                                 VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                                                                                 d);
        VkCommandBuffer copyCmd = copyCmds[0];
        VkBufferCopy copyRegion = {};

        copyRegion.size = vBufferSize;
        vkCmdCopyBuffer(copyCmd, vertexStaging.GetBuffer(), vertices.GetBuffer(), 1, &copyRegion);

        copyRegion.size = iBufferSize;
        vkCmdCopyBuffer(copyCmd, indexStaging.GetBuffer(), indices.GetBuffer(), 1, &copyRegion);

        Command::EndAndSubmitCommandBuffer(copyCmd, device.FamilyQueues().transfer.queue, command.ShortLivedTransferPool(), d);
    }
}
