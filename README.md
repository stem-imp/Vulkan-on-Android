# Vulkan on Android

## Dependencies
* GLM 0.9.9.3
* Assimp 4.1.0
* stb_image v2.19

## Scenes
### An Empty Window
https://medium.com/@jlwu.will/android-vulkan-3-rendering-loop-9ed6a041b9cb
* NDK
* layer and extension enumeration
* instance creation
* surface creation
* device selection and creation
* swapchain creation
* render pass creation
* framebuffer creation
* command buffer/pool creation
* synchronization with semaphores and fences

### Model Rendering
https://medium.com/@jlwu.will/android-vulkan-8-model-loading-eb25902645d8
* Assimp building
* shader compilation
* desriptor set layout/desriptor set/descriptor pool creation
* graphics pipeline creation
* color/depth image creation
* image layout transition
* uniform buffer creation

### MSAA
https://medium.com/@jlwu.will/vulkan-on-android-12-vr-stereo-rendering-part-3-implementation-f02ea16fe1a0
* multisample anti-aliasing

### VR Stereo Rendering
https://medium.com/@jlwu.will/vulkan-on-android-12-vr-stereo-rendering-part-4-visual-comfort-9f12ec18fe32

* depth perception
* off-axis viewing frustum
* multiple render pass
* multiple descriptor set layout and descriptor set
* multiple graphics pipeline
* barrel distortion
* dynamic uniform buffer
* synchronization with dependent images


