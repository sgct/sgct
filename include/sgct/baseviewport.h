/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__BASEVIEWPORT__H__
#define __SGCT__BASEVIEWPORT__H__

#include <sgct/definitions.h>

#include <sgct/sgctexports.h>
#include <sgct/math.h>
#include <sgct/projection.h>
#include <sgct/projection/projectionplane.h>

namespace sgct {

class Window;
class User;

/**
 * This class holds and manages viewportdata and calculates frustums.
 */
class SGCT_EXPORT BaseViewport {
public:
    explicit BaseViewport(const Window& parent, FrustumMode eye = FrustumMode::Mono);
    virtual ~BaseViewport();

    void setupViewport(FrustumMode frustum) const;

    const Projection& projection(FrustumMode frustumMode) const;
    ProjectionPlane& projectionPlane();

    virtual void calculateFrustum(FrustumMode mode, float nearClip, float farClip);

    /**
     * Make projection symmetric relative to user.
     */
    void calculateNonLinearFrustum(FrustumMode mode, float nearClip, float farClip);
    void setViewPlaneCoordsUsingFOVs(float up, float down, float left, float right,
        quat rot, float dist = 10.f);
    void updateFovToMatchAspectRatio(float oldRatio, float newRatio);

    void setEnabled(bool state);
    bool isEnabled() const;

    void setPosition(vec2 position);
    const vec2& position() const;

    void setSize(vec2 size);
    const vec2& size() const;

    void setUser(User& user);
    User& user() const;

    FrustumMode eye() const;

    void setHorizontalFieldOfView(float hFov);
    float horizontalFieldOfViewDegrees() const;

    const Window& window() const;

protected:
    const Window& _parent;

    Projection _monoProj;
    Projection _stereoLeftProj;
    Projection _stereoRightProj;

    ProjectionPlane _projPlane;

    bool _isEnabled = true;
    const FrustumMode _eye;

    vec2 _position = vec2{ 0.f, 0.f };
    vec2 _size = vec2{ 1.f, 1.f };

    User* _user;

    struct {
        vec3 lowerLeft = vec3{ 0.f, 0.f, 0.f };
        vec3 upperLeft = vec3{ 0.f, 0.f, 0.f };
        vec3 upperRight = vec3{ 0.f, 0.f, 0.f };
    } _viewPlane;
    quat _rotation = quat{ 0.f, 0.f, 0.f, 1.f };
};

} // namespace sgct

#endif // __SGCT__BASEVIEWPORT__H__
