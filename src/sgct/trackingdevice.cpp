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
#include <sgct/logger.h>
#include <sgct/mutexes.h>
#include <sgct/tracker.h>
#include <sgct/trackingmanager.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace sgct {

TrackingDevice::TrackingDevice(int parentIndex, std::string name)
    : _name(std::move(name))
    , _parentIndex(parentIndex)
{}

void TrackingDevice::setEnabled(bool state) {
    std::unique_lock lock(core::mutex::Tracking);
    _isEnabled = state;
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

void TrackingDevice::setSensorTransform(glm::vec3 vec, glm::quat rot) {
    Tracker* parent = TrackingManager::instance().tracker(_parentIndex);

    if (parent == nullptr) {
        Logger::Error("Error getting handle to tracker for device '%s'", _name.c_str());
        return;
    }
    
    const glm::mat4 parentTrans = parent->getTransform();

    // create matrixes
    const glm::mat4 sensorTransMat = glm::translate(glm::mat4(1.f), vec);
    const glm::mat4 sensorRotMat(glm::mat4_cast(rot));

    {
        std::unique_lock lock(core::mutex::Tracking);

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

void TrackingDevice::setButtonValue(bool val, int index) {
    if (index >= _nButtons) {
        return;
    }

    {
        std::unique_lock lock(core::mutex::Tracking);
        // swap
        _buttonsPrevious[index] = _buttons[index];
        _buttons[index] = val;
    }
    setButtonTimeStamp(index);
}

void TrackingDevice::setAnalogValue(const double* array, int size) {
    {
        std::unique_lock lock(core::mutex::Tracking);
        for (int i = 0; i < std::min(size, _nAxes); i++) {
            _axesPrevious[i] = _axes[i];
            _axes[i] = array[i];
        }
    }
    setAnalogTimeStamp();
}

void TrackingDevice::setOrientation(float xRot, float yRot, float zRot) {
    // create rotation quaternion based on x, y, z rotations
    glm::quat rotQuat = glm::quat(1.f, 0.f, 0.f, 0.f);
    rotQuat = glm::rotate(rotQuat, glm::radians(xRot), glm::vec3(1.f, 0.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(yRot), glm::vec3(0.f, 1.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(zRot), glm::vec3(0.f, 0.f, 1.f));

    std::unique_lock lock(core::mutex::Tracking);
    _orientation = std::move(rotQuat);
    calculateTransform();
}

void TrackingDevice::setOrientation(glm::quat q) {
    std::unique_lock lock(core::mutex::Tracking);
    _orientation = std::move(q);
    calculateTransform();
}

void TrackingDevice::setOffset(glm::vec3 offset) {
    std::unique_lock lock(core::mutex::Tracking);
    _offset = std::move(offset);
    calculateTransform();
}

void TrackingDevice::setTransform(glm::mat4 mat) {
    std::unique_lock lock(core::mutex::Tracking);
    _deviceTransform = std::move(mat);
}

const std::string& TrackingDevice::name() const {
    return _name;
}

int TrackingDevice::numberOfButtons() const {
    return _nButtons;
}

int TrackingDevice::numberOfAxes() const {
    return _nAxes;
}

void TrackingDevice::calculateTransform() {
    const glm::mat4 transMat = glm::translate(glm::mat4(1.f), _offset);
    _deviceTransform = transMat * glm::mat4_cast(_orientation);
}

int TrackingDevice::sensorId() {
    std::unique_lock lock(core::mutex::Tracking);
    return _sensorId;
}

bool TrackingDevice::button(int index) const {
    std::unique_lock lock(core::mutex::Tracking);
    return index < _nButtons ? _buttons[index] : false;
}

bool TrackingDevice::buttonPrevious(int index) const {
    std::unique_lock lock(core::mutex::Tracking);
    return index < _nButtons ? _buttonsPrevious[index] : false;
}

double TrackingDevice::analog(int index) const {
    std::unique_lock lock(core::mutex::Tracking);
    return index < _nAxes ? _axes[index] : 0.0;
}

double TrackingDevice::analogPrevious(int index) const {
    std::unique_lock lock(core::mutex::Tracking);
    return index < _nAxes ? _axesPrevious[index] : 0.0;
}

glm::vec3 TrackingDevice::position() const {
    std::unique_lock lock(core::mutex::Tracking);
    return glm::vec3(_worldTransform[3]);
}

glm::vec3 TrackingDevice::previousPosition() const {
    std::unique_lock lock(core::mutex::Tracking);
    return glm::vec3(_worldTransformPrevious[3]);
}

glm::vec3 TrackingDevice::eulerAngles() const {
    std::unique_lock lock(core::mutex::Tracking);
    return glm::eulerAngles(glm::quat_cast(_worldTransform));
}

glm::vec3 TrackingDevice::eulerAnglesPrevious() const {
    std::unique_lock lock(core::mutex::Tracking);
    return glm::eulerAngles(glm::quat_cast(_worldTransformPrevious));
}

glm::quat TrackingDevice::rotation() const {
    std::unique_lock lock(core::mutex::Tracking);
    return glm::quat_cast(_worldTransform);
}

glm::quat TrackingDevice::rotationPrevious() const {
    std::unique_lock lock(core::mutex::Tracking);
    return glm::quat_cast(_worldTransformPrevious);
}

glm::mat4 TrackingDevice::worldTransform() const {
    std::unique_lock lock(core::mutex::Tracking);
    return _worldTransform;
}

glm::mat4 TrackingDevice::worldTransformPrevious() const {
    std::unique_lock lock(core::mutex::Tracking);
    return _worldTransformPrevious;
}

glm::dquat TrackingDevice::sensorRotation() const {
    std::unique_lock lock(core::mutex::Tracking);
    return _sensorRotation;
}

glm::dquat TrackingDevice::sensorRotationPrevious() const {
    std::unique_lock lock(core::mutex::Tracking);
    return _sensorRotationPrevious;
}

glm::dvec3 TrackingDevice::sensorPosition() const {
    std::unique_lock lock(core::mutex::Tracking);
    return _sensorPos;
}

glm::dvec3 TrackingDevice::sensorPositionPrevious() const {
    std::unique_lock lock(core::mutex::Tracking);
    return _sensorPosPrevious;
}

bool TrackingDevice::isEnabled() const {
    std::unique_lock lock(core::mutex::Tracking);
    return _isEnabled;
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
    std::unique_lock lock(core::mutex::Tracking);
    _trackerTimePrevious = _trackerTime;
    _trackerTime = Engine::getTime();
}

void TrackingDevice::setAnalogTimeStamp() {
    std::unique_lock lock(core::mutex::Tracking);
    _analogTimePrevious = _analogTime;
    _analogTime = Engine::getTime();
}

void TrackingDevice::setButtonTimeStamp(int index) {
    std::unique_lock lock(core::mutex::Tracking);
    _buttonTimePrevious[index] = _buttonTime[index];
    _buttonTime[index] = Engine::getTime();
}

double TrackingDevice::trackerTimeStamp() {
    std::unique_lock lock(core::mutex::Tracking);
    return _trackerTime;
}

double TrackingDevice::trackerTimeStampPrevious() {
    std::unique_lock lock(core::mutex::Tracking);
    return _trackerTimePrevious;
}

double TrackingDevice::analogTimeStamp() const {
    std::unique_lock lock(core::mutex::Tracking);
    return _analogTime;
}

double TrackingDevice::analogTimeStampPrevious() const {
    std::unique_lock lock(core::mutex::Tracking);
    return _analogTimePrevious;
}

double TrackingDevice::buttonTimeStamp(int index) const {
    std::unique_lock lock(core::mutex::Tracking);
    return _buttonTime[index];
}

double TrackingDevice::buttonTimeStampPrevious(int index) const {
    std::unique_lock lock(core::mutex::Tracking);
    return _buttonTimePrevious[index];
}

double TrackingDevice::trackerDeltaTime() const {
    std::unique_lock lock(core::mutex::Tracking);
    return _trackerTime - _trackerTimePrevious;
}

double TrackingDevice::analogDeltaTime() const {
    std::unique_lock lock(core::mutex::Tracking);
    return _analogTime - _analogTimePrevious;
}

double TrackingDevice::buttonDeltaTime(int index) const {
    std::unique_lock lock(core::mutex::Tracking);
    return _buttonTime[index] - _buttonTimePrevious[index];
}

} // namespace sgct
