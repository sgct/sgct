/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__BASEVIEWPORT__H__
#define __SGCT__BASEVIEWPORT__H__

#include <sgct/sgctexports.h>
#include <sgct/frustum.h>
#include <sgct/math.h>
#include <sgct/projection.h>
#include <sgct/projection/projectionplane.h>
#include <string>

namespace sgct {

class Window;
class User;

/**
 * This class holds and manages viewportdata and calculates frustums.
 */
class SGCT_EXPORT BaseViewport {
public:
    BaseViewport(const Window* parent);
    virtual ~BaseViewport();

    void setPos(vec2 position);
    void setSize(vec2 size);
    void setEnabled(bool state);
    void setUser(User* user);
    void setUserName(std::string userName);
    void setEye(Frustum::Mode eye);

    const vec2& position() const;
    const vec2& size() const;
    float horizontalFieldOfViewDegrees() const;

    User& user() const;
    const Window& window() const;
    Frustum::Mode eye() const;

    const Projection& projection(Frustum::Mode frustumMode) const;
    ProjectionPlane& projectionPlane();

    bool isEnabled() const;
    void linkUserName();

    void calculateFrustum(Frustum::Mode mode, float nearClip, float farClip);

    /**
     * Make projection symmetric relative to user.
     */
    void calculateNonLinearFrustum(Frustum::Mode mode, float nearClip, float farClip);
    void setViewPlaneCoordsUsingFOVs(float up, float down, float left, float right,
        quat rot, float dist = 10.f);
    void updateFovToMatchAspectRatio(float oldRatio, float newRatio);
    void setHorizontalFieldOfView(float hFov);

protected:
    const Window* _parent = nullptr;

    Projection _monoProj;
    Projection _stereoLeftProj;
    Projection _stereoRightProj;

    ProjectionPlane _projPlane;
    Frustum::Mode _eye = Frustum::Mode::MonoEye;

    User* _user;

    std::string _userName;
    bool _isEnabled = true;
    vec2 _position = vec2{ 0.f, 0.f };
    vec2 _size = vec2{ 1.f, 1.f };

    struct {
        vec3 lowerLeft = vec3{ 0.f, 0.f, 0.f };
        vec3 upperLeft = vec3{ 0.f, 0.f, 0.f };
        vec3 upperRight = vec3{ 0.f, 0.f, 0.f };
    } _viewPlane;
    quat _rotation = quat{ 0.f, 0.f, 0.f, 1.f };
};

} // namespace sgct

#endif // __SGCT__BASEVIEWPORT__H__
