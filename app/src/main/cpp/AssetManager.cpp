#include "AssetManager.h"

#include <sys/stat.h>
#include <dirent.h>

#include "log.h"
#include "exceptionUtils.h"

#define ASSET_MANAGER_TAG "PT_ASSET_MANAGER"

AssetManager::AssetManager() {

}

void AssetManager::initialize(AAssetManager* nativeManager) {

    if (this->initialized == 1)
        return;

    this->nativeManager = nativeManager;

    this->initialized = 1;
}

void AssetManager::finalize() {

    if (this->initialized == 0)
        return;

    this->initialized = 0;
}

string AssetManager::loadTextAsset(string assertName) {

    AAsset* asset = AAssetManager_open(nativeManager, assertName.c_str(), AASSET_MODE_BUFFER);
    if (!asset)
        return "";

    const void* data = AAsset_getBuffer(asset);
    off64_t size = AAsset_getLength64(asset);

    string result = string(static_cast<const char*>(data), (unsigned int) size);

    AAsset_close(asset);

    return result;
}

GLuint AssetManager::loadTextureAsset(string assertName) {

    AAsset* asset = AAssetManager_open(nativeManager, assertName.c_str(), AASSET_MODE_BUFFER);
    if (!asset)
        return 0;

    auto data = (const unsigned char*)AAsset_getBuffer(asset);
    off64_t size = AAsset_getLength64(asset);

    GLuint textureID = 0;

    if (size > 54) {
        unsigned int dataPos    = 54;

        unsigned int width      = *(unsigned int*)&(data[0x12]);
        unsigned int height     = *(unsigned int*)&(data[0x16]);

        if (size == 54 + width * height * 3) {

            auto pixels = data + dataPos;

            glGenTextures(1, &textureID);

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE,
                         pixels);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glGenerateMipmap(GL_TEXTURE_2D);
        }
    }

    AAsset_close(asset);

    return textureID;
}