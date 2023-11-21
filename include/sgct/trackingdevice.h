/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__TRACKINGDEVICE__H__
#define __SGCT__TRACKINGDEVICE__H__

#include <sgct/sgctexports.h>
#include <sgct/math.h>
#include <string>
#include <vector>

namespace sgct {

/**
 * Helper class that holds tracking device/sensor data.
 */
class SGCT_EXPORT TrackingDevice {
public:
    /**
     * Constructor.
     */
    TrackingDevice(int parentIndex, std::string name);

    /**
     * Set if this device is enabled or not.
     */
    void setEnabled(bool state);

    /**
     * Set the id for this sensor.
     */
    void setSensorId(int id);

    /**
     * Set the number of digital buttons.
     */
    void setNumberOfButtons(int numOfButtons);

    /**
     * Set the number of analog axes.
     */
    void setNumberOfAxes(int numOfAxes);
    void setSensorTransform(vec3 vec, quat rot);
    void setButtonValue(bool val, int index);
    void setAnalogValue(const double* array, int size);

    /**
     * Set the orientation euler angles (degrees) used to generate the orientation matrix.
     */
    void setOrientation(float xRot, float yRot, float zRot);

    /**
     * Set the orientation quaternion used to generate the orientation matrix.
     */
    void setOrientation(quat q);

    /**
     * Set the offset vector used to generate the offset matrix.
     */
    void setOffset(vec3 offset);

    /**
     * Set the device transform matrix.
     */
    void setTransform(mat4 mat);

    const std::string& name() const;
    int numberOfButtons() const;
    int numberOfAxes() const;

    /**
     * \return A digital value from array
     */
    bool button(int index) const;

    /**
     * \return A digital value from array
     */
    bool buttonPrevious(int index) const;

    /**
     * \return An analog value from array
     */
    double analog(int index) const;

    /**
     * \return An analog value from array
     */
    double analogPrevious(int index) const;

    bool isEnabled() const;
    bool hasSensor() const;
    bool hasButtons() const;
    bool hasAnalogs() const;

    /**
     * \return The id of this device/sensor
     */
    int sensorId();

    /**
     * \return The sensor's position in world coordinates
     */
    vec3 position() const;

    /**
     * \return The sensor's position in world coordinates
     */
    vec3 previousPosition() const;

    /**
     * \return The sensor's rotation as as euler angles in world coordinates
     */
    vec3 eulerAngles() const;

    /**
     * \return The sensor's rotation as as euler angles in world coordinates
     */
    vec3 eulerAnglesPrevious() const;

    /**
     * \return The sensor's rotation as a quaternion in world coordinates
     */
    quat rotation() const;

    /**
     * \return The sensor's rotation as a quaternion in world coordinates
     */
    quat rotationPrevious() const;

    /**
     * \return The sensor's transform matrix in world coordinates
     */
    mat4 worldTransform() const;

    /**
     * \return The sensor's transform matrix in world coordinates
     */
    mat4 worldTransformPrevious() const;

    /**
     * \return The raw sensor rotation quaternion
     */
    quat sensorRotation() const;

    /**
     * \return The raw sensor rotation quaternion
     */
    quat sensorRotationPrevious() const;

    /**
     * \return The raw sensor position vector
     */
    vec3 sensorPosition() const;

    /**
     * \return The raw sensor position vector
     */
    vec3 sensorPositionPrevious() const;

    double trackerTimeStamp();
    double trackerTimeStampPrevious();

    double analogTimeStamp() const;
    double analogTimeStampPrevious() const;
    double buttonTimeStamp(int index) const;
    double buttonTimeStampPrevious(int index) const;

    double trackerDeltaTime() const;
    double analogDeltaTime() const;
    double buttonDeltaTime(int index) const;

private:
    void calculateTransform();
    void setTrackerTimeStamp();
    void setAnalogTimeStamp();
    void setButtonTimeStamp(int index);

    bool _isEnabled = true;
    const std::string _name;
#ifdef SGCT_HAS_VRPN
    const int _parentIndex; // the index of parent Tracker
#endif // SGCT_HAS_VRPN
    int _nButtons = 0;
    int _nAxes = 0;
    int _sensorId = -1;

    mat4 _deviceTransform = mat4(1.f);

    mat4 _worldTransform = mat4(1.f);
    mat4 _worldTransformPrevious = mat4(1.f);

    quat _sensorRotation = quat{ 0.f, 0.f, 0.f, 0.f };
    quat _sensorRotationPrevious = quat{ 0.f, 0.f, 0.f, 0.f };

    vec3 _sensorPos = vec3{ 0.f, 0.f, 0.f };
    vec3 _sensorPosPrevious = vec3{ 0.f, 0.f, 0.f };
    quat _orientation = quat{ 0.f, 0.f, 0.f, 0.f };
    vec3 _offset = vec3{ 0.f, 0.f, 0.f };

    double _trackerTime = 0.0;
    double _trackerTimePrevious = 0.0;

    double _analogTime = 0.0;
    double _analogTimePrevious = 0.0;

    std::vector<double> _buttonTime;
    std::vector<double> _buttonTimePrevious;
    std::vector<bool> _buttons;
    std::vector<bool> _buttonsPrevious;
    std::vector<double> _axes;
    std::vector<double> _axesPrevious;
};

} // namespace sgct

#endif // __SGCT__TRACKINGDEVICE__H__
