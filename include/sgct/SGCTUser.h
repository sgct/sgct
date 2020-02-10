/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_USER_H_
#define _SGCT_USER_H_

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include "Frustum.h"

namespace sgct_core
{

/*!
Helper class for setting user variables
*/
class SGCTUser
{
public:
    SGCTUser(std::string name);

    void setPos(float x, float y, float z);
    void setPos(glm::vec3 pos);
    void setPos(glm::dvec4 pos);
    void setPos(float * pos);
    void setHeadTracker(const char * trackerName, const char * deviceName);

    void setTransform(const glm::mat4 & transform);
    void setTransform(const glm::dmat4 & transform);
    void setOrientation(float xRot, float yRot, float zRot);
    void setOrientation(glm::quat q);
    void setEyeSeparation(float eyeSeparation);

    std::string getName();
    const glm::vec3 & getPos(Frustum::FrustumMode fm = Frustum::MonoEye);
    glm::vec3 * getPosPtr() { return &mPos[Frustum::MonoEye]; }
    glm::vec3 * getPosPtr(Frustum::FrustumMode fm) { return &mPos[fm]; }

    inline const float & getEyeSeparation() { return mEyeSeparation; }
    inline const float & getHalfEyeSeparation() { return mHalfEyeSeparation; }
    inline const float & getXPos() { return mPos[Frustum::MonoEye].x; }
    inline const float & getYPos() { return mPos[Frustum::MonoEye].y; }
    inline const float & getZPos() { return mPos[Frustum::MonoEye].z; }
    inline const char * getHeadTrackerName() { return mHeadTrackerName.c_str(); }
    inline const char * getHeadTrackerDeviceName() { return mHeadTrackerDeviceName.c_str(); }

    bool isTracked() const;

private:
    void updateEyeSeparation();
    void updateEyeTransform();

private:
    glm::vec3 mPos[3];
    glm::mat4 mTransform = glm::mat4(1.f);

    float mEyeSeparation;
    float mHalfEyeSeparation;

    std::string mName;
    std::string mHeadTrackerDeviceName;
    std::string mHeadTrackerName;

};

} // sgct_core

#endif
