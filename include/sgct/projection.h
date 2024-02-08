/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__PROJECTION__H__
#define __SGCT__PROJECTION__H__

#include <sgct/sgctexports.h>
#include <sgct/frustum.h>
#include <sgct/math.h>

namespace sgct {

class ProjectionPlane;

/**
 * This class holds and manages 3D projections.
 */
class SGCT_EXPORT Projection {
public:
    void calculateProjection(vec3 base, const ProjectionPlane& proj, float nearClip,
        float farClip, vec3 viewOffset = vec3{ 0.f, 0.f, 0.f });

    const mat4& viewProjectionMatrix() const;
    const mat4& viewMatrix() const;
    const mat4& projectionMatrix() const;

private:
    mat4 _viewMatrix = mat4(1.f);
    mat4 _viewProjectionMatrix = mat4(1.f);
    mat4 _projectionMatrix = mat4(1.f);

    Frustum _frustum;
};

} // namespace sgct

#endif // __SGCT__PROJECTION__H__
