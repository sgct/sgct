/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/tracker.h>

#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/mutexmanager.h>
#include <sgct/trackingdevice.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace sgct {

Tracker::Tracker(std::string name) : _name(std::move(name)) {}

void Tracker::setEnabled(bool state) {
    for (std::unique_ptr<TrackingDevice>& dev : _trackingDevices) {
        dev->setEnabled(state);
    }
}

void Tracker::addDevice(std::string name, size_t index) {
    _trackingDevices.push_back(std::make_unique<TrackingDevice>(index, name));

    MessageHandler::printInfo("%s: Adding device '%s'", _name.c_str(), name.c_str());
}

TrackingDevice* Tracker::getLastDevice() const {
    return !_trackingDevices.empty() ? _trackingDevices.back().get() : nullptr;
}

TrackingDevice* Tracker::getDevice(size_t index) const {
    return index < _trackingDevices.size() ? _trackingDevices[index].get() : nullptr;
}

TrackingDevice* Tracker::getDevice(const std::string& name) const {
    auto it = std::find_if(
        _trackingDevices.begin(),
        _trackingDevices.end(),
        [name](const std::unique_ptr<TrackingDevice>& dev) {
            return dev->getName() == name;
        }
    );
    if (it != _trackingDevices.end()) {
        return it->get();
    }
    else {
        return nullptr;
    }
}

TrackingDevice* Tracker::getDeviceBySensorId(int id) const {
    auto it = std::find_if(
        _trackingDevices.begin(),
        _trackingDevices.end(),
        [id](const std::unique_ptr<TrackingDevice>& dev) {
            return dev->getSensorId() == id;
        }
    );
    if (it != _trackingDevices.end()) {
        return it->get();
    }
    else {
        return nullptr;
    }
}

void Tracker::setOrientation(glm::quat q) {
    std::unique_lock lock(mutex::TrackingMutex);

    // create inverse rotation matrix
    _orientation = glm::inverse(glm::mat4_cast(q));

    calculateTransform();
}

void Tracker::setOrientation(float xRot, float yRot, float zRot) {
    // create rotation quaternion based on x, y, z rotations
    glm::quat rotQuat;
    rotQuat = glm::rotate(rotQuat, glm::radians(xRot), glm::vec3(1.f, 0.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(yRot), glm::vec3(0.f, 1.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(zRot), glm::vec3(0.f, 0.f, 1.f));
    setOrientation(std::move(rotQuat));
}

void Tracker::setOffset(glm::vec3 offset) {
    std::unique_lock lock(mutex::TrackingMutex);
    _offset = std::move(offset);
    calculateTransform();
}

void Tracker::setScale(double scaleVal) {
    std::unique_lock lock(mutex::TrackingMutex);
    if (scaleVal > 0.0) {
        _scale = scaleVal;
    }
}

void Tracker::setTransform(glm::mat4 mat) {
    std::unique_lock lock(mutex::TrackingMutex);
    _transform = std::move(mat);
}

glm::mat4 Tracker::getTransform() const { 
    std::unique_lock lock(mutex::TrackingMutex);
    return _transform;
}

double Tracker::getScale() const {
    std::unique_lock lock(mutex::TrackingMutex);
    return _scale;
}

int Tracker::getNumberOfDevices() const {
    return static_cast<int>(_trackingDevices.size());
}

const std::string& Tracker::getName() const {
    return _name;
}

void Tracker::calculateTransform() {
    // create offset translation matrix
    glm::mat4 transMat = glm::translate(glm::mat4(1.f), _offset);

    // calculate transform
    _transform = transMat * _orientation;
}

} // namespace sgct
