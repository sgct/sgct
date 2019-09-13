/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/SGCTTrackingDevice.h>

#include <sgct/ClusterManager.h>
#include <sgct/Engine.h>
#include <sgct/MessageHandler.h>
#include <sgct/SGCTMutexManager.h>
#include <sgct/SGCTTracker.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace sgct {

SGCTTrackingDevice::SGCTTrackingDevice(size_t parentIndex, std::string name)
    : mName(std::move(name))
    , mParentIndex(parentIndex)
{}

void SGCTTrackingDevice::setEnabled(bool state) {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    mEnabled = state;
}

void SGCTTrackingDevice::setSensorId(int id) {
    mSensorId = id;
}

void SGCTTrackingDevice::setNumberOfButtons(int numOfButtons) {
    mButtons.resize(numOfButtons, false);
    mButtonsPrevious.resize(numOfButtons, false);
    mButtonTime.resize(numOfButtons, 0.0);
    mButtonTimePrevious.resize(numOfButtons, 0.0);
    mNumberOfButtons = numOfButtons;
}

void SGCTTrackingDevice::setNumberOfAxes(int numOfAxes) {
    mAxes.resize(numOfAxes, 0.0);
    mAxesPrevious.resize(numOfAxes, 0.0);
    mNumberOfAxes = numOfAxes;
}

void SGCTTrackingDevice::setSensorTransform(glm::dvec3 vec, glm::dquat rot) {
    sgct_core::ClusterManager& cm = *sgct_core::ClusterManager::instance();
    SGCTTracker* parent = cm.getTrackingManager().getTracker(mParentIndex);

    if (parent == nullptr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "SGCTTrackingDevice: Error, can't get handle to tracker for device '%s'\n",
            mName.c_str()
        );
        return;
    }
    
    glm::mat4 parentTrans = parent->getTransform();

    // convert from double to float
    glm::quat sensorRot = rot;
    glm::vec3 sensorPos(vec);

    // create matrixes
    const glm::mat4 sensorTransMat = glm::translate(glm::mat4(1.f), sensorPos);
    const glm::mat4 sensorRotMat(glm::mat4_cast(sensorRot));

    {
        std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);

        // swap
        mSensorRotationPrevious = std::move(mSensorRotation);
        mSensorRotation = std::move(rot);

        mSensorPosPrevious = std::move(mSensorPos);
        mSensorPos = std::move(vec);

        mWorldTransformPrevious = std::move(mWorldTransform);
        mWorldTransform = parentTrans * sensorTransMat * sensorRotMat * mDeviceTransform;
    }
    setTrackerTimeStamp();
}

void SGCTTrackingDevice::setButtonVal(bool val, int index) {
    if (index >= mNumberOfButtons) {
        return;
    }

    {
        std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
        SGCTMutexManager::instance()->mTrackingMutex.lock();
        // swap
        mButtonsPrevious[index] = mButtons[index];
        mButtons[index] = val;
    }
    setButtonTimeStamp(index);
}

void SGCTTrackingDevice::setAnalogVal(const double* array, int size) {
    {
        std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
        for (int i = 0; i < std::min(size, mNumberOfAxes); i++) {
            mAxesPrevious[i] = mAxes[i];
            mAxes[i] = array[i];
        }
    }
    setAnalogTimeStamp();
}

void SGCTTrackingDevice::setOrientation(float xRot, float yRot, float zRot) {
    // create rotation quaternion based on x, y, z rotations
    glm::quat rotQuat;
    rotQuat = glm::rotate(rotQuat, glm::radians(xRot), glm::vec3(1.f, 0.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(yRot), glm::vec3(0.f, 1.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(zRot), glm::vec3(0.f, 0.f, 1.f));

    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    mOrientation = std::move(rotQuat);
    calculateTransform();
}

void SGCTTrackingDevice::setOrientation(glm::quat q) {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    mOrientation = std::move(q);
    calculateTransform();
}

void SGCTTrackingDevice::setOffset(glm::vec3 offset) {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    mOffset = std::move(offset);
    calculateTransform();
}

void SGCTTrackingDevice::setTransform(glm::mat4 mat) {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    mDeviceTransform = std::move(mat);
}

const std::string& SGCTTrackingDevice::getName() const {
    return mName;
}

int SGCTTrackingDevice::getNumberOfButtons() const {
    return mNumberOfButtons;
}

int SGCTTrackingDevice::getNumberOfAxes() const {
    return mNumberOfAxes;
}

void SGCTTrackingDevice::calculateTransform() {
    // create offset translation matrix
    glm::mat4 transMat = glm::translate(glm::mat4(1.f), mOffset);
    // calculate transform
    mDeviceTransform = transMat * glm::mat4_cast(mOrientation);
}

int SGCTTrackingDevice::getSensorId() {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return mSensorId;
}

bool SGCTTrackingDevice::getButton(int index) const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return index < mNumberOfButtons ? mButtons[index] : false;;
}

bool SGCTTrackingDevice::getButtonPrevious(int index) const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return index < mNumberOfButtons ? mButtonsPrevious[index] : false;;
}

