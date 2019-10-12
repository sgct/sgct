/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/trackingdevice.h>

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/mutexmanager.h>
#include <sgct/tracker.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace sgct {

TrackingDevice::TrackingDevice(size_t parentIndex, std::string name)
    : _name(std::move(name))
    , _parentIndex(parentIndex)
{}

void TrackingDevice::setEnabled(bool state) {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    _enabled = state;
}

void TrackingDevice::setSensorId(int id) {
    _sensorId = id;
}

void TrackingDevice::setNumberOfButtons(int numOfButtons) {
    _buttons.resize(numOfButtons, false);
    _buttonsPrevious.resize(numOfButtons, false);
    _buttonTime.resize(numOfButtons, 0.0);
    _buttonTimePrevious.resize(numOfButtons, 0.0);
    _nButtons = numOfButtons;
}

void TrackingDevice::setNumberOfAxes(int numOfAxes) {
    _axes.resize(numOfAxes, 0.0);
    _axesPrevious.resize(numOfAxes, 0.0);
    _nAxes = numOfAxes;
}

void TrackingDevice::setSensorTransform(glm::dvec3 vec, glm::dquat rot) {
    core::ClusterManager& cm = *core::ClusterManager::instance();
    Tracker* parent = cm.getTrackingManager().getTracker(_parentIndex);

    if (parent == nullptr) {
        MessageHandler::instance()->printError(
            "TrackingDevice: Error getting handle to tracker for device '%s'",
            _name.c_str()
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
        std::unique_lock lock(MutexManager::instance()->trackingMutex);

        // swap
        _sensorRotationPrevious = std::move(_sensorRotation);
        _sensorRotation = std::move(rot);

        _sensorPosPrevious = std::move(_sensorPos);
        _sensorPos = std::move(vec);

        _worldTransformPrevious = std::move(_worldTransform);
        _worldTransform = parentTrans * sensorTransMat * sensorRotMat * _deviceTransform;
    }
    setTrackerTimeStamp();
}

void TrackingDevice::setButtonVal(bool val, int index) {
    if (index >= _nButtons) {
        return;
    }

    {
        std::unique_lock lock(MutexManager::instance()->trackingMutex);
        MutexManager::instance()->trackingMutex.lock();
        // swap
        _buttonsPrevious[index] = _buttons[index];
        _buttons[index] = val;
    }
    setButtonTimeStamp(index);
}

void TrackingDevice::setAnalogVal(const double* array, int size) {
    {
        std::unique_lock lock(MutexManager::instance()->trackingMutex);
        for (int i = 0; i < std::min(size, _nAxes); i++) {
            _axesPrevious[i] = _axes[i];
            _axes[i] = array[i];
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

    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    _orientation = std::move(rotQuat);
    calculateTransform();
}

void TrackingDevice::setOrientation(glm::quat q) {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    _orientation = std::move(q);
    calculateTransform();
}

void TrackingDevice::setOffset(glm::vec3 offset) {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    _offset = std::move(offset);
    calculateTransform();
}

void TrackingDevice::setTransform(glm::mat4 mat) {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    _deviceTransform = std::move(mat);
}

const std::string& TrackingDevice::getName() const {
    return _name;
}

int TrackingDevice::getNumberOfButtons() const {
    return _nButtons;
}

int TrackingDevice::getNumberOfAxes() const {
    return _nAxes;
}

void TrackingDevice::calculateTransform() {
    // create offset translation matrix
    glm::mat4 transMat = glm::translate(glm::mat4(1.f), _offset);
    // calculate transform
    _deviceTransform = transMat * glm::mat4_cast(_orientation);
}

int TrackingDevice::getSensorId() {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return _sensorId;
}

bool TrackingDevice::getButton(int index) const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return index < _nButtons ? _buttons[index] : false;
}

bool TrackingDevice::getButtonPrevious(int index) const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return index < _nButtons ? _buttonsPrevious[index] : false;
}

double TrackingDevice::getAnalog(int index) const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return index < _nAxes ? _axes[index] : 0.0;
}

double TrackingDevice::getAnalogPrevious(int index) const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return index < _nAxes ? _axesPrevious[index] : 0.0;
}

glm::vec3 TrackingDevice::getPosition() const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return glm::vec3(_worldTransform[3]);
}

glm::vec3 TrackingDevice::getPreviousPosition() const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return glm::vec3(_worldTransformPrevious[3]);
}

glm::vec3 TrackingDevice::getEulerAngles() const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return glm::eulerAngles(glm::quat_cast(_worldTransform));
}

glm::vec3 TrackingDevice::getEulerAnglesPrevious() const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return glm::eulerAngles(glm::quat_cast(_worldTransformPrevious));
}

glm::quat TrackingDevice::getRotation() const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return glm::quat_cast(_worldTransform);
}

glm::quat TrackingDevice::getRotationPrevious() const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return glm::quat_cast(_worldTransformPrevious);
}

glm::mat4 TrackingDevice::getWorldTransform() const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return _worldTransform;
}

glm::mat4 TrackingDevice::getWorldTransformPrevious() const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return _worldTransformPrevious;
}

glm::dquat TrackingDevice::getSensorRotation() const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return _sensorRotation;
}

glm::dquat TrackingDevice::getSensorRotationPrevious() const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return _sensorRotationPrevious;
}

glm::dvec3 TrackingDevice::getSensorPosition() const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return _sensorPos;
}

glm::dvec3 TrackingDevice::getSensorPositionPrevious() const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return _sensorPosPrevious;
}

bool TrackingDevice::isEnabled() const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return _enabled;
}

bool TrackingDevice::hasSensor() const {
    return _sensorId != -1;
}

bool TrackingDevice::hasButtons() const {
    return _nButtons > 0;
}

bool TrackingDevice::hasAnalogs() const {
    return _nAxes > 0;
}

void TrackingDevice::setTrackerTimeStamp() {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    _trackerTimePrevious = _trackerTime;
    _trackerTime = Engine::getTime();
}

void TrackingDevice::setAnalogTimeStamp() {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    _analogTimePrevious = _analogTime;
    _analogTime = Engine::getTime();
}

void TrackingDevice::setButtonTimeStamp(size_t index) {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    _buttonTimePrevious[index] = _buttonTime[index];
    _buttonTime[index] = Engine::getTime();
}

double TrackingDevice::getTrackerTimeStamp() {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return _trackerTime;
}

double TrackingDevice::getTrackerTimeStampPrevious() {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return _trackerTimePrevious;
}

double TrackingDevice::getAnalogTimeStamp() const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return _analogTime;
}

double TrackingDevice::getAnalogTimeStampPrevious() const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return _analogTimePrevious;
}

double TrackingDevice::getButtonTimeStamp(size_t index) const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return _buttonTime[index];
}

double TrackingDevice::getButtonTimeStampPrevious(size_t index) const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return _buttonTimePrevious[index];
}


double TrackingDevice::getTrackerDeltaTime() const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return _trackerTime - _trackerTimePrevious;
}

double TrackingDevice::getAnalogDeltaTime() const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return _analogTime - _analogTimePrevious;
}

double TrackingDevice::getButtonDeltaTime(size_t index) const {
    std::unique_lock lock(MutexManager::instance()->trackingMutex);
    return _buttonTime[index] - _buttonTimePrevious[index];
}

} // namespace sgct
