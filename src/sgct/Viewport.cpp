/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/Viewport.h"
#include "../include/sgct/TextureManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <string.h>

sgct_core::Viewport::Viewport()
{
	set(0.0, 0.0, 1.0, 1.0);

	for(int i=0; i<3; i++)
	{
		mViewMatrix[i]			= glm::mat4(1.0f);
		mProjectionMatrix[i]	= glm::mat4(1.0f);
		mFrustumMat[i]			= glm::mat4(1.0f);
	}
}

sgct_core::Viewport::Viewport(double x, double y, double xSize, double ySize)
{
	set(x, y, xSize, ySize);

	for(int i=0; i<3; i++)
	{
		mViewMatrix[i]			= glm::mat4(1.0f);
		mProjectionMatrix[i]	= glm::mat4(1.0f);
		mFrustumMat[i]			= glm::mat4(1.0f);
	}
}

void sgct_core::Viewport::set(double x, double y, double xSize, double ySize)
{
	mX = x;
	mY = y;
	mXSize = xSize;
	mYSize = ySize;
	mEye = Frustum::Mono;
	mOverlayTexture = false;
	mCorrectionMesh = false;
	mOverlayFilename = NULL;
	mMeshFilename = NULL;
	mTextureIndex = 0;
	mTracked = false;
	mEnabled = true;

	mCM.setViewportCoords(static_cast<float>(mXSize), 
		static_cast<float>(mYSize),
		static_cast<float>(mX),
		static_cast<float>(mY));
}

void sgct_core::Viewport::setPos(double x, double y)
{
	mX = x;
	mY = y;
	mCM.setViewportCoords(static_cast<float>(mXSize), 
		static_cast<float>(mYSize),
		static_cast<float>(mX),
		static_cast<float>(mY));
}

void sgct_core::Viewport::setSize(double x, double y)
{
	mXSize = x;
	mYSize = y;
	mCM.setViewportCoords(static_cast<float>(mXSize), 
		static_cast<float>(mYSize),
		static_cast<float>(mX),
		static_cast<float>(mY));
}

void sgct_core::Viewport::setEye(sgct_core::Frustum::FrustumMode eye)
{
	mEye = eye;
}

void sgct_core::Viewport::setOverlayTexture(const char * texturePath)
{
	//copy filename
	if( strlen(texturePath) > 4 )
	{
		mOverlayFilename = new char[strlen(texturePath)+1];
		#if (_MSC_VER >= 1400) //visual studio 2005 or later
		if( strcpy_s(mOverlayFilename, strlen(texturePath)+1, texturePath ) != 0)
			return;
		#else
		strcpy(mOverlayFilename, texturePath );
		#endif
	}
}

void sgct_core::Viewport::setCorrectionMesh(const char * meshPath)
{
	//copy filename
	if( strlen(meshPath) > 3 )
	{
		mMeshFilename = new char[strlen(meshPath)+1];
		#if (_MSC_VER >= 1400) //visual studio 2005 or later
		if( strcpy_s(mMeshFilename, strlen(meshPath)+1, meshPath ) != 0)
			return;
		#else
		strcpy(mMeshFilename, meshPath );
		#endif
	}
}

void sgct_core::Viewport::setTracked(bool state)
{
	mTracked = state;
}

void sgct_core::Viewport::setEnabled(bool state)
{
	mEnabled = state;
}

void sgct_core::Viewport::loadData()
{
	if( mOverlayFilename != NULL )
		mOverlayTexture = sgct::TextureManager::Instance()->loadTexure(mTextureIndex, "ViewportOverlayTexture", mOverlayFilename, true, 1);

	if( mMeshFilename != NULL )
		mCorrectionMesh = mCM.readAndGenerateMesh(mMeshFilename);
}

