/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__PROJECTION__H__
#define __SGCT__PROJECTION__H__

#include <sgct/sgctexports.h>

#include <sgct/glm.h>

namespace sgct {

class ProjectionPlane;

/**
 * This class holds and manages 3D projections.
 */
class SGCT_EXPORT Projection {
public:
    void calculateProjection(glm::vec3 base, const ProjectionPlane& proj, float nearClip,
        float farClip, glm::vec3 offset = glm::vec3(0.f, 0.f, 0.f));

    const glm::mat4& viewProjectionMatrix() const;
    const glm::mat4& viewMatrix() const;
    const glm::mat4& projectionMatrix() const;

private:
    struct Frustum {
        float left = -1.f;
        float right = 1.f;
        float bottom = -1.f;
        float top = 1.f;
        float nearPlane = 0.1f;
        float farPlane = 100.f;
    };

    glm::mat4 _viewMatrix = glm::mat4(1.f);
    glm::mat4 _viewProjectionMatrix = glm::mat4(1.f);
    glm::mat4 _projectionMatrix = glm::mat4(1.f);

    Frustum _frustum;
};

} // namespace sgct

#endif // __SGCT__PROJECTION__H__
