/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/tracker.h>

#include <sgct/engine.h>
#include <sgct/format.h>
#include <sgct/log.h>
#include <sgct/mutexes.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
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
    Log::Info(std::format("{}: Adding device '{}'", _name, name));
}

const std::vector<std::unique_ptr<TrackingDevice>>& Tracker::devices() const {
    return _trackingDevices;
}

TrackingDevice* Tracker::device(std::string_view name) const {
    const auto it = std::find_if(
        _trackingDevices.cbegin(),
        _trackingDevices.cend(),
        [name](const std::unique_ptr<TrackingDevice>& d) { return d->name() == name; }
    );
    return it != _trackingDevices.cend() ? it->get() : nullptr;
}

TrackingDevice* Tracker::deviceBySensorId(int id) const {
    const auto it = std::find_if(
        _trackingDevices.cbegin(),
        _trackingDevices.cend(),
        [id](const std::unique_ptr<TrackingDevice>& d) { return d->sensorId() == id; }
    );
    return it != _trackingDevices.cend() ? it->get() : nullptr;
}

void Tracker::setOrientation(quat q) {
    const std::unique_lock lock(mutex::Tracking);

    // create inverse rotation matrix
    glm::mat4 orientation = glm::inverse(glm::mat4_cast(glm::make_quat(&q.x)));
    std::memcpy(&_orientation, glm::value_ptr(orientation), 16 * sizeof(float));

    glm::mat4 transMat = glm::translate(glm::mat4(1.f), glm::make_vec3(&_offset.x));
    std::memcpy(&_transform, glm::value_ptr(transMat), 16 * sizeof(float));
}

void Tracker::setOrientation(float xRot, float yRot, float zRot) {
    glm::quat rotQuat = glm::quat(1.f, 0.f, 0.f, 0.f);
    rotQuat = glm::rotate(rotQuat, glm::radians(xRot), glm::vec3(1.f, 0.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(yRot), glm::vec3(0.f, 1.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(zRot), glm::vec3(0.f, 0.f, 1.f));
    setOrientation(sgct::quat(rotQuat.x, rotQuat.y, rotQuat.z, rotQuat.w));
}

void Tracker::setOffset(vec3 offset) {
    const std::unique_lock lock(mutex::Tracking);
    _offset = std::move(offset);
    glm::mat4 trans =
        glm::translate(glm::mat4(1.f), glm::make_vec3(&_offset.x)) *
        glm::make_mat4(_orientation.values);
    std::memcpy(&_transform, glm::value_ptr(trans), 16 * sizeof(float));
}

void Tracker::setScale(double scaleVal) {
    const std::unique_lock lock(mutex::Tracking);
    if (scaleVal > 0.0) {
        _scale = scaleVal;
    }
}

void Tracker::setTransform(mat4 mat) {
    const std::unique_lock lock(mutex::Tracking);
    _transform = std::move(mat);
}

mat4 Tracker::transform() const {
    const std::unique_lock lock(mutex::Tracking);
    return _transform;
}

double Tracker::scale() const {
    const std::unique_lock lock(mutex::Tracking);
    return _scale;
}

const std::string& Tracker::name() const {
    return _name;
}

} // namespace sgct
