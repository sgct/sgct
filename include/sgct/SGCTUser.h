/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__USER__H__
#define __SGCT__USER__H__

#include <glm/glm.hpp>
#include <string>

namespace sgct_core {

/**
 * Helper class for setting user variables
 */
class SGCTUser {
public:
    /// Default contructor
    SGCTUser(std::string name);

    /// Sets user's head position
    void setPos(glm::vec3 pos);

    /**
     * Set if the user's head position & orientation should be managed by a VRPN tracking
     * device. This is normally done using the XML configuration file.
     *
     * \param trackerName the pointer to the tracker
     * \param deviceName the name of the device which is mapped to the tracker
     */
    void setHeadTracker(std::string trackerName, std::string deviceName);

    /**
     * Set the user's head transform matrix.
     *
     * \param transform the transform matrix
     */
    void setTransform(glm::mat4 transform);

    /**
     * Set the user's head orientation using euler angles. Note that rotations are
     * dependent of each other, total rotation = xRot * yRot * zRot.
     *
     * \param xRot the rotation around the x-axis
     * \param yRot the rotation around the y-axis
     * \param zRot the rotation around the z-axis
     */
    void setOrientation(float xRot, float yRot, float zRot);

    /// Set the user's head orientation using a quaternion
    void setOrientation(glm::quat q);

    /// Changes the interocular distance and recalculates the user's eye positions.
    void setEyeSeparation(float eyeSeparation);

    /// Get the users name
    const std::string& getName() const;

    const glm::vec3& getPosMono() const;
    const glm::vec3& getPosLeftEye() const;
    const glm::vec3& getPosRightEye() const;

    float getEyeSeparation() const;
    const std::string& getHeadTrackerName() const;
    const std::string& getHeadTrackerDeviceName() const;

    /// \returns true if user is tracked
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

    glm::vec3 mPosMono = glm::vec3(0.f);
    glm::vec3 mPosLeftEye = glm::vec3(0.f);
    glm::vec3 mPosRightEye = glm::vec3(0.f);

    glm::mat4 mTransform = glm::mat4(1.0);
    float mEyeSeparation = 0.06f;

    std::string mName;
    std::string mHeadTrackerDeviceName;
    std::string mHeadTrackerName;
};

} // sgct_core

#endif // __SGCT__USER__H__
