#ifndef PHYSICSTEST_PHYSICS_H
#define PHYSICSTEST_PHYSICS_H

#include <glm/glm.hpp>

#include <string>

using namespace glm;
using namespace std;

struct SerializedCube {
    vec3 position;
    mat3 rotation;
};

class Cube {
private:
    vec3 position;
    mat3 rotation;
    vec3 size;

    vec3 points[8];

    // physics
    void calcPoints();
public:
    Cube(vec3 position, mat3 rotation, vec3 size);

    const vec3 getPosition() const;
    const mat3 getRotation() const;
    const vec3 getSize() const;

    static const unsigned int POINTS_COUNT = 8;
    const vec3* getPoints() const;

    const vec3 getLeftBottomNear() const;
    const vec3 getRightTopFar() const;

    void integrateTransforms(vec3 positionDelta, vec3 rotationDelta);

    void loadFromState(SerializedCube state);
    void saveToState(SerializedCube* state);
};

struct SerializedPhysics {
    vec3 linearVelocity, angularVelocity;
};

class PhysicsData {
private:
    const float RESTITUTION = 0.0f;
    const float FRICTION = 1.0f;

    Cube *cube, *walls;

    vec3 linearVelocity, angularVelocity;

    float invMass;
    mat3 localInvInertiaTensor, worldInvInertiaTensor;

    void updateInertiaTensor();

    void integrateTransforms(vec3 positionDelta, vec3 rotationDelta);
public:
    PhysicsData(Cube* cube, Cube* walls, float mass);

    void applyGravity(vec3 gravity, double dt);

    void applyPseudoImpulse(vec3 impulse, vec3 localPoint);
    void applyImpulse(vec3 impulse, vec3 localPoint);

    void processCollisions();
    void integrate(double dt);

    void applyDamping(double dt, float damping);

    void loadFromState(SerializedPhysics state);
    void saveToState(SerializedPhysics* state);
};

class Physics {
public:
    static Physics& getInstance() {
        static Physics instance;

        return instance;
    }

    struct SerializedScene {
        vec3 gravity;
        SerializedCube cubeState;
        SerializedPhysics cubePhysicsState;
    };

    Physics(Physics const&) = delete;
    void operator=(Physics const&)  = delete;
private:
    Physics();

    int initialized;

    vec3 gravity;

    Cube *cube, *walls;
    PhysicsData *cubePhysics;

    void subStep(double dt);

    const string STATE_FILE_NAME = "state.bin";

    void loadSimulationState();
    void saveSimulationState();
public:
    void initialize();
    void finalize();

    const Cube* getCube();
    const Cube* getWalls();

    void setGravity(vec3 gravity);

    void step(double dt);
};

#endif //PHYSICSTEST_PHYSICS_H
