/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__USER__H__
#define __SGCT__USER__H__

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <sgct/Frustum.h>

namespace sgct_core {

/*!
Helper class for setting user variables
*/
class SGCTUser {
public:
    SGCTUser(std::string name);

    void setPos(float x, float y, float z);
    void setPos(glm::vec3 pos);
    void setPos(glm::dvec4 pos);
    void setPos(float* pos);
    void setHeadTracker(const char* trackerName, const char* deviceName);

    void setTransform(glm::mat4 transform);
    void setTransform(glm::dmat4 transform);
    void setOrientation(float xRot, float yRot, float zRot);
    void setOrientation(glm::quat q);
    void setEyeSeparation(float eyeSeparation);

    std::string getName();
    const glm::vec3 & getPos(Frustum::FrustumMode fm = Frustum::MonoEye);
    glm::vec3* getPosPtr();
    glm::vec3* getPosPtr(Frustum::FrustumMode fm);

    float getEyeSeparation();
    float getHalfEyeSeparation();
    float getXPos();
    float getYPos();
    float getZPos();
    const char* getHeadTrackerName();
    const char* getHeadTrackerDeviceName();

    bool isTracked() const;

private:
    void updateEyeSeparation();
    void updateEyeTransform();

private:
    glm::vec3 mPos[3] = { glm::vec3(0.f), glm::vec3(0.f), glm::vec3(0.f) };
    glm::mat4 mTransform = glm::dmat4(1.0);

    float mEyeSeparation = 0.06f;

    std::string mName;
    std::string mHeadTrackerDeviceName;
    std::string mHeadTrackerName;

};

} // sgct_core

#endif // __SGCT__USER__H__
