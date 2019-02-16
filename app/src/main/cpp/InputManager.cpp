#include "InputManager.h"

#include <glm/gtc/matrix_transform.hpp>

#include "log.h"
#include "exceptionUtils.h"

#include "Physics.h"

#define INPUT_MANAGER_TAG "PT_INPUT_MANAGER"

InputManager::InputManager() {

}

void InputManager::initialize() {

    if (this->initialized == 1)
        return;

    // init accelerometer sensor

    ASensorManager* sensorManager = ASensorManager_getInstance();
    my_assert(sensorManager != nullptr);

    const ASensor* accelerometer = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_ACCELEROMETER);
    my_assert(accelerometer != nullptr);

    ALooper* looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    my_assert(looper != nullptr);

    accelerometerEventQueue = ASensorManager_createEventQueue(sensorManager, looper, 3, nullptr, nullptr);
    my_assert(accelerometerEventQueue != nullptr);

    auto status = ASensorEventQueue_enableSensor(accelerometerEventQueue, accelerometer);
    my_assert(status >= 0);

    status = ASensorEventQueue_setEventRate(accelerometerEventQueue, accelerometer, SENSOR_REFRESH_PERIOD_US);
    my_assert(status >= 0);

    this->sensorRotation = rotate(mat4(1.0f), radians(-90.0f), vec3(1, 0, 0));

    this->initialized = 1;

    print_log(ANDROID_LOG_INFO, INPUT_MANAGER_TAG, "Input manager is initialized");
}

void InputManager::finalize() {

    if (this->initialized == 0)
        return;

    this->initialized = 0;

    print_log(ANDROID_LOG_INFO, INPUT_MANAGER_TAG, "Input manager is finalized");
}

void InputManager::applyUserInput() {

    ALooper_pollAll(0, nullptr, nullptr, nullptr);
    ASensorEvent event;
    float a = SENSOR_FILTER_ALPHA;
    while (ASensorEventQueue_getEvents(accelerometerEventQueue, &event, 1) > 0) {
        vec3 eventAcceleration = vec3(event.acceleration.x, event.acceleration.y, event.acceleration.z);

        sensorDataFilter = a * eventAcceleration + (1.0f - a) * sensorDataFilter;
        sensorDataFilter = eventAcceleration;
    }

    vec3 rotatedVec = sensorRotation * sensorDataFilter;

    rotatedVec.x = fabs(rotatedVec.x) > 5 ? sign(rotatedVec.x) * 9.8f : 0;
    rotatedVec.y = fabs(rotatedVec.y) > 5 ? sign(rotatedVec.y) * 9.8f : 0;
    rotatedVec.z = fabs(rotatedVec.z) > 5 ? sign(rotatedVec.z) * 9.8f : 0;

    print_log(ANDROID_LOG_INFO, INPUT_MANAGER_TAG, "sensor %f %f %f",
              rotatedVec.x, rotatedVec.y, rotatedVec.z);

    Physics::getInstance().setGravity(rotatedVec);
}