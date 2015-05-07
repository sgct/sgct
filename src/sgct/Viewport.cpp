/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/Viewport.h"
#include "../include/sgct/TextureManager.h"
#include "../include/sgct/ClusterManager.h"
#include "../include/sgct/MessageHandler.h"
#include <glm/gtc/matrix_transform.hpp>

sgct_core::Viewport::Viewport()
{
	reset(0.0f, 0.0f, 1.0f, 1.0f);

	for(int i=0; i<3; i++)
	{
		mViewMatrix[i]				= glm::mat4(1.0f);
		mProjectionMatrix[i]		= glm::mat4(1.0f);
		mViewProjectionMatrix[i]	= glm::mat4(1.0f);
	}
}

/*!
	Create a viewport coordinates are relative to the window size [0, 1]
*/
sgct_core::Viewport::Viewport(float x, float y, float xSize, float ySize)
{
	reset(x, y, xSize, ySize);

	for(int i=0; i<3; i++)
	{
		mViewMatrix[i]				= glm::mat4(1.0f);
		mProjectionMatrix[i]		= glm::mat4(1.0f);
		mViewProjectionMatrix[i]	= glm::mat4(1.0f);
	}
}

/*!
Destructor that deletes any overlay or mask textures
*/
sgct_core::Viewport::~Viewport()
{
	if (mOverlayTextureIndex)
		glDeleteTextures(1, &mOverlayTextureIndex);

	if (mMaskTextureIndex)
		glDeleteTextures(1, &mMaskTextureIndex);
}

void sgct_core::Viewport::reset(float x, float y, float xSize, float ySize)
{
	mX = x;
	mY = y;
	mXSize = xSize;
	mYSize = ySize;
	mEye = Frustum::Mono;
	mCorrectionMesh = false;
	mOverlayTextureIndex = GL_FALSE;
	mMaskTextureIndex = GL_FALSE;
	mTracked = false;
	mEnabled = true;
    mGenerateGPUData = true;
    mName.assign("NoName");
	mUser = ClusterManager::instance()->getDefaultUserPtr();

	mViewPlaneCoords[LowerLeft] = glm::vec3(-1.0f, -1.0f, -2.0f);
	mViewPlaneCoords[UpperLeft] = glm::vec3(-1.0f, 1.0f, -2.0f);
	mViewPlaneCoords[UpperRight] = glm::vec3(1.0f, 1.0f, -2.0f);
}

void sgct_core::Viewport::setEye(sgct_core::Frustum::FrustumMode eye)
{
	mEye = eye;
}

void sgct_core::Viewport::setOverlayTexture(const char * texturePath)
{
	mOverlayFilename.assign(texturePath);
}

void sgct_core::Viewport::setMaskTexture(const char * texturePath)
{
	mMaskFilename.assign(texturePath);
}

void sgct_core::Viewport::setCorrectionMesh(const char * meshPath)
{
	mMeshFilename.assign(meshPath);
}

void sgct_core::Viewport::setTracked(bool state)
{
	mTracked = state;
}

/*!
Set if this viewport should disable all VBO, VAO and texture usage.
*/
void sgct_core::Viewport::setAsDummy()
{
    mGenerateGPUData = false;
}

void sgct_core::Viewport::loadData()
{
	if(mGenerateGPUData)
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Viewport: loading GPU data for '%s'\n", mName.c_str());
        
        if( mOverlayFilename.size() > 0 )
            sgct::TextureManager::instance()->loadUnManagedTexture(mOverlayTextureIndex, mOverlayFilename, true, 1);

        if ( mMaskFilename.size() > 0 )
            sgct::TextureManager::instance()->loadUnManagedTexture(mMaskTextureIndex, mMaskFilename, true, 1);

        //load default if mMeshFilename is NULL
		mCM.setViewportCoords(mXSize, mYSize, mX, mY);
        mCorrectionMesh = mCM.readAndGenerateMesh(mMeshFilename.c_str(), this);
    }
}

void sgct_core::Viewport::calculateFrustum(const sgct_core::Frustum::FrustumMode &frustumMode, float near_clipping_plane, float far_clipping_plane)
{
	glm::vec3 eyePos = mUser->getPos(frustumMode);
	
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
	glm::vec3 transformedEyePos = DCM_inv * eyePos;

	//nearFactor = near clipping plane / focus plane dist
	float nearFactor = near_clipping_plane / (transformedViewPlaneCoords[LowerLeft].z - transformedEyePos.z);
	if( nearFactor < 0 )
		nearFactor = -nearFactor;

	mFrustums[frustumMode].set(
		(transformedViewPlaneCoords[ LowerLeft ].x - transformedEyePos.x)*nearFactor,
		(transformedViewPlaneCoords[ UpperRight ].x - transformedEyePos.x)*nearFactor,
		(transformedViewPlaneCoords[ LowerLeft ].y - transformedEyePos.y)*nearFactor,
		(transformedViewPlaneCoords[ UpperRight ].y - transformedEyePos.y)*nearFactor,
		near_clipping_plane,
		far_clipping_plane);

	mViewMatrix[frustumMode] = glm::mat4(DCM_inv) * glm::translate(glm::mat4(1.0f), -eyePos);

	//calc frustum matrix
	mProjectionMatrix[frustumMode] = glm::frustum( 
		mFrustums[frustumMode].getLeft(),
		mFrustums[frustumMode].getRight(),
        mFrustums[frustumMode].getBottom(),
		mFrustums[frustumMode].getTop(),
        mFrustums[frustumMode].getNear(),
		mFrustums[frustumMode].getFar() );

	mViewProjectionMatrix[frustumMode] = mProjectionMatrix[frustumMode] * mViewMatrix[frustumMode];
}