double SGCTTrackingDevice::getAnalog(int index) const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return index < mNumberOfAxes ? mAxes[index] : 0.0;;
}

double SGCTTrackingDevice::getAnalogPrevious(int index) const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return index < mNumberOfAxes ? mAxesPrevious[index] : 0.0;;
}

glm::vec3 SGCTTrackingDevice::getPosition() const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return glm::vec3(mWorldTransform[3]);
}

glm::vec3 SGCTTrackingDevice::getPreviousPosition() const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return glm::vec3(mWorldTransformPrevious[3]);
}


glm::vec3 SGCTTrackingDevice::getEulerAngles() const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return glm::eulerAngles(glm::quat_cast(mWorldTransform));;
}

glm::vec3 SGCTTrackingDevice::getEulerAnglesPrevious() const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return glm::eulerAngles(glm::quat_cast(mWorldTransformPrevious));;
}


glm::quat SGCTTrackingDevice::getRotation() const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return glm::quat_cast(mWorldTransform);;
}

glm::quat SGCTTrackingDevice::getRotationPrevious() const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return glm::quat_cast(mWorldTransformPrevious);;
}

glm::mat4 SGCTTrackingDevice::getWorldTransform() const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return mWorldTransform;
}

glm::mat4 SGCTTrackingDevice::getWorldTransformPrevious() const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return mWorldTransformPrevious;
}

glm::dquat SGCTTrackingDevice::getSensorRotation() const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return mSensorRotation;
}

glm::dquat SGCTTrackingDevice::getSensorRotationPrevious() const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return mSensorRotationPrevious;
}

glm::dvec3 SGCTTrackingDevice::getSensorPosition() const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return mSensorPos;
}

glm::dvec3 SGCTTrackingDevice::getSensorPositionPrevious() const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return mSensorPosPrevious;
}

bool SGCTTrackingDevice::isEnabled() const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return mEnabled;
}

bool SGCTTrackingDevice::hasSensor() const {
    return mSensorId != -1;
}

bool SGCTTrackingDevice::hasButtons() const {
    return mNumberOfButtons > 0;
}

bool SGCTTrackingDevice::hasAnalogs() const {
    return mNumberOfAxes > 0;
}

void SGCTTrackingDevice::setTrackerTimeStamp() {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    mTrackerTimePrevious = mTrackerTime;
    mTrackerTime = Engine::getTime();
}

void SGCTTrackingDevice::setAnalogTimeStamp() {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    mAnalogTimePrevious = mAnalogTime;
    mAnalogTime = Engine::getTime();
}

void SGCTTrackingDevice::setButtonTimeStamp(size_t index) {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    mButtonTimePrevious[index] = mButtonTime[index];
    mButtonTime[index] = Engine::getTime();
}

double SGCTTrackingDevice::getTrackerTimeStamp() {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return mTrackerTime;
}

double SGCTTrackingDevice::getTrackerTimeStampPrevious() {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return mTrackerTimePrevious;
}

double SGCTTrackingDevice::getAnalogTimeStamp() const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return mAnalogTime;
}

double SGCTTrackingDevice::getAnalogTimeStampPrevious() const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return mAnalogTimePrevious;
}

double SGCTTrackingDevice::getButtonTimeStamp(size_t index) const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return mButtonTime[index];
}

double SGCTTrackingDevice::getButtonTimeStampPrevious(size_t index) const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return mButtonTimePrevious[index];
}


double SGCTTrackingDevice::getTrackerDeltaTime() const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return mTrackerTime - mTrackerTimePrevious;
}

double SGCTTrackingDevice::getAnalogDeltaTime() const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return mAnalogTime - mAnalogTimePrevious;
}

double SGCTTrackingDevice::getButtonDeltaTime(size_t index) const {
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return mButtonTime[index] - mButtonTimePrevious[index];;
}

} // namespace sgct
