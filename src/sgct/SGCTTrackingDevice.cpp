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
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    mEnabled = state;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
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

    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    
    // swap
    mSensorRotationPrevious = std::move(mSensorRotation);
    mSensorRotation = std::move(rot);

    mSensorPosPrevious = std::move(mSensorPos);
    mSensorPos = std::move(vec);

    mWorldTransformPrevious = std::move(mWorldTransform);
    mWorldTransform = parentTrans * sensorTransMat * sensorRotMat * mDeviceTransform;

    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

    setTrackerTimeStamp();
}

void SGCTTrackingDevice::setButtonVal(bool val, int index) {
    if (index >= mNumberOfButtons) {
        return;
    }

    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    // swap
    mButtonsPrevious[index] = mButtons[index];
    mButtons[index] = val;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

    setButtonTimeStamp(index);
}

void SGCTTrackingDevice::setAnalogVal(const double* array, int size) {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    for (size_t i = 0; i < std::min(size, mNumberOfAxes); i++) {
        mAxesPrevious[i] = mAxes[i];
        mAxes[i] = array[i];
    }
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

    setAnalogTimeStamp();
}

void SGCTTrackingDevice::setOrientation(float xRot, float yRot, float zRot) {
    // create rotation quaternion based on x, y, z rotations
    glm::quat rotQuat;
    rotQuat = glm::rotate(rotQuat, glm::radians(xRot), glm::vec3(1.f, 0.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(yRot), glm::vec3(0.f, 1.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(zRot), glm::vec3(0.f, 0.f, 1.f));

    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    mOrientation = std::move(rotQuat);
    calculateTransform();
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

void SGCTTrackingDevice::setOrientation(glm::quat q) {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    mOrientation = std::move(q);
    calculateTransform();
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

void SGCTTrackingDevice::setOffset(glm::vec3 offset) {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    mOffset = std::move(offset);
    calculateTransform();
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

void SGCTTrackingDevice::setTransform(glm::mat4 mat) {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    mDeviceTransform = std::move(mat);
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
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
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    int tmpVal = mSensorId;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpVal;
}

bool SGCTTrackingDevice::getButton(size_t index) const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    bool tmpVal = index < mNumberOfButtons ? mButtons[index] : false;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpVal;
}

bool SGCTTrackingDevice::getButtonPrevious(size_t index) const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    bool tmpVal = index < mNumberOfButtons ? mButtonsPrevious[index] : false;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpVal;
}

double SGCTTrackingDevice::getAnalog(size_t index) const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    double tmpVal = index < mNumberOfAxes ? mAxes[index] : 0.0;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpVal;
}

double SGCTTrackingDevice::getAnalogPrevious(size_t index) const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    double tmpVal = index < mNumberOfAxes ? mAxesPrevious[index] : 0.0;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpVal;
}

glm::vec3 SGCTTrackingDevice::getPosition() const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    glm::vec3 tmpVal = glm::vec3(mWorldTransform[3]);
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpVal;
}

glm::vec3 SGCTTrackingDevice::getPreviousPosition() const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    glm::vec3 tmpVal = glm::vec3(mWorldTransformPrevious[3]);
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpVal;
}


glm::vec3 SGCTTrackingDevice::getEulerAngles() const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    glm::vec3 tmpVal = glm::eulerAngles(glm::quat_cast(mWorldTransform));
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpVal;
}

glm::vec3 SGCTTrackingDevice::getEulerAnglesPrevious() const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    glm::vec3 tmpVal = glm::eulerAngles(glm::quat_cast(mWorldTransformPrevious));
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpVal;
}


glm::quat SGCTTrackingDevice::getRotation() const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    glm::quat tmpQuat = glm::quat_cast(mWorldTransform);
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpQuat;
}

glm::quat SGCTTrackingDevice::getRotationPrevious() const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    glm::quat tmpQuat = glm::quat_cast(mWorldTransformPrevious);
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpQuat;
}

glm::mat4 SGCTTrackingDevice::getWorldTransform() const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    glm::mat4 tmpMat = mWorldTransform;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpMat;
}

glm::mat4 SGCTTrackingDevice::getWorldTransformPrevious() const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    glm::mat4 tmpMat = mWorldTransformPrevious;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpMat;
}

glm::dquat SGCTTrackingDevice::getSensorRotation() const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    glm::dquat tmpQuat = mSensorRotation;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpQuat;
}

glm::dquat SGCTTrackingDevice::getSensorRotationPrevious() const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    glm::dquat tmpQuat = mSensorRotationPrevious;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpQuat;
}

glm::dvec3 SGCTTrackingDevice::getSensorPosition() const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    glm::dvec3 tmpVec = mSensorPos;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpVec;
}

glm::dvec3 SGCTTrackingDevice::getSensorPositionPrevious() const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    glm::dvec3 tmpVec = mSensorPosPrevious;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
    return tmpVec;
}

bool SGCTTrackingDevice::isEnabled() const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex );
    bool tmpVal = mEnabled;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex );
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
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    // swap
    mTrackerTimePrevious = mTrackerTime;
    mTrackerTime = Engine::getTime();
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

void SGCTTrackingDevice::setAnalogTimeStamp() {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    //swap
    mAnalogTimePrevious = mAnalogTime;
    mAnalogTime = Engine::getTime();
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

void SGCTTrackingDevice::setButtonTimeStamp(size_t index) {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    //swap
    mButtonTimePrevious[index] = mButtonTime[index];
    mButtonTime[index] = Engine::getTime();
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

double SGCTTrackingDevice::getTrackerTimeStamp() {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex );
    double tmpVal = mTrackerTime;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex );

    return tmpVal;
}

double SGCTTrackingDevice::getTrackerTimeStampPrevious() {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    double tmpVal = mTrackerTimePrevious;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

    return tmpVal;
}

double SGCTTrackingDevice::getAnalogTimeStamp() const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    double tmpVal = mAnalogTime;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

    return tmpVal;
}

double SGCTTrackingDevice::getAnalogTimeStampPrevious() const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    double tmpVal = mAnalogTimePrevious;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

    return tmpVal;
}

double SGCTTrackingDevice::getButtonTimeStamp(size_t index) const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex );
    double tmpVal = mButtonTime[index];
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex );

    return tmpVal;
}

double SGCTTrackingDevice::getButtonTimeStampPrevious(size_t index) const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    double tmpVal = mButtonTimePrevious[index];
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

    return tmpVal;
}


double SGCTTrackingDevice::getTrackerDeltaTime() const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    double tmpVal = mTrackerTime - mTrackerTimePrevious;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

    return tmpVal;
}

double SGCTTrackingDevice::getAnalogDeltaTime() const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    double tmpVal = mAnalogTime - mAnalogTimePrevious;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

    return tmpVal;
}

double SGCTTrackingDevice::getButtonDeltaTime(size_t index) const {
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    double tmpVal = mButtonTime[index] - mButtonTimePrevious[index];
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

    return tmpVal;
}

} // namespace sgct
