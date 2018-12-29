#ifndef ANDROID_IO_ASSET_H
#define ANDROID_IO_ASSET_H

#include <android_native_app_glue.h>
#include <vector>

using std::vector;

namespace AndroidNative
{
    template<typename T>
    void Open(const char *filePath, android_app *app, vector<T> &fileContent) {
        AAsset *file = AAssetManager_open(app->activity->assetManager, filePath,
                                          AASSET_MODE_BUFFER);
        size_t fileLength = AAsset_getLength(file);
        fileContent.resize(fileLength, 0);
        AAsset_read(file, fileContent.data(), fileLength);
        AAsset_close(file);
    }
}

#endif // ANDROID_IO_ASSET_H