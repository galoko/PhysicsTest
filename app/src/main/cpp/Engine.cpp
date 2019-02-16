#include "Engine.h"

#include <unistd.h>

#include "log.h"
#include "exceptionUtils.h"

#include "AssetManager.h"
#include "Physics.h"
#include "Render.h"
#include "InputManager.h"

extern "C" {
#include "generalUtils.h"
}

#define ENGINE_TAG "PT_ENGINE"

Engine::Engine() : eventQueue(30) {

}

void Engine::initialize(AAssetManager* nativeAssetManager) {

    if (this->initialized == 1)
        return;

    this->finalized = false;
    this->started = false;

    pthread_check_error(pthread_create(&thread, nullptr, thread_entrypoint, nullptr));

    auto initStruct = new InitStruct();
    initStruct->nativeAssetManager = nativeAssetManager;

    pushEvent(Initialize, (void*)initStruct);

    this->initialized = 1;

    print_log(ANDROID_LOG_INFO, ENGINE_TAG, "Engine is initialized");
}

void Engine::finalize() {

    if (this->initialized == 0)
        return;

    pushEvent(Finalize);

    pthread_check_error(pthread_join(thread, nullptr));

    this->initialized = 0;

    print_log(ANDROID_LOG_INFO, ENGINE_TAG, "Engine is finalized");
}

void Engine::start() {
    pushEvent(Start);
}

void Engine::stop() {
    pushEvent(Stop);
}

void Engine::setOutputWindow(ANativeWindow* window) {
    pushEvent(SetOutputWindow, (void*)window);
}

// thread

void* Engine::thread_entrypoint(void* opaque) {

    Engine::getInstance().threadLoop();
    return nullptr;
}

void Engine::threadLoop() {

    while (true) {
        processEventsUntilNextTick();
        if (finalized)
            break;

        my_assert(started);

        InputManager::getInstance().applyUserInput();
        Physics::getInstance().step(FRAME_TIME);
        Render::getInstance().draw();

        setNextTickTime();
    }
}

void Engine::setNextTickTime() {

    nextTickTime += FRAME_TIME;

    double now = getTime();
    if (now > nextTickTime + FRAME_TIME * MAX_LAG_IN_FRAMES)
        nextTickTime = now;
}

// messages

void Engine::pushEvent(EventMessage message, void *param) {

    EngineEvent event = { message, param };
    eventQueue.enqueue(event);
}

void Engine::processEventsUntilNextTick() {

    while (!finalized) {

        int64_t delta;
        if (started) {
            double now = getTime();
            delta = timeToUSec(nextTickTime - now);
            // have no time
            if (delta < 1)
                break;
        } else
            delta = -1;

        EngineEvent event;
        // expired
        if (!eventQueue.wait_dequeue_timed(event, delta))
            break;

        processEvent(event);
    }
}

void Engine::processEvent(EngineEvent& event) {

    print_log(ANDROID_LOG_INFO, ENGINE_TAG, "Event: %d", event.message);

    switch (event.message) {
        case Initialize: {
            nice(-20);

            InitStruct* initStruct = (InitStruct*)event.param;

            AssetManager::getInstance().initialize(initStruct->nativeAssetManager);
            Physics::getInstance().initialize();
            Render::getInstance().initialize();
            InputManager::getInstance().initialize();

            delete initStruct;

            break;
        }
        case Finalize:
            InputManager::getInstance().finalize();
            Render::getInstance().finalize();
            Physics::getInstance().finalize();
            AssetManager::getInstance().finalize();
            finalized = true;
            break;
        case Start:
            nextTickTime = getTime();
            started = true;
            break;
        case Stop:
            started = false;
            break;
        case SetOutputWindow:
            Render::getInstance().setOutputWindow((ANativeWindow*)event.param);
            break;
        default:
            my_assert(false);
            break;
    }
}