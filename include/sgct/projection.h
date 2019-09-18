/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__PROJECTION__H__
#define __SGCT__PROJECTION__H__

#include <sgct/frustum.h>
#include <glm/glm.hpp>

namespace sgct_core {

class SGCTProjectionPlane;

/**
 * This class holds and manages 3D projections
 */
class SGCTProjection {
public:
    void calculateProjection(glm::vec3 base, const SGCTProjectionPlane& projectionPlane,
        float nearClippingPlane, float farClippingPlane,
        glm::vec3 viewOffset = glm::vec3(0.f));

    Frustum& getFrustum();
    const glm::mat4& getViewProjectionMatrix() const;
    const glm::mat4& getViewMatrix() const;
    const glm::mat4& getProjectionMatrix() const;
    
protected:
    glm::mat4 mViewMatrix = glm::mat4(1.f);
    glm::mat4 mViewProjectionMatrix = glm::mat4(1.f);
    glm::mat4 mProjectionMatrix = glm::mat4(1.f);

    Frustum mFrustum;
};

} // namespace sgct_core

#endif // __SGCT__PROJECTION__H__
