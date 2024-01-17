/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/trackingdevice.h>

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/fmt.h>
#include <sgct/log.h>
#include <sgct/mutexes.h>
#include <sgct/tracker.h>
#ifdef SGCT_HAS_VRPN
#include <sgct/trackingmanager.h>
#endif
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

namespace sgct {

TrackingDevice::TrackingDevice([[maybe_unused]] int parentIndex, std::string name)
    : _name(std::move(name))
#ifdef SGCT_HAS_VRPN
    , _parentIndex(parentIndex)
#endif // SGCT_HAS_VRPN
{}

void TrackingDevice::setEnabled(bool state) {
    std::unique_lock lock(mutex::Tracking);
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

void TrackingDevice::setSensorTransform(vec3 vec, quat rot) {
#ifdef SGCT_HAS_VRPN
    Tracker* parent = TrackingManager::instance().trackers()[_parentIndex].get();
#else
    Tracker* parent = nullptr;
#endif

    if (parent == nullptr) {
        Log::Error(fmt::format("Error getting handle to tracker for device '{}'", _name));
        return;
    }

    const glm::mat4 parentTrans = glm::make_mat4(parent->getTransform().values);

    // create matrixes
    const glm::mat4 sensorTransMat = glm::translate(
        glm::mat4(1.f),
        glm::make_vec3(&vec.x)
    );
    const glm::mat4 sensorRotMat = glm::mat4_cast(glm::make_quat(&rot.x));

    {
        std::unique_lock lock(mutex::Tracking);

        // swap
        _sensorRotationPrevious = std::move(_sensorRotation);
        _sensorRotation = std::move(rot);

        _sensorPosPrevious = std::move(_sensorPos);
        _sensorPos = std::move(vec);

        _worldTransformPrevious = std::move(_worldTransform);
        glm::mat4 m = parentTrans * sensorTransMat * sensorRotMat *
                      glm::make_mat4(_deviceTransform.values);
        std::memcpy(&_worldTransform, glm::value_ptr(m), sizeof(float[16]));
    }
    setTrackerTimeStamp();
}

void TrackingDevice::setButtonValue(bool val, int index) {
    if (index >= _nButtons) {
        return;
    }

    {
        std::unique_lock lock(mutex::Tracking);
        // swap
        _buttonsPrevious[index] = _buttons[index];
        _buttons[index] = val;
    }
    setButtonTimeStamp(index);
}

void TrackingDevice::setAnalogValue(const double* array, int size) {
    {
        std::unique_lock lock(mutex::Tracking);
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

    std::unique_lock lock(mutex::Tracking);
    _orientation = sgct::quat(rotQuat.x, rotQuat.y, rotQuat.z, rotQuat.w);
    calculateTransform();
}

void TrackingDevice::setOrientation(quat q) {
    std::unique_lock lock(mutex::Tracking);
    _orientation = std::move(q);
    calculateTransform();
}

void TrackingDevice::setOffset(vec3 offset) {
    std::unique_lock lock(mutex::Tracking);
    _offset = std::move(offset);
    calculateTransform();
}

void TrackingDevice::setTransform(mat4 mat) {
    std::unique_lock lock(mutex::Tracking);
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
    glm::mat4 transMat = glm::translate(glm::mat4(1.f), glm::make_vec3(&_offset.x)) *
        glm::mat4_cast(glm::make_quat(&_orientation.x));
    std::memcpy(&_deviceTransform, glm::value_ptr(transMat), sizeof(float[16]));
}

int TrackingDevice::sensorId() {
    std::unique_lock lock(mutex::Tracking);
    return _sensorId;
}

bool TrackingDevice::button(int index) const {
    std::unique_lock lock(mutex::Tracking);
    return index < _nButtons ? _buttons[index] : false;
}

bool TrackingDevice::buttonPrevious(int index) const {
    std::unique_lock lock(mutex::Tracking);
    return index < _nButtons ? _buttonsPrevious[index] : false;
}

double TrackingDevice::analog(int index) const {
    std::unique_lock lock(mutex::Tracking);
    return index < _nAxes ? _axes[index] : 0.0;
}

double TrackingDevice::analogPrevious(int index) const {
    std::unique_lock lock(mutex::Tracking);
    return index < _nAxes ? _axesPrevious[index] : 0.0;
}

vec3 TrackingDevice::position() const {
    std::unique_lock lock(mutex::Tracking);
    glm::mat4 m = glm::make_mat4(_worldTransform.values);
    glm::vec3 p = glm::vec3(m[3]);
    return sgct::vec3(p.x, p.y, p.z);
}

vec3 TrackingDevice::previousPosition() const {
    std::unique_lock lock(mutex::Tracking);
    glm::mat4 m = glm::make_mat4(_worldTransformPrevious.values);
    glm::vec3 p = glm::vec3(m[3]);
    return sgct::vec3(p.x, p.y, p.z);
}

vec3 TrackingDevice::eulerAngles() const {
    std::unique_lock lock(mutex::Tracking);
    glm::vec3 v =
        glm::eulerAngles(glm::quat_cast(glm::make_mat4(_worldTransform.values)));
    return sgct::vec3(v.x, v.y, v.z);
}

vec3 TrackingDevice::eulerAnglesPrevious() const {
    std::unique_lock lock(mutex::Tracking);
    glm::vec3 v =
        glm::eulerAngles(glm::quat_cast(glm::make_mat4(_worldTransformPrevious.values)));
    return sgct::vec3(v.x, v.y, v.z);
}

quat TrackingDevice::rotation() const {
    std::unique_lock lock(mutex::Tracking);
    glm::quat q = glm::quat_cast(glm::make_mat4(_worldTransform.values));
    return quat(q.x, q.y, q.z, q.w);
}

quat TrackingDevice::rotationPrevious() const {
    std::unique_lock lock(mutex::Tracking);
    glm::quat q = glm::quat_cast(glm::make_mat4(_worldTransformPrevious.values));
    return quat(q.x, q.y, q.z, q.w);
}

mat4 TrackingDevice::worldTransform() const {
    std::unique_lock lock(mutex::Tracking);
    return _worldTransform;
}

mat4 TrackingDevice::worldTransformPrevious() const {
    std::unique_lock lock(mutex::Tracking);
    return _worldTransformPrevious;
}

quat TrackingDevice::sensorRotation() const {
    std::unique_lock lock(mutex::Tracking);
    return _sensorRotation;
}

quat TrackingDevice::sensorRotationPrevious() const {
    std::unique_lock lock(mutex::Tracking);
    return _sensorRotationPrevious;
}

vec3 TrackingDevice::sensorPosition() const {
    std::unique_lock lock(mutex::Tracking);
    return _sensorPos;
}

vec3 TrackingDevice::sensorPositionPrevious() const {
    std::unique_lock lock(mutex::Tracking);
    return _sensorPosPrevious;
}

bool TrackingDevice::isEnabled() const {
    std::unique_lock lock(mutex::Tracking);
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
    std::unique_lock lock(mutex::Tracking);
    _trackerTimePrevious = _trackerTime;
    _trackerTime = time();
}

void TrackingDevice::setAnalogTimeStamp() {
    std::unique_lock lock(mutex::Tracking);
    _analogTimePrevious = _analogTime;
    _analogTime = time();
}

void TrackingDevice::setButtonTimeStamp(int index) {
    std::unique_lock lock(mutex::Tracking);
    _buttonTimePrevious[index] = _buttonTime[index];
    _buttonTime[index] = time();
}

double TrackingDevice::trackerTimeStamp() {
    std::unique_lock lock(mutex::Tracking);
    return _trackerTime;
}

double TrackingDevice::trackerTimeStampPrevious() {
    std::unique_lock lock(mutex::Tracking);
    return _trackerTimePrevious;
}

double TrackingDevice::analogTimeStamp() const {
    std::unique_lock lock(mutex::Tracking);
    return _analogTime;
}

double TrackingDevice::analogTimeStampPrevious() const {
    std::unique_lock lock(mutex::Tracking);
    return _analogTimePrevious;
}

double TrackingDevice::buttonTimeStamp(int index) const {
    std::unique_lock lock(mutex::Tracking);
    return _buttonTime[index];
}

double TrackingDevice::buttonTimeStampPrevious(int index) const {
    std::unique_lock lock(mutex::Tracking);
    return _buttonTimePrevious[index];
}

double TrackingDevice::trackerDeltaTime() const {
    std::unique_lock lock(mutex::Tracking);
    return _trackerTime - _trackerTimePrevious;
}

double TrackingDevice::analogDeltaTime() const {
    std::unique_lock lock(mutex::Tracking);
    return _analogTime - _analogTimePrevious;
}

double TrackingDevice::buttonDeltaTime(int index) const {
    std::unique_lock lock(mutex::Tracking);
    return _buttonTime[index] - _buttonTimePrevious[index];
}

} // namespace sgct
