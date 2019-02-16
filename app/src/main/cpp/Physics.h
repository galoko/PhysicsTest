#ifndef PHYSICSTEST_PHYSICS_H
#define PHYSICSTEST_PHYSICS_H

#include <glm/glm.hpp>

using namespace glm;

class Cube {
    friend class Physics;
    friend class PhysicsData;
private:
    vec3 position;
    mat3 rotation;
    vec3 size;

    vec3 points[8];

    // physics

    Cube(vec3 position, mat3 rotation, vec3 size);

    void calcPoints();
public:
    static const unsigned int POINTS_COUNT = 8;

    const vec3 getPosition() const;
    const mat3 getRotation() const;
    const vec3 getSize() const;

    const vec3* getPoints() const;

    const vec3 getLeftBottomNear() const;
    const vec3 getRightTopFar() const;
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
    bool checkCollision(vec3 point, vec3 leftBottomNear, vec3 rightTopFar, vec3& normal);
public:
    PhysicsData(Cube* cube, Cube* walls, float mass);

    void applyGravity(vec3 gravity, double dt);

    void applyPseudoImpulse(vec3 impulse, vec3 localPoint);
    void applyImpulse(vec3 impulse, vec3 localPoint);

    void processCollisions();
    void integrate(double dt);

    void applyDamping(double dt, float damping);
};

class Physics {
public:
    static Physics& getInstance() {
        static Physics instance;

        return instance;
    }

    Physics(Physics const&) = delete;
    void operator=(Physics const&)  = delete;
private:
    Physics();

    int initialized;

    vec3 gravity;

    Cube *cube, *walls;
    PhysicsData *cubePhysics;

    void subStep(double dt);
public:
    void initialize();
    void finalize();

    const Cube* getCube();
    const Cube* getWalls();

    void setGravity(vec3 gravity);

    void step(double dt);
};

#endif //PHYSICSTEST_PHYSICS_H
