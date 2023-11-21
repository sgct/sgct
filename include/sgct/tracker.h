/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__TRACKER__H__
#define __SGCT__TRACKER__H__

#include <sgct/sgctexports.h>
#include <sgct/math.h>
#include <sgct/trackingdevice.h>
#include <memory>
#include <string_view>
#include <vector>

namespace sgct {

/**
 * Class that manages a tracking system's properties and devices/sensors.
 */
class SGCT_EXPORT Tracker {
public:
    explicit Tracker(std::string name);
    Tracker(const Tracker&) = delete;
    Tracker(Tracker&&) = default;
    Tracker& operator=(const Tracker&) = delete;
    Tracker& operator=(Tracker&&) = delete;

    void setEnabled(bool state);
    void addDevice(std::string name, int index);

    const std::vector<std::unique_ptr<TrackingDevice>>& devices() const;

    TrackingDevice* device(std::string_view name) const;
    TrackingDevice* deviceBySensorId(int id) const;

    /**
     * Set the orientation as quaternion.
     */
    void setOrientation(quat q);

    /**
     * Set the orientation as euler angles (degrees).
     */
    void setOrientation(float xRot, float yRot, float zRot);
    void setOffset(vec3 offset);
    void setScale(double scaleVal);

    /**
     * Set the tracker system transform matrix:
     * `worldTransform = (trackerTransform * sensorMat) * deviceTransformMat`
     */
    void setTransform(mat4 mat);

    mat4 getTransform() const;
    double scale() const;

    const std::string& name() const;

private:
    const std::string _name;

    std::vector<std::unique_ptr<TrackingDevice>> _trackingDevices;

    double _scale = 1.0;
    mat4 _transform = mat4(1.f);
    mat4 _orientation = mat4(1.f);
    vec3 _offset = vec3{ 0.f, 0.f, 0.f };
};

} // namespace sgct

#endif // __SGCT__TRACKER__H__
