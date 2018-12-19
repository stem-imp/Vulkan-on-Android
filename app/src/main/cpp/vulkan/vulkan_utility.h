#ifndef VULKAN_UTILITY_H
#define VULKAN_UTILITY_H

#include "../log/log.h"
#ifdef __ANDROID__
#include "vulkan_wrapper.h"
#endif
#include <vector>

using Utility::Log;

#define VK_CHECK_RESULT(func)											                \
{																		                \
    VkResult res = (func);                                                              \
    if (res != VK_SUCCESS) {                                                             \
        Log::Error("%s: code[%d], file[%s], line[%d]", #func, res, __FILE__, __LINE__); \
        assert(false);                                                                   \
    }                                                                                   \
}

uint32_t GetAPIVersion();
void AppendInstanceExtension(std::vector<const char*>& instanceExtensionNames);

#endif // VULKAN_UTILITY_H