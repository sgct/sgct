/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__TRACKER__H__
#define __SGCT__TRACKER__H__

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace sgct {

class SGCTTrackingDevice;

/**
 * Class that manages a tracking system's properties and devices/sensors
 */
class SGCTTracker {
public:
    explicit SGCTTracker(std::string name);

    void setEnabled(bool state);
    void addDevice(std::string name, size_t index);

    SGCTTrackingDevice* getLastDevicePtr() const;
    SGCTTrackingDevice* getDevicePtr(size_t index) const;
    SGCTTrackingDevice* getDevicePtr(const std::string& name) const;
    SGCTTrackingDevice* getDevicePtrBySensorId(int id) const;

    /// Set the orientation as quaternion
    void setOrientation(glm::quat q);
    /// Set the orientation as euler angles (degrees)
    void setOrientation(float xRot, float yRot, float zRot);
    void setOffset(glm::vec3 offset);
    void setScale(double scaleVal);

    /**
     * Set the tracker system transform matrix
     * worldTransform = (trackerTransform * sensorMat) * deviceTransformMat
     */
    void setTransform(glm::mat4 mat);

    glm::mat4 getTransform() const;
    double getScale() const;

    int getNumberOfDevices() const;
    const std::string& getName() const;

private:
    void calculateTransform();

    std::vector<std::unique_ptr<SGCTTrackingDevice>> mTrackingDevices;
    std::string mName;

    double mScale = 1.0;
    glm::mat4 mXform = glm::mat4(1.f);
    glm::mat4 mOrientation;
    glm::vec3 mOffset = glm::vec3(0.f);
};

} // namespace sgct

#endif // __SGCT__TRACKER__H__
