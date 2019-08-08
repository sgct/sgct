/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__TRACKER__H__
#define __SGCT__TRACKER__H__

#include <vector>
#include "SGCTTrackingDevice.h"

namespace sgct {

    class SGCTTrackingDevice;

/*!
Class that manages a tracking system's properties and devices/sensors
*/
class SGCTTracker {
public:
    explicit SGCTTracker(std::string name);
    ~SGCTTracker();
    void setEnabled(bool state);
    void addDevice(std::string name, size_t index);

    SGCTTrackingDevice* getLastDevicePtr();
    SGCTTrackingDevice* getDevicePtr(size_t index);
    SGCTTrackingDevice* getDevicePtr(const char* name);
    SGCTTrackingDevice* getDevicePtrBySensorId(int id);

    void setOrientation(glm::quat q);
    void setOrientation(float xRot, float yRot, float zRot);
    void setOrientation(float w, float x, float y, float z);
    void setOffset(float x, float y, float z);
    void setScale(double scaleVal);
    void setTransform(glm::mat4 mat);

    glm::mat4 getTransform();
    double getScale();

    size_t getNumberOfDevices();
    const std::string& getName();

private:
    void calculateTransform();

private:
    std::vector<SGCTTrackingDevice*> mTrackingDevices;
    std::string mName;

    double mScale = 1.0;
    glm::mat4 mXform = glm::mat4(1.f);
    glm::mat4 mOrientation;
    glm::vec3 mOffset = glm::vec3(0.f);
};

} // namespace sgct

#endif // __SGCT__TRACKER__H__
