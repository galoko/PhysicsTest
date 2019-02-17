#include <jni.h>

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "coffeecatch.h"
#include "coffeejni.h"

#include "exceptionUtils.h"

#include "Engine.h"

extern "C" JNIEXPORT void JNICALL Java_com_example_physicstest_JNIHandler_init(
        JNIEnv *env, jclass /*this*/, jobject assetManager) {
    try {
        COFFEE_TRY() {
            AAssetManager* nativeAssetManager = AAssetManager_fromJava(env, assetManager);

            Engine::getInstance().initialize(nativeAssetManager);
        } COFFEE_CATCH() {
            coffeecatch_throw_exception(env);
        } COFFEE_END();
    } catch(...) {
        swallow_cpp_exception_and_throw_java(env);
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_example_physicstest_JNIHandler_destroy(
        JNIEnv *env, jclass /*this*/) {
    try {
        COFFEE_TRY() {
            Engine::getInstance().finalize();
        } COFFEE_CATCH() {
            coffeecatch_throw_exception(env);
        } COFFEE_END();
    } catch(...) {
        swallow_cpp_exception_and_throw_java(env);
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_example_physicstest_JNIHandler_start(
        JNIEnv *env, jclass /*this*/) {
    try {
        COFFEE_TRY() {
            Engine::getInstance().start();
        } COFFEE_CATCH() {
            coffeecatch_throw_exception(env);
        } COFFEE_END();
    } catch(...) {
        swallow_cpp_exception_and_throw_java(env);
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_example_physicstest_JNIHandler_stop(
        JNIEnv *env, jclass /*this*/) {
    try {
        COFFEE_TRY() {
            Engine::getInstance().stop();
        } COFFEE_CATCH() {
            coffeecatch_throw_exception(env);
        } COFFEE_END();
    } catch(...) {
        swallow_cpp_exception_and_throw_java(env);
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_example_physicstest_JNIHandler_setOutputSurface(
        JNIEnv *env, jclass /*this*/, jobject surface) {
    try {
        COFFEE_TRY() {
            ANativeWindow *window = surface ? ANativeWindow_fromSurface(env, surface) : nullptr;
            Engine::getInstance().setOutputWindow(window);
        } COFFEE_CATCH() {
            coffeecatch_throw_exception(env);
        } COFFEE_END();
    } catch(...) {
        swallow_cpp_exception_and_throw_java(env);
    }
}