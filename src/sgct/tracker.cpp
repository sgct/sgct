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
#include <sgct/mutexes.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace sgct {

Tracker::Tracker(std::string name) : _name(std::move(name)) {}

void Tracker::setEnabled(bool state) {
    for (std::unique_ptr<TrackingDevice>& device : _trackingDevices) {
        device->setEnabled(state);
    }
}

void Tracker::addDevice(std::string name, int index) {
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
    const auto it = std::find_if(
        _trackingDevices.cbegin(),
        _trackingDevices.cend(),
        [name](const std::unique_ptr<TrackingDevice>& d) { return d->getName() == name; }
    );
    return it != _trackingDevices.cend() ? it->get() : nullptr;
}

TrackingDevice* Tracker::getDeviceBySensorId(int id) const {
    const auto it = std::find_if(
        _trackingDevices.cbegin(),
        _trackingDevices.cend(),
        [id](const std::unique_ptr<TrackingDevice>& d) { return d->getSensorId() == id; }
    );
    return it != _trackingDevices.cend() ? it->get() : nullptr;
}

void Tracker::setOrientation(glm::quat q) {
    std::unique_lock lock(core::mutex::Tracking);

    // create inverse rotation matrix
    _orientation = glm::inverse(glm::mat4_cast(q));
    const glm::mat4 transMat = glm::translate(glm::mat4(1.f), _offset);
    _transform = transMat * _orientation;
}

void Tracker::setOrientation(float xRot, float yRot, float zRot) {
    glm::quat rotQuat = glm::quat(1.f, 0.f, 0.f, 0.f);
    rotQuat = glm::rotate(rotQuat, glm::radians(xRot), glm::vec3(1.f, 0.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(yRot), glm::vec3(0.f, 1.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(zRot), glm::vec3(0.f, 0.f, 1.f));
    setOrientation(std::move(rotQuat));
}

void Tracker::setOffset(glm::vec3 offset) {
    std::unique_lock lock(core::mutex::Tracking);
    _offset = std::move(offset);
    const glm::mat4 transMat = glm::translate(glm::mat4(1.f), _offset);
    _transform = transMat * _orientation;
}

void Tracker::setScale(double scaleVal) {
    std::unique_lock lock(core::mutex::Tracking);
    if (scaleVal > 0.0) {
        _scale = scaleVal;
    }
}

void Tracker::setTransform(glm::mat4 mat) {
    std::unique_lock lock(core::mutex::Tracking);
    _transform = std::move(mat);
}

glm::mat4 Tracker::getTransform() const { 
    std::unique_lock lock(core::mutex::Tracking);
    return _transform;
}

double Tracker::getScale() const {
    std::unique_lock lock(core::mutex::Tracking);
    return _scale;
}

int Tracker::getNumberOfDevices() const {
    return static_cast<int>(_trackingDevices.size());
}

const std::string& Tracker::getName() const {
    return _name;
}

} // namespace sgct
