#include "../../renderer.h"

Renderer::Renderer(void* genericWindow, uint32_t screenWidth, uint32_t screenHeight)
{
    window            = (ANativeWindow*)genericWindow;
    screenSize.width  = screenWidth;
    screenSize.height = screenHeight;
    currentFrameIndex = 0;
}