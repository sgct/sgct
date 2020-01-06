/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__PROJECTION__H__
#define __SGCT__PROJECTION__H__

#include <sgct/frustum.h>
#include <glm/glm.hpp>

namespace sgct {

class ProjectionPlane;

/// This class holds and manages 3D projections
class Projection {
public:
    void calculateProjection(glm::vec3 base, const ProjectionPlane& proj, float nearClip, 
        float farClip, glm::vec3 viewOffset = glm::vec3(0.f));

    const glm::mat4& viewProjectionMatrix() const;
    const glm::mat4& viewMatrix() const;
    const glm::mat4& projectionMatrix() const;

private:
    glm::mat4 _viewMatrix = glm::mat4(1.f);
    glm::mat4 _viewProjectionMatrix = glm::mat4(1.f);
    glm::mat4 _projectionMatrix = glm::mat4(1.f);

    Frustum _frustum;
};

} // namespace sgct

#endif // __SGCT__PROJECTION__H__