void sgct_core::Viewport::calculateFrustum(const sgct_core::Frustum::FrustumMode &frustumMode, glm::vec3 * eyePos, float near, float far)
{
	//calculate viewplane's internal coordinate system bases
	glm::vec3 plane_x = mViewPlaneCoords[ UpperRight ] - mViewPlaneCoords[ UpperLeft ];
	glm::vec3 plane_y = mViewPlaneCoords[ UpperLeft ] - mViewPlaneCoords[ LowerLeft ];
	glm::vec3 plane_z = glm::cross(plane_x, plane_y);
	
	//normalize
	plane_x = glm::normalize(plane_x);
	plane_y = glm::normalize(plane_y);
	plane_z = glm::normalize(plane_z);

	glm::vec3 world_x(1.0f, 0.0f, 0.0f);
	glm::vec3 world_y(0.0f, 1.0f, 0.0f);
	glm::vec3 world_z(0.0f, 0.0f, 1.0f);

	//calculate plane rotation using
	//Direction Cosine Matrix (DCM)
	glm::mat3 DCM(1.0f); //init as identity matrix
	DCM[0][0] = glm::dot( plane_x, world_x );
	DCM[0][1] = glm::dot( plane_x, world_y );
	DCM[0][2] = glm::dot( plane_x, world_z );

	DCM[1][0] = glm::dot( plane_y, world_x );
	DCM[1][1] = glm::dot( plane_y, world_y );
	DCM[1][2] = glm::dot( plane_y, world_z );

	DCM[2][0] = glm::dot( plane_z, world_x );
	DCM[2][1] = glm::dot( plane_z, world_y );
	DCM[2][2] = glm::dot( plane_z, world_z );

	//invert & transform
	glm::mat3 DCM_inv = glm::inverse(DCM);
	glm::vec3 transformedViewPlaneCoords[3];
	transformedViewPlaneCoords[ LowerLeft ] = DCM_inv * mViewPlaneCoords[ LowerLeft ];
	transformedViewPlaneCoords[ UpperLeft ] = DCM_inv * mViewPlaneCoords[ UpperLeft ];
	transformedViewPlaneCoords[ UpperRight ] = DCM_inv * mViewPlaneCoords[ UpperRight ];
	glm::vec3 transformedEyePos = DCM_inv * (*eyePos);

	//nearFactor = near clipping plane / focus plane dist
	float nearFactor = near / (transformedViewPlaneCoords[ LowerLeft ].z - transformedEyePos.z);
	if( nearFactor < 0 )
		nearFactor = -nearFactor;

	mFrustums[frustumMode].set(
		(transformedViewPlaneCoords[ LowerLeft ].x - transformedEyePos.x)*nearFactor,
		(transformedViewPlaneCoords[ UpperRight ].x - transformedEyePos.x)*nearFactor,
		(transformedViewPlaneCoords[ LowerLeft ].y - transformedEyePos.y)*nearFactor,
		(transformedViewPlaneCoords[ UpperRight ].y - transformedEyePos.y)*nearFactor,
		near,
		far);

	mViewMatrix[frustumMode] = glm::mat4(DCM_inv) * glm::translate(glm::mat4(1.0f), -(*eyePos));

	//calc frustum matrix
	mFrustumMat[frustumMode] = glm::frustum( 
		mFrustums[frustumMode].getLeft(),
		mFrustums[frustumMode].getRight(),
        mFrustums[frustumMode].getBottom(),
		mFrustums[frustumMode].getTop(),
        mFrustums[frustumMode].getNear(),
		mFrustums[frustumMode].getFar() );

	mProjectionMatrix[frustumMode] = mFrustumMat[frustumMode] * mViewMatrix[frustumMode];
}

void sgct_core::Viewport::setViewPlaneCoords(const unsigned int cornerIndex, glm::vec3 cornerPos)
{
	mViewPlaneCoords[cornerIndex] = cornerPos;
}

void sgct_core::Viewport::setViewPlaneCoords(const unsigned int cornerIndex, glm::vec4 cornerPos)
{
	mViewPlaneCoords[cornerIndex] = glm::vec3(cornerPos);
}

void sgct_core::Viewport::renderMesh()
{
	mCM.render();
}
