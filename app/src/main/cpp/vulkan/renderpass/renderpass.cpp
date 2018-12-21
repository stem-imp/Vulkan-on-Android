#include "../../log/log.h"
#include "renderpass.h"

using Utility::Log;
using Vulkan::RenderPass;

RenderPass::~RenderPass()
{
    DebugLog("~RenderPass()");
}

void RenderPass::CreateRenderPass()
{
    CreateRenderPassImpl();
};
