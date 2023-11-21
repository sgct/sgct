/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__USER__H__
#define __SGCT__USER__H__

#include <sgct/sgctexports.h>
#include <sgct/math.h>
#include <string>

namespace sgct {

/**
 * Helper class for setting user variables.
 */
class SGCT_EXPORT User {
public:
    /**
     * Default contructor.
     */
    User(std::string name);

    /**
     * Sets user's head position.
     */
    void setPos(vec3 pos);

    /**
     * Set if the user's head position & orientation should be managed by a VRPN tracking
     * device. This is normally done using the configuration file.
     *
     * \param trackerName The pointer to the tracker
     * \param deviceName The name of the device which is mapped to the tracker
     */
    void setHeadTracker(std::string trackerName, std::string deviceName);

    /**
     * Set the user's head transform matrix.
     *
     * \param transform The transform matrix
     */
    void setTransform(mat4 transform);

    /**
     * Set the user's head orientation using euler angles. Note that rotations are
     * dependent of each other, total `rotation = xRot * yRot * zRot`.
     *
     * \param xRot, yRot, zRot The rotations around the x, y, and z axes
     */
    void setOrientation(float xRot, float yRot, float zRot);

    /**
     * Set the user's head orientation using a quaternion.
     */
    void setOrientation(quat q);

    /**
     * Changes the interocular distance and recalculates the user's eye positions.
     */
    void setEyeSeparation(float eyeSeparation);

    /**
     * Get the users name.
     */
    const std::string& name() const;

    const vec3& posMono() const;
    const vec3& posLeftEye() const;
    const vec3& posRightEye() const;

    float eyeSeparation() const;
    const std::string& headTrackerName() const;
    const std::string& headTrackerDeviceName() const;

    /**
     * \return `true` if user is tracked
     */
    bool isTracked() const;

private:
    /**
     * Recalculates the user's eye positions based on head position and eye separation
     * (interocular distance).
     */
    void updateEyeSeparation();

    /**
     * Recalculates the user's eye orientations based on head position and eye separation
     * (interocular distance).
     */
    void updateEyeTransform();

    vec3 _posMono = vec3{ 0.f, 0.f, 0.f };
    vec3 _posLeftEye = vec3{ 0.f, 0.f, 0.f };
    vec3 _posRightEye = vec3{ 0.f, 0.f, 0.f };

    mat4 _transform = mat4(1.0);
    float _eyeSeparation = 0.06f;

    const std::string _name;
    std::string _headTrackerDeviceName;
    std::string _headTrackerName;
};

} // namespace sgct

#endif // __SGCT__USER__H__
