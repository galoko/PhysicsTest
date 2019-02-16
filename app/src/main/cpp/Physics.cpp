#include "Physics.h"

#include "log.h"
#include "exceptionUtils.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define PHYSICS_TAG "PT_PHYSICS"

Physics::Physics() {

}

void Physics::initialize() {

    if (this->initialized == 1)
        return;

    this->gravity = normalize(vec3(1, 1, 1)) * -9.8f;

    mat3 rotation = rotate(mat4(1.f), radians(50.0f), normalize(vec3(1, 1, 1)));

    this->cube = new Cube({ 0, 0, 0 }, rotation, { 1, 1, 1 });
    this->walls = new Cube({ 0, 0, 0 }, mat4(1.0f), { 4.3f, 4.3f, 4.3f });

    this->cubePhysics = new PhysicsData(this->cube, this->walls, 1.0f);

    this->initialized = 1;
}

const Cube* Physics::getCube() {
    return cube;
}

const Cube* Physics::getWalls() {
    return walls;
}

void Physics::finalize() {

    if (this->initialized == 0)
        return;

    this->initialized = 0;
}

void Physics::step(double dt) {

    unsigned int SUB_STEP_COUNT = 1;
    unsigned int DEBUG_SPEED = 1;
    double subDt = dt / SUB_STEP_COUNT;

    for (unsigned int debugCounter = 0; debugCounter < DEBUG_SPEED; debugCounter++)
        for (unsigned int counter = 0; counter < SUB_STEP_COUNT; counter++)
            subStep(subDt);
}

void Physics::subStep(double dt) {

    this->cubePhysics->applyGravity(gravity, dt);
    this->cubePhysics->processCollisions();
    this->cubePhysics->integrate(dt);
    // this->cubePhysics->applyDamping(dt, 0.01);
}

// PhysicsData

PhysicsData::PhysicsData(Cube* cube, Cube* walls, float mass) {

    this->cube = cube;
    this->walls = walls;

    this->linearVelocity = { 0, 0, 0 };
    this->angularVelocity = { 0, 0, 0 };

    this->invMass = 1.0f / mass;

    vec3 size = cube->getSize();
    vec3 sizeSq = size * size;

    vec3 inertia = {
        mass / 12.0f * (sizeSq.y + sizeSq.z),
        mass / 12.0f * (sizeSq.x + sizeSq.z),
        mass / 12.0f * (sizeSq.x + sizeSq.y)
    };

    this->localInvInertiaTensor = scale(mat4(1.0f), 1.0f / inertia);

    updateInertiaTensor();
}

void PhysicsData::updateInertiaTensor() {

    mat3 rotation = cube->getRotation();

    this->worldInvInertiaTensor = rotation * this->localInvInertiaTensor * transpose(rotation);
}

void PhysicsData::applyGravity(vec3 gravity, double dt) {
    this->linearVelocity += gravity * (float)dt;
}

void PhysicsData::applyPseudoImpulse(vec3 impulse, vec3 localPoint) {
    this->cube->position += this->invMass * impulse;

    vec3 angularVelocityDelta = this->worldInvInertiaTensor * cross(localPoint, impulse);

    float angularSpeed = length(angularVelocityDelta);
    if (angularSpeed > 10e-5) {
        vec3 axis = angularVelocityDelta / angularSpeed;

        this->cube->rotation = rotate(mat4(this->cube->rotation), angularSpeed, axis);

        this->updateInertiaTensor();
    }
}

void PhysicsData::applyImpulse(vec3 impulse, vec3 localPoint) {
    this->linearVelocity += this->invMass * impulse;
    this->angularVelocity += this->worldInvInertiaTensor * cross(localPoint, impulse);
}

bool isZeroVec(vec3 v) {
    float manhattanDist = fabs(v.x) + fabs(v.y) + fabs(v.z);
    return manhattanDist <= 10e-5;
}

