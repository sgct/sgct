/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__BASE_VIEWPORT__H__
#define __SGCT__BASE_VIEWPORT__H__

#include <sgct/Frustum.h>
#include <sgct/SGCTProjection.h>
#include <sgct/SGCTProjectionPlane.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>

namespace sgct_core {

class SGCTUser;

/**
 * This class holds and manages viewportdata and calculates frustums
 */
class BaseViewport {
public:
    BaseViewport();
    virtual ~BaseViewport() = default;

    /// Name this viewport
    void setName(std::string name);
    void setPos(float x, float y);
    void setSize(float x, float y);
    void setEnabled(bool state);
    void setUser(SGCTUser* user);
    void setUserName(std::string userName);
    void setEye(Frustum::FrustumMode eye);
    
    const std::string& getName() const;
    glm::vec2 getPosition() const;
    glm::vec2 getSize() const;
    float getHorizontalFieldOfViewDegrees() const;
    
    SGCTUser* getUser() const;
    Frustum::FrustumMode getEye() const;
    SGCTProjection& getProjection(Frustum::FrustumMode frustumMode);
    const SGCTProjection& getProjection(Frustum::FrustumMode frustumMode) const;
    SGCTProjection& getProjection();
    SGCTProjectionPlane& getProjectionPlane();
    glm::quat getRotation() const;
    glm::vec4 getFOV() const;
    float getDistance() const;
    
    bool isEnabled() const;
    void linkUserName();

    void calculateFrustum(const Frustum::FrustumMode& frustumMode,
        float nearClippingPlane, float farClippingPlane);

    /// Make projection symmetric relative to user
    void calculateNonLinearFrustum(const Frustum::FrustumMode& frustumMode,
        float nearClippingPlane, float farClippingPlane);
    void setViewPlaneCoordsUsingFOVs(float up, float down, float left, float right,
        glm::quat rot, float dist = 10.f);
    void setViewPlaneCoordsFromUnTransformedCoords(glm::vec3 lowerLeft,
        glm::vec3 upperLeft, glm::vec3 upperRight, const glm::quat& rot);
    void updateFovToMatchAspectRatio(float oldRatio, float newRatio);
    void setHorizontalFieldOfView(float horizFovDeg, float aspectRatio);

protected:
    SGCTProjection mProjections[3];
    SGCTProjectionPlane mProjectionPlane;
    Frustum::FrustumMode mEye = Frustum::MonoEye;

    SGCTUser* mUser;
    std::string mName = "NoName";
    std::string mUserName;
    bool mEnabled = true;
    float mX = 0.f;
    float mY = 0.f;
    float mXSize = 1.f;
    float mYSize = 1.f;

    struct {
        glm::vec3 lowerLeft;
        glm::vec3 upperLeft;
        glm::vec3 upperRight;
    } mUnTransformedViewPlaneCoords;
    glm::quat mRot;
    float mDistance;
    glm::vec4 mFOV;
};

} // namespace sgct_core

#endif // __SGCT__BASE_VIEWPORT__H__
