/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__PROJECTION__H__
#define __SGCT__PROJECTION__H__

//#include <string>
//#include "SGCTUser.h"
#include <glm/glm.hpp>
#include "Frustum.h"
#include "SGCTProjectionPlane.h"

namespace sgct_core {

/*!
    This class holds and manages 3D projections
*/
class SGCTProjection {
public:
    void calculateProjection(glm::vec3 base, SGCTProjectionPlane* projectionPlanePtr, float near_clipping_plane, float far_clipping_plane, glm::vec3 viewOffset = glm::vec3(0.0f, 0.0f, 0.0f));

    Frustum* getFrustum();
    const glm::mat4& getViewProjectionMatrix();
    const glm::mat4& getViewMatrix();
    const glm::mat4& getProjectionMatrix();
    
protected:
    glm::mat4 mViewMatrix = glm::mat4(1.f);
    glm::mat4 mViewProjectionMatrix = glm::mat4(1.f);
    glm::mat4 mProjectionMatrix = glm::mat4(1.f);

    Frustum mFrustum;
};

} // namespace sgct_core

#endif // __SGCT__PROJECTION__H__