void sgct_core::Viewport::calculateFisheyeFrustum(const sgct_core::Frustum::FrustumMode &frustumMode, float near_clipping_plane, float far_clipping_plane)
{
	glm::vec3 eyePos = mUser->getPos();
	glm::vec3 offset = mUser->getPos(frustumMode);
	
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
	glm::vec3 transformedEyePos = DCM_inv * eyePos;

	//nearFactor = near clipping plane / focus plane dist
	float nearFactor = near_clipping_plane / (transformedViewPlaneCoords[LowerLeft].z - transformedEyePos.z);
	if( nearFactor < 0 )
		nearFactor = -nearFactor;

	mFrustums[frustumMode].set(
		(transformedViewPlaneCoords[ LowerLeft ].x - transformedEyePos.x)*nearFactor,
		(transformedViewPlaneCoords[ UpperRight ].x - transformedEyePos.x)*nearFactor,
		(transformedViewPlaneCoords[ LowerLeft ].y - transformedEyePos.y)*nearFactor,
		(transformedViewPlaneCoords[ UpperRight ].y - transformedEyePos.y)*nearFactor,
		near_clipping_plane,
		far_clipping_plane);

	mViewMatrix[frustumMode] = glm::mat4(DCM_inv) * glm::translate(glm::mat4(1.0f), -offset);

	//calc frustum matrix
	mProjectionMatrix[frustumMode] = glm::frustum( 
		mFrustums[frustumMode].getLeft(),
		mFrustums[frustumMode].getRight(),
        mFrustums[frustumMode].getBottom(),
		mFrustums[frustumMode].getTop(),
        mFrustums[frustumMode].getNear(),
		mFrustums[frustumMode].getFar() );

	mViewProjectionMatrix[frustumMode] = mProjectionMatrix[frustumMode] * mViewMatrix[frustumMode];
}

void sgct_core::Viewport::setViewPlaneCoords(const unsigned int cornerIndex, glm::vec3 cornerPos)
{
	mViewPlaneCoords[cornerIndex] = cornerPos;
}

void sgct_core::Viewport::setViewPlaneCoords(const unsigned int cornerIndex, glm::vec4 cornerPos)
{
	mViewPlaneCoords[cornerIndex] = glm::vec3(cornerPos);
}

void sgct_core::Viewport::setViewPlaneCoordsUsingFOVs(float up, float down, float left, float right, glm::quat rot, float dist)
{
	glm::vec3 unTransformedViewPlaneCoords[3];
	
	unTransformedViewPlaneCoords[LowerLeft].x = dist * tanf(glm::radians<float>(left));
	unTransformedViewPlaneCoords[LowerLeft].y = dist * tanf(glm::radians<float>(down));
	unTransformedViewPlaneCoords[LowerLeft].z = -dist;

	unTransformedViewPlaneCoords[UpperLeft].x = dist * tanf(glm::radians<float>(left));
	unTransformedViewPlaneCoords[UpperLeft].y = dist * tanf(glm::radians<float>(up));
	unTransformedViewPlaneCoords[UpperLeft].z = -dist;

	unTransformedViewPlaneCoords[UpperRight].x = dist * tanf(glm::radians<float>(right));
	unTransformedViewPlaneCoords[UpperRight].y = dist * tanf(glm::radians<float>(up));
	unTransformedViewPlaneCoords[UpperRight].z = -dist;

	for (unsigned int i = 0; i < 3; i++)
		mViewPlaneCoords[i] = (rot * unTransformedViewPlaneCoords[i]) - mUser->getPos();

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG,
		"Viewplane lower left coords: %f %f %f\n",
		mViewPlaneCoords[LowerLeft].x,
		mViewPlaneCoords[LowerLeft].y,
		mViewPlaneCoords[LowerLeft].z);

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG,
		"Viewplane upper left coords: %f %f %f\n",
		mViewPlaneCoords[UpperLeft].x,
		mViewPlaneCoords[UpperLeft].y,
		mViewPlaneCoords[UpperLeft].z);

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG,
		"Viewplane upper right coords: %f %f %f\n\n",
		mViewPlaneCoords[UpperRight].x,
		mViewPlaneCoords[UpperRight].y,
		mViewPlaneCoords[UpperRight].z);
}

/*!
Render the viewport mesh which the framebuffer texture is attached to
\param type of mesh; quad, warped or mask
*/
void sgct_core::Viewport::renderMesh(sgct_core::CorrectionMesh::MeshType mt)
{
	if( mEnabled )
		mCM.render(mt);
}
