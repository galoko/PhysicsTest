#ifndef PHYSICSTEST_ASSET_MANAGER_H
#define PHYSICSTEST_ASSET_MANAGER_H

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <GLES2/gl2.h>

#include <string>

using namespace std;

class AssetManager {
public:
    static AssetManager& getInstance() {
        static AssetManager instance;

        return instance;
    }

    AssetManager(AssetManager const&) = delete;
    void operator=(AssetManager const&)  = delete;
private:
    AssetManager();

    int initialized;

    AAssetManager* nativeManager;

    string externalFilesDir;
public:
    void initialize(AAssetManager* nativeManager, string externalFilesDir);
    void finalize();

    string loadTextAsset(string assertName);
    GLuint loadTextureAsset(string assertName);

    bool loadExternalBinaryFile(string fileName, void* dest, unsigned int size);
    void saveExternalBinaryFile(string fileName, void* src, unsigned int size);
};

#endif //PHYSICSTEST_ASSET_MANAGER_H
