#ifndef PHYSICSTEST_ENGINE_H
#define PHYSICSTEST_ENGINE_H

#include <pthread.h>

#include <android/asset_manager.h>
#include <android/native_window.h>

#include "readerwriterqueue.h"

#include <string>

using namespace moodycamel;
using namespace std;

class Engine {
public:
    static Engine& getInstance() {
        static Engine instance;

        return instance;
    }

    Engine(Engine const&)          = delete;
    void operator=(Engine const&)  = delete;
private:
    int initialized;

    enum EventMessage {
        Initialize,
        Finalize,
        Start,
        Stop,
        SetOutputWindow
    };

    struct EngineEvent {
        EventMessage message;
        void* param;
    };

    struct InitStruct {
        AAssetManager* nativeAssetManager;
        string externalFilesDir;
    };

    BlockingReaderWriterQueue<EngineEvent> eventQueue;
    pthread_t thread;

    const double FPS = 60.0;
    const double FRAME_TIME = 1.0 / FPS;
    const int MAX_LAG_IN_FRAMES = 0;

    bool started, finalized;
    double nextTickTime;

    void setNextTickTime();

    static void* thread_entrypoint(void* opaque);
    void threadLoop();

    void pushEvent(EventMessage message, void *param = nullptr);
    void processEventsUntilNextTick();
    void processEvent(EngineEvent& event);
public:
    Engine();

    // all these methods should be called from single thread

    void initialize(AAssetManager* nativeAssetManager, string externalFilesDir);
    void finalize();

    void start();
    void stop();

    void setOutputWindow(ANativeWindow* window);
};

#endif //PHYSICSTESTENGINE_H
