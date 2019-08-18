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
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    mEnabled = state;
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
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
    SGCTTracker* parent = cm.getTrackingManagerPtr().getTrackerPtr(mParentIndex);

    if (parent == nullptr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "SGCTTrackingDevice: Error, can't get handle to tracker for device '%s'!\n",
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

    SGCTMutexManager::instance()->mTrackingMutex.lock();
    
    // swap
    mSensorRotationPrevious = std::move(mSensorRotation);
    mSensorRotation = std::move(rot);

    mSensorPosPrevious = std::move(mSensorPos);
    mSensorPos = std::move(vec);

    mWorldTransformPrevious = std::move(mWorldTransform);
    mWorldTransform = parentTrans * sensorTransMat * sensorRotMat * mDeviceTransform;

    SGCTMutexManager::instance()->mTrackingMutex.unlock();

    setTrackerTimeStamp();
}

void SGCTTrackingDevice::setButtonVal(bool val, int index) {
    if (index >= mNumberOfButtons) {
        return;
    }

    SGCTMutexManager::instance()->mTrackingMutex.lock();
    // swap
    mButtonsPrevious[index] = mButtons[index];
    mButtons[index] = val;
    SGCTMutexManager::instance()->mTrackingMutex.unlock();

    setButtonTimeStamp(index);
}

void SGCTTrackingDevice::setAnalogVal(const double* array, int size) {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    for (size_t i = 0; i < std::min(size, mNumberOfAxes); i++) {
        mAxesPrevious[i] = mAxes[i];
        mAxes[i] = array[i];
    }
    SGCTMutexManager::instance()->mTrackingMutex.unlock();

    setAnalogTimeStamp();
}

void SGCTTrackingDevice::setOrientation(float xRot, float yRot, float zRot) {
    // create rotation quaternion based on x, y, z rotations
    glm::quat rotQuat;
    rotQuat = glm::rotate(rotQuat, glm::radians(xRot), glm::vec3(1.f, 0.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(yRot), glm::vec3(0.f, 1.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(zRot), glm::vec3(0.f, 0.f, 1.f));

    SGCTMutexManager::instance()->mTrackingMutex.lock();
    mOrientation = std::move(rotQuat);
    calculateTransform();
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
}

void SGCTTrackingDevice::setOrientation(glm::quat q) {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    mOrientation = std::move(q);
    calculateTransform();
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
}

void SGCTTrackingDevice::setOffset(glm::vec3 offset) {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    mOffset = std::move(offset);
    calculateTransform();
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
}

void SGCTTrackingDevice::setTransform(glm::mat4 mat) {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    mDeviceTransform = std::move(mat);
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
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
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    int tmpVal = mSensorId;
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
    return tmpVal;
}

bool SGCTTrackingDevice::getButton(size_t index) const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    bool tmpVal = index < mNumberOfButtons ? mButtons[index] : false;
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
    return tmpVal;
}

bool SGCTTrackingDevice::getButtonPrevious(size_t index) const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    bool tmpVal = index < mNumberOfButtons ? mButtonsPrevious[index] : false;
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
    return tmpVal;
}

double SGCTTrackingDevice::getAnalog(size_t index) const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    double tmpVal = index < mNumberOfAxes ? mAxes[index] : 0.0;
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
    return tmpVal;
}

double SGCTTrackingDevice::getAnalogPrevious(size_t index) const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    double tmpVal = index < mNumberOfAxes ? mAxesPrevious[index] : 0.0;
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
    return tmpVal;
}

glm::vec3 SGCTTrackingDevice::getPosition() const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    glm::vec3 tmpVal = glm::vec3(mWorldTransform[3]);
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
    return tmpVal;
}

glm::vec3 SGCTTrackingDevice::getPreviousPosition() const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    glm::vec3 tmpVal = glm::vec3(mWorldTransformPrevious[3]);
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
    return tmpVal;
}


glm::vec3 SGCTTrackingDevice::getEulerAngles() const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    glm::vec3 tmpVal = glm::eulerAngles(glm::quat_cast(mWorldTransform));
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
    return tmpVal;
}

glm::vec3 SGCTTrackingDevice::getEulerAnglesPrevious() const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    glm::vec3 tmpVal = glm::eulerAngles(glm::quat_cast(mWorldTransformPrevious));
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
    return tmpVal;
}


glm::quat SGCTTrackingDevice::getRotation() const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    glm::quat tmpQuat = glm::quat_cast(mWorldTransform);
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
    return tmpQuat;
}

glm::quat SGCTTrackingDevice::getRotationPrevious() const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    glm::quat tmpQuat = glm::quat_cast(mWorldTransformPrevious);
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
    return tmpQuat;
}

glm::mat4 SGCTTrackingDevice::getWorldTransform() const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    glm::mat4 tmpMat = mWorldTransform;
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
    return tmpMat;
}

glm::mat4 SGCTTrackingDevice::getWorldTransformPrevious() const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    glm::mat4 tmpMat = mWorldTransformPrevious;
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
    return tmpMat;
}

glm::dquat SGCTTrackingDevice::getSensorRotation() const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    glm::dquat tmpQuat = mSensorRotation;
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
    return tmpQuat;
}

glm::dquat SGCTTrackingDevice::getSensorRotationPrevious() const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    glm::dquat tmpQuat = mSensorRotationPrevious;
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
    return tmpQuat;
}

glm::dvec3 SGCTTrackingDevice::getSensorPosition() const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    glm::dvec3 tmpVec = mSensorPos;
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
    return tmpVec;
}

glm::dvec3 SGCTTrackingDevice::getSensorPositionPrevious() const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    glm::dvec3 tmpVec = mSensorPosPrevious;
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
    return tmpVec;
}

bool SGCTTrackingDevice::isEnabled() const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    bool tmpVal = mEnabled;
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
    return tmpVal;
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
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    // swap
    mTrackerTimePrevious = mTrackerTime;
    mTrackerTime = Engine::getTime();
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
}

void SGCTTrackingDevice::setAnalogTimeStamp() {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    //swap
    mAnalogTimePrevious = mAnalogTime;
    mAnalogTime = Engine::getTime();
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
}

void SGCTTrackingDevice::setButtonTimeStamp(size_t index) {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    //swap
    mButtonTimePrevious[index] = mButtonTime[index];
    mButtonTime[index] = Engine::getTime();
    SGCTMutexManager::instance()->mTrackingMutex.unlock();
}

double SGCTTrackingDevice::getTrackerTimeStamp() {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    double tmpVal = mTrackerTime;
    SGCTMutexManager::instance()->mTrackingMutex.unlock();

    return tmpVal;
}

double SGCTTrackingDevice::getTrackerTimeStampPrevious() {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    double tmpVal = mTrackerTimePrevious;
    SGCTMutexManager::instance()->mTrackingMutex.unlock();

    return tmpVal;
}

double SGCTTrackingDevice::getAnalogTimeStamp() const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    double tmpVal = mAnalogTime;
    SGCTMutexManager::instance()->mTrackingMutex.unlock();

    return tmpVal;
}

double SGCTTrackingDevice::getAnalogTimeStampPrevious() const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    double tmpVal = mAnalogTimePrevious;
    SGCTMutexManager::instance()->mTrackingMutex.unlock();

    return tmpVal;
}

double SGCTTrackingDevice::getButtonTimeStamp(size_t index) const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    double tmpVal = mButtonTime[index];
    SGCTMutexManager::instance()->mTrackingMutex.unlock();

    return tmpVal;
}

double SGCTTrackingDevice::getButtonTimeStampPrevious(size_t index) const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    double tmpVal = mButtonTimePrevious[index];
    SGCTMutexManager::instance()->mTrackingMutex.unlock();

    return tmpVal;
}


double SGCTTrackingDevice::getTrackerDeltaTime() const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    double tmpVal = mTrackerTime - mTrackerTimePrevious;
    SGCTMutexManager::instance()->mTrackingMutex.unlock();

    return tmpVal;
}

double SGCTTrackingDevice::getAnalogDeltaTime() const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    double tmpVal = mAnalogTime - mAnalogTimePrevious;
    SGCTMutexManager::instance()->mTrackingMutex.unlock();

    return tmpVal;
}

double SGCTTrackingDevice::getButtonDeltaTime(size_t index) const {
    SGCTMutexManager::instance()->mTrackingMutex.lock();
    double tmpVal = mButtonTime[index] - mButtonTimePrevious[index];
    SGCTMutexManager::instance()->mTrackingMutex.unlock();

    return tmpVal;
}

} // namespace sgct
