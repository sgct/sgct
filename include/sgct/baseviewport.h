/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

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

    void setPos(glm::vec2 position);
    void setSize(glm::vec2 size);
    void setEnabled(bool state);
    void setUser(User* user);
    void setUserName(std::string userName);
    void setEye(Frustum::Mode eye);
    
    const glm::vec2& getPosition() const;
    const glm::vec2& getSize() const;
    float getHorizontalFieldOfViewDegrees() const;

    User& getUser() const;
    Frustum::Mode getEye() const;

    const Projection& getProjection(Frustum::Mode frustumMode) const;
    ProjectionPlane& getProjectionPlane();

    bool isEnabled() const;
    void linkUserName();

    void calculateFrustum(Frustum::Mode mode, float nearClip, float farClip);

    /// Make projection symmetric relative to user
    void calculateNonLinearFrustum(Frustum::Mode mode, float nearClip, float farClip);
    void setViewPlaneCoordsUsingFOVs(float up, float down, float left, float right,
        glm::quat rot, float dist = 10.f);
    void updateFovToMatchAspectRatio(float oldRatio, float newRatio);
    void setHorizontalFieldOfView(float hFov);

protected:
    struct {
        Projection mono;
        Projection stereoLeft;
        Projection stereoRight;
    } _projections;
    
    ProjectionPlane _projectionPlane;
    Frustum::Mode _eye = Frustum::Mode::MonoEye;

    User* _user;

    std::string _userName;
    bool _isEnabled = true;
    glm::vec2 _position = glm::vec2(0.f, 0.f);
    glm::vec2 _size = glm::vec2(1.f, 1.f);

    struct {
        glm::vec3 lowerLeft;
        glm::vec3 upperLeft;
        glm::vec3 upperRight;
    } _viewPlane;
    glm::quat _rotation;
};

} // namespace sgct::core

#endif // __SGCT__BASE_VIEWPORT__H__