void PhysicsData::processCollisions() {

    vec3 leftBottomNear = walls->getLeftBottomNear();
    vec3 rightTopFar = walls->getRightTopFar();

    vec3 error, errorPointSum;
    unsigned int errorPointCount;

    const unsigned int normalCount = 3;
    const vec3 normals[normalCount] = {
        { 1, 0, 0 },
        { 0, 1, 0 },
        { 0, 0, 1 }
    };

    const vec3* points = this->cube->getPoints();
    for (unsigned int normalIndex = 0; normalIndex < normalCount; normalIndex++) {

        vec3 normal = normals[normalIndex];

        error = {0, 0, 0};
        errorPointSum = {0, 0, 0};
        errorPointCount = 0;

        for (unsigned int pointIndex = 0; pointIndex < Cube::POINTS_COUNT; pointIndex++) {

            vec3 point = points[pointIndex];


            vec3 pointError = normal *
                (
                    min(dot(point - leftBottomNear, normal), 0.0f) +
                    max(dot(point - rightTopFar, normal), 0.0f)
                );

            if (!isZeroVec(pointError)) {
                error += pointError;
                errorPointSum += point;
                errorPointCount++;
            }
        }

        if (errorPointCount > 0) {

            vec3 errorNormal = normal;
            if (!isZeroVec(errorNormal)) {
                vec3 errorPoint = errorPointSum / (float) errorPointCount;

                vec3 localErrorPoint, linearVelocityAtPoint, velocityErrorCorrection, temp, impulse;

                // normal error correction
                localErrorPoint = errorPoint - this->cube->getPosition();

                linearVelocityAtPoint = this->linearVelocity + cross(this->angularVelocity, localErrorPoint);
                velocityErrorCorrection = -(error * 0.25f / (float)errorPointCount);
                temp = this->worldInvInertiaTensor * cross(localErrorPoint, errorNormal);

                impulse = velocityErrorCorrection /
                          (this->invMass + dot(errorNormal, cross(temp, localErrorPoint)));
                applyPseudoImpulse(impulse, localErrorPoint);

                // normal impulse
                localErrorPoint = errorPoint - this->cube->getPosition();

                linearVelocityAtPoint = this->linearVelocity + cross(this->angularVelocity, localErrorPoint);
                velocityErrorCorrection =
                        (-(1.0f + RESTITUTION)) * (dot(linearVelocityAtPoint, errorNormal) * errorNormal);
                temp = this->worldInvInertiaTensor * cross(localErrorPoint, errorNormal);

                impulse = (velocityErrorCorrection) /
                          (this->invMass + dot(errorNormal, cross(temp, localErrorPoint)));
                applyImpulse(impulse, localErrorPoint);

                // tangent impulse
                linearVelocityAtPoint = this->linearVelocity + cross(this->angularVelocity, localErrorPoint);
                vec3 c = cross(errorNormal, linearVelocityAtPoint);
                if (!isZeroVec(c)) {
                    vec3 errorTangent = normalize(cross(c, errorNormal));
                    if (!isZeroVec(errorTangent)) {
                        velocityErrorCorrection =
                                (-(0.0f + FRICTION)) * (dot(linearVelocityAtPoint, errorTangent) * errorTangent);
                        temp = this->worldInvInertiaTensor * cross(localErrorPoint, errorTangent);
                        impulse = velocityErrorCorrection /
                                  (this->invMass + dot(errorTangent, cross(temp, localErrorPoint)));
                        applyImpulse(impulse, localErrorPoint);
                    }
                }
            }
        }

        // print_log(ANDROID_LOG_INFO, PHYSICS_TAG, "points: %d", errorPointCount);
    }
}

void PhysicsData::integrate(double dt) {

    this->cube->position += this->linearVelocity * (float)dt;

    float angularSpeed = length(this->angularVelocity);
    if (angularSpeed > 10e-5) {
        vec3 axis = this->angularVelocity / angularSpeed;

        this->cube->rotation = rotate(mat4(this->cube->rotation), angularSpeed * (float) dt, axis);

        this->updateInertiaTensor();
    }

    this->cube->calcPoints();
}

void PhysicsData::applyDamping(double dt, float damping) {

    float m = 1.0f - (float)dt * damping;

    this->linearVelocity *= m;
    this->angularVelocity *= m;
}

// Cube

Cube::Cube(vec3 position, mat3 rotation, vec3 size) :
    position(position), rotation(rotation), size(size) {
    calcPoints();
}

void Cube::calcPoints() {

    vec3 CUBE_POINTS[8] = {
            {  0.50f,  0.50f,  0.50f },
            {  0.50f, -0.50f,  0.50f },
            {  0.50f,  0.50f, -0.50f },
            {  0.50f, -0.50f, -0.50f },

            { -0.50f,  0.50f,  0.50f },
            { -0.50f, -0.50f,  0.50f },
            { -0.50f,  0.50f, -0.50f },
            { -0.50f, -0.50f, -0.50f },
    };

    for (unsigned int pointIndex = 0; pointIndex < POINTS_COUNT; pointIndex++) {
        points[pointIndex] = (rotation * (CUBE_POINTS[pointIndex] * size)) + position;
    }
}

const vec3 Cube::getPosition() const {
    return this->position;
}

const mat3 Cube::getRotation() const {
    return this->rotation;
}

const vec3 Cube::getSize() const {
    return this->size;
}

const vec3* Cube::getPoints() const {
    return this->points;
}

const vec3 Cube::getLeftBottomNear() const {
    return position - size * 0.5f;
}

const vec3 Cube::getRightTopFar() const {
    return position + size * 0.5f;
}