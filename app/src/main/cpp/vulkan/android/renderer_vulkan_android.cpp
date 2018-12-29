#ifdef __ANDROID__

#include <android_native_app_glue.h>
#include "../../renderer.h"

Renderer::Renderer(void* app, uint32_t screenWidth, uint32_t screenHeight)
{
    window            = ((android_app*)app)->window;
    screenSize.width  = screenWidth;
    screenSize.height = screenHeight;
    currentFrameIndex = 0;
}

#endif