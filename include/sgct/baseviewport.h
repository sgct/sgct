/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__BASE_VIEWPORT__H__
#define __SGCT__BASE_VIEWPORT__H__

#include <sgct/frustum.h>
#include <sgct/projection.h>
#include <sgct/projectionplane.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>

namespace sgct::core {

class User;

/**
 * This class holds and manages viewportdata and calculates frustums
 */
class BaseViewport {
public:
    BaseViewport();
    virtual ~BaseViewport() = default;

    /// Name this viewport
    void setName(std::string name);
    void setPos(glm::vec2 position);
    void setSize(glm::vec2 size);
    void setEnabled(bool state);
    void setUser(User& user);
    void setUserName(std::string userName);
    void setEye(Frustum::Mode eye);
    
    const std::string& getName() const;
    const glm::vec2& getPosition() const;
    const glm::vec2& getSize() const;
    float getHorizontalFieldOfViewDegrees() const;
    
    User& getUser() const;
    Frustum::Mode getEye() const;
    Projection& getProjection(Frustum::Mode frustumMode);
    const Projection& getProjection(Frustum::Mode frustumMode) const;
    Projection& getProjection();
    ProjectionPlane& getProjectionPlane();
    glm::quat getRotation() const;
    glm::vec4 getFOV() const;
    float getDistance() const;
    
    bool isEnabled() const;
    void linkUserName();

    void calculateFrustum(Frustum::Mode mode, float nearClip, float farClip);

    /// Make projection symmetric relative to user
    void calculateNonLinearFrustum(Frustum::Mode mode, float nearClip, float farClip);
    void setViewPlaneCoordsUsingFOVs(float up, float down, float left, float right,
        glm::quat rot, float dist = 10.f);
    void setViewPlaneCoordsFromUnTransformedCoords(glm::vec3 lowerLeft,
        glm::vec3 upperLeft, glm::vec3 upperRight, const glm::quat& rot);
    void updateFovToMatchAspectRatio(float oldRatio, float newRatio);
    void setHorizontalFieldOfView(float horizFovDeg, float aspectRatio);

protected:
    struct {
        Projection mono;
        Projection stereoLeft;
        Projection stereoRight;
    } mProjections;
    
    ProjectionPlane mProjectionPlane;
    Frustum::Mode mEye = Frustum::MonoEye;

    User& mUser;
    std::string mName = "NoName";
    std::string mUserName;
    bool mEnabled = true;
    glm::vec2 mPosition = glm::vec2(0.f, 0.f);
    glm::vec2 mSize = glm::vec2(1.f, 1.f);

    struct {
        glm::vec3 lowerLeft;
        glm::vec3 upperLeft;
        glm::vec3 upperRight;
    } mUnTransformedViewPlaneCoords;
    glm::quat mRot;
    float mDistance = 10.f;
    glm::vec4 mFOV;
};

} // namespace sgct::core

#endif // __SGCT__BASE_VIEWPORT__H__
