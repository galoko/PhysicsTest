#include "Physics.h"

#include "log.h"
#include "exceptionUtils.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "AssetManager.h"

#define PHYSICS_TAG "PT_PHYSICS"

Physics::Physics() {

}

void Physics::initialize() {

    if (this->initialized == 1)
        return;

    this->gravity = normalize(vec3(0, 0, -1)) * 9.8f;

    mat3 rotation = rotate(mat4(1.f), radians(0.0f), normalize(vec3(0, 1, 0)));

    this->cube = new Cube({ 0, 0, 0 }, rotation, { 1, 1, 1 });
    this->walls = new Cube({ 0, 0, 0 }, mat4(1.0f), { 4.3f, 4.3f, 4.3f });

    this->cubePhysics = new PhysicsData(this->cube, this->walls, 1.0f);

    loadSimulationState();

    this->initialized = 1;
}

void Physics::loadSimulationState() {

    SerializedScene scene;
    if (!AssetManager::getInstance().loadExternalBinaryFile(STATE_FILE_NAME, &scene, sizeof(scene)))
        return;

    this->cube->loadFromState(scene.cubeState);
    this->cubePhysics->loadFromState(scene.cubePhysicsState);

    setGravity(scene.gravity);
}

void Physics::saveSimulationState() {

    SerializedScene scene = { };

    scene.gravity = getGravity();

    this->cube->saveToState(&scene.cubeState);
    this->cubePhysics->saveToState(&scene.cubePhysicsState);

    AssetManager::getInstance().saveExternalBinaryFile(STATE_FILE_NAME, &scene, sizeof(scene));
}

const Cube* Physics::getCube() {
    return cube;
}

const Cube* Physics::getWalls() {
    return walls;
}

void Physics::setGravity(vec3 gravity) {
    this->gravity = gravity;
}

vec3 Physics::getGravity() {
    return this->gravity;
}

void Physics::finalize() {

    if (this->initialized == 0)
        return;

    saveSimulationState();

    delete this->cubePhysics;
    this->cubePhysics = nullptr;

    delete this->cube;
    this->cube = nullptr;

    delete this->walls;
    this->walls = nullptr;

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
    // this->cubePhysics->applyDamping(dt, 0.1);
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

    // inertia *= 0.5f;

    this->localInvInertiaTensor = scale(mat4(1.0f), 1.0f / inertia);

    updateInertiaTensor();
}

void PhysicsData::integrateTransforms(vec3 positionDelta, vec3 rotationDelta) {

    this->cube->integrateTransforms(positionDelta, rotationDelta);
    this->updateInertiaTensor();
}

void PhysicsData::updateInertiaTensor() {

    mat3 rotation = cube->getRotation();

    this->worldInvInertiaTensor = rotation * this->localInvInertiaTensor * transpose(rotation);
}

void PhysicsData::applyGravity(vec3 gravity, double dt) {
    this->linearVelocity += gravity * (float)dt;
}

void PhysicsData::applyPseudoImpulse(vec3 impulse, vec3 localPoint) {
    this->integrateTransforms(this->invMass * impulse, this->worldInvInertiaTensor * cross(localPoint, impulse));
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

    vec3 errorPointSum;
    float errorSum, errorMin;

    const unsigned int normalCount = 3;
    const vec3 normals[normalCount] = {
            { 1, 0, 0 },
            { 0, 1, 0 },
            { 0, 0, 1 }
    };

    const vec3* points = this->cube->getPoints();
    for (unsigned int normalIndex = 0; normalIndex < normalCount; normalIndex++) {

        vec3 normal = normals[normalIndex];

        errorPointSum = {0, 0, 0};
        errorSum = 0;
        errorMin = 0;

        for (unsigned int pointIndex = 0; pointIndex < Cube::POINTS_COUNT; pointIndex++) {

            vec3 point = points[pointIndex];

            float errorDist = glm::min(dot(point - leftBottomNear, normal), 0.0f) +
                          glm::max(dot(point - rightTopFar, normal), 0.0f);

            if (fabs(errorDist) > 0) {
                errorSum += errorDist;
                errorPointSum += point * errorDist;
                errorMin = std::max(fabs(errorMin), fabs(errorDist)) * sign(errorDist);
            }
        }

        if (fabs(errorSum) > 0) {

            vec3 errorNormal = normal;

            vec3 errorPoint = errorPointSum / errorSum;

            vec3 linearVelocityAtPoint, velocityErrorCorrection, temp, impulse;

            vec3 localErrorPoint = errorPoint - this->cube->getPosition();

            // tangent impulse
            linearVelocityAtPoint =
                    this->linearVelocity + cross(this->angularVelocity, localErrorPoint);
            vec3 c = cross(errorNormal, linearVelocityAtPoint);
            if (!isZeroVec(c)) {
                vec3 errorTangent = normalize(cross(c, errorNormal));
                if (!isZeroVec(errorTangent)) {
                    velocityErrorCorrection =
                            (-(0.0f + FRICTION)) *
                            (dot(linearVelocityAtPoint, errorTangent) * errorTangent);
                    temp = this->worldInvInertiaTensor * cross(localErrorPoint, errorTangent);
                    impulse = velocityErrorCorrection /
                              (this->invMass + dot(errorTangent, cross(temp, localErrorPoint)));
                    applyImpulse(impulse, localErrorPoint);
                }
            }

            // normal impulse
            linearVelocityAtPoint =
                    this->linearVelocity + cross(this->angularVelocity, localErrorPoint);
            velocityErrorCorrection =
                    (-(1.0f + RESTITUTION)) *
                    (dot(linearVelocityAtPoint, errorNormal) * errorNormal);
            temp = this->worldInvInertiaTensor * cross(localErrorPoint, errorNormal);

            impulse = (velocityErrorCorrection) /
                      (this->invMass + dot(errorNormal, cross(temp, localErrorPoint)));
            applyImpulse(impulse, localErrorPoint);

            vec3 error = errorNormal * errorMin;

            // normal error correction
            linearVelocityAtPoint =
                    this->linearVelocity + cross(this->angularVelocity, localErrorPoint);
            velocityErrorCorrection = -(error * 0.1f);
            temp = this->worldInvInertiaTensor * cross(localErrorPoint, errorNormal);

            impulse = velocityErrorCorrection /
                      (this->invMass + dot(errorNormal, cross(temp, localErrorPoint)));
            applyPseudoImpulse(impulse, localErrorPoint);
        }
    }
}

void PhysicsData::integrate(double dt) {
    this->integrateTransforms(this->linearVelocity * (float)dt, this->angularVelocity * (float)dt);
}

void PhysicsData::applyDamping(double dt, float damping) {

    float m = 1.0f - (float)dt * damping;

    this->linearVelocity *= m;
    this->angularVelocity *= m;
}

void PhysicsData::loadFromState(SerializedPhysics state) {

}

void PhysicsData::saveToState(SerializedPhysics* state) {

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

void Cube::integrateTransforms(vec3 positionDelta, vec3 rotationDelta) {

    this->position += positionDelta;

    quat rotation = quat_cast(this->getRotation());
    quat rotationDeltaQ = quat(0, rotationDelta.x, rotationDelta.y, rotationDelta.z);
    rotation += (rotationDeltaQ * rotation) * 0.5f;

    this->rotation = mat3_cast(normalize(rotation));

    this->calcPoints();
}

void Cube::loadFromState(SerializedCube state) {
    this->position = state.position;
    this->rotation = state.rotation;
    this->calcPoints();
}

void Cube::saveToState(SerializedCube* state) {
    state->position = this->position;
    state->rotation = this->rotation;
}