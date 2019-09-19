/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/trackingdevice.h>

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/messageHandler.h>
#include <sgct/mutexmanager.h>
#include <sgct/tracker.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace sgct {

TrackingDevice::TrackingDevice(size_t parentIndex, std::string name)
    : mName(std::move(name))
    , mParentIndex(parentIndex)
{}

void TrackingDevice::setEnabled(bool state) {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    mEnabled = state;
}

void TrackingDevice::setSensorId(int id) {
    mSensorId = id;
}

void TrackingDevice::setNumberOfButtons(int numOfButtons) {
    mButtons.resize(numOfButtons, false);
    mButtonsPrevious.resize(numOfButtons, false);
    mButtonTime.resize(numOfButtons, 0.0);
    mButtonTimePrevious.resize(numOfButtons, 0.0);
    mNumberOfButtons = numOfButtons;
}

void TrackingDevice::setNumberOfAxes(int numOfAxes) {
    mAxes.resize(numOfAxes, 0.0);
    mAxesPrevious.resize(numOfAxes, 0.0);
    mNumberOfAxes = numOfAxes;
}

void TrackingDevice::setSensorTransform(glm::dvec3 vec, glm::dquat rot) {
    core::ClusterManager& cm = *core::ClusterManager::instance();
    Tracker* parent = cm.getTrackingManager().getTracker(mParentIndex);

    if (parent == nullptr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "TrackingDevice: Error, can't get handle to tracker for device '%s'\n",
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
        std::unique_lock lock(MutexManager::instance()->mTrackingMutex);

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

void TrackingDevice::setButtonVal(bool val, int index) {
    if (index >= mNumberOfButtons) {
        return;
    }

    {
        std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
        MutexManager::instance()->mTrackingMutex.lock();
        // swap
        mButtonsPrevious[index] = mButtons[index];
        mButtons[index] = val;
    }
    setButtonTimeStamp(index);
}

void TrackingDevice::setAnalogVal(const double* array, int size) {
    {
        std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
        for (int i = 0; i < std::min(size, mNumberOfAxes); i++) {
            mAxesPrevious[i] = mAxes[i];
            mAxes[i] = array[i];
        }
    }
    setAnalogTimeStamp();
}

void TrackingDevice::setOrientation(float xRot, float yRot, float zRot) {
    // create rotation quaternion based on x, y, z rotations
    glm::quat rotQuat;
    rotQuat = glm::rotate(rotQuat, glm::radians(xRot), glm::vec3(1.f, 0.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(yRot), glm::vec3(0.f, 1.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(zRot), glm::vec3(0.f, 0.f, 1.f));

    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    mOrientation = std::move(rotQuat);
    calculateTransform();
}

void TrackingDevice::setOrientation(glm::quat q) {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    mOrientation = std::move(q);
    calculateTransform();
}

void TrackingDevice::setOffset(glm::vec3 offset) {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    mOffset = std::move(offset);
    calculateTransform();
}

void TrackingDevice::setTransform(glm::mat4 mat) {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    mDeviceTransform = std::move(mat);
}

const std::string& TrackingDevice::getName() const {
    return mName;
}

int TrackingDevice::getNumberOfButtons() const {
    return mNumberOfButtons;
}

int TrackingDevice::getNumberOfAxes() const {
    return mNumberOfAxes;
}

void TrackingDevice::calculateTransform() {
    // create offset translation matrix
    glm::mat4 transMat = glm::translate(glm::mat4(1.f), mOffset);
    // calculate transform
    mDeviceTransform = transMat * glm::mat4_cast(mOrientation);
}

int TrackingDevice::getSensorId() {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return mSensorId;
}

bool TrackingDevice::getButton(int index) const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return index < mNumberOfButtons ? mButtons[index] : false;;
}

bool TrackingDevice::getButtonPrevious(int index) const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return index < mNumberOfButtons ? mButtonsPrevious[index] : false;;
}

double TrackingDevice::getAnalog(int index) const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return index < mNumberOfAxes ? mAxes[index] : 0.0;;
}

double TrackingDevice::getAnalogPrevious(int index) const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return index < mNumberOfAxes ? mAxesPrevious[index] : 0.0;;
}

glm::vec3 TrackingDevice::getPosition() const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return glm::vec3(mWorldTransform[3]);
}

glm::vec3 TrackingDevice::getPreviousPosition() const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return glm::vec3(mWorldTransformPrevious[3]);
}

glm::vec3 TrackingDevice::getEulerAngles() const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return glm::eulerAngles(glm::quat_cast(mWorldTransform));;
}

glm::vec3 TrackingDevice::getEulerAnglesPrevious() const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return glm::eulerAngles(glm::quat_cast(mWorldTransformPrevious));;
}

glm::quat TrackingDevice::getRotation() const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return glm::quat_cast(mWorldTransform);;
}

glm::quat TrackingDevice::getRotationPrevious() const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return glm::quat_cast(mWorldTransformPrevious);;
}

glm::mat4 TrackingDevice::getWorldTransform() const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return mWorldTransform;
}

glm::mat4 TrackingDevice::getWorldTransformPrevious() const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return mWorldTransformPrevious;
}

glm::dquat TrackingDevice::getSensorRotation() const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return mSensorRotation;
}

glm::dquat TrackingDevice::getSensorRotationPrevious() const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return mSensorRotationPrevious;
}

glm::dvec3 TrackingDevice::getSensorPosition() const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return mSensorPos;
}

glm::dvec3 TrackingDevice::getSensorPositionPrevious() const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return mSensorPosPrevious;
}

bool TrackingDevice::isEnabled() const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return mEnabled;
}

bool TrackingDevice::hasSensor() const {
    return mSensorId != -1;
}

bool TrackingDevice::hasButtons() const {
    return mNumberOfButtons > 0;
}

bool TrackingDevice::hasAnalogs() const {
    return mNumberOfAxes > 0;
}

void TrackingDevice::setTrackerTimeStamp() {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    mTrackerTimePrevious = mTrackerTime;
    mTrackerTime = Engine::getTime();
}

void TrackingDevice::setAnalogTimeStamp() {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    mAnalogTimePrevious = mAnalogTime;
    mAnalogTime = Engine::getTime();
}

void TrackingDevice::setButtonTimeStamp(size_t index) {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    mButtonTimePrevious[index] = mButtonTime[index];
    mButtonTime[index] = Engine::getTime();
}

double TrackingDevice::getTrackerTimeStamp() {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return mTrackerTime;
}

double TrackingDevice::getTrackerTimeStampPrevious() {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return mTrackerTimePrevious;
}

double TrackingDevice::getAnalogTimeStamp() const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return mAnalogTime;
}

double TrackingDevice::getAnalogTimeStampPrevious() const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return mAnalogTimePrevious;
}

double TrackingDevice::getButtonTimeStamp(size_t index) const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return mButtonTime[index];
}

double TrackingDevice::getButtonTimeStampPrevious(size_t index) const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return mButtonTimePrevious[index];
}


double TrackingDevice::getTrackerDeltaTime() const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return mTrackerTime - mTrackerTimePrevious;
}

double TrackingDevice::getAnalogDeltaTime() const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return mAnalogTime - mAnalogTimePrevious;
}

double TrackingDevice::getButtonDeltaTime(size_t index) const {
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return mButtonTime[index] - mButtonTimePrevious[index];;
}

} // namespace sgct
