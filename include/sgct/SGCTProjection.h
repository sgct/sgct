/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _PROJECTION_H
#define _PROJECTION_H

//#include <string>
//#include "SGCTUser.h"
#include <glm/glm.hpp>
#include "Frustum.h"
#include "SGCTProjectionPlane.h"

namespace sgct_core
{

/*!
	This class holds and manages 3D projections
*/
class SGCTProjection
{
public:
	SGCTProjection();
	void calculateProjection(glm::vec3 base, SGCTProjectionPlane * projectionPlanePtr, float near_clipping_plane, float far_clipping_plane, glm::vec3 viewOffset = glm::vec3(0.0f, 0.0f, 0.0f));

	inline Frustum * getFrustum() { return &mFrustum; }
	inline const glm::mat4 & getViewProjectionMatrix() { return mViewProjectionMatrix; }
	inline const glm::mat4 & getViewMatrix() { return mViewMatrix; }
	inline const glm::mat4 & getProjectionMatrix() { return mProjectionMatrix; }
    
protected:
	glm::mat4 mViewMatrix;
	glm::mat4 mViewProjectionMatrix;
	glm::mat4 mProjectionMatrix;

	Frustum mFrustum;
};

}

#endif
