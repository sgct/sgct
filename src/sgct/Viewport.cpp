/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/Viewport.h"
#include "../include/sgct/TextureManager.h"
#include "../include/sgct/ClusterManager.h"
#include "../include/sgct/MessageHandler.h"
//#include <glm/gtc/matrix_transform.hpp>

sgct_core::Viewport::Viewport()
{
	reset(0.0f, 0.0f, 1.0f, 1.0f);
}

/*!
	Create a viewport coordinates are relative to the window size [0, 1]
*/
sgct_core::Viewport::Viewport(float x, float y, float xSize, float ySize)
{
	reset(x, y, xSize, ySize);
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
	mCM.setViewportCoords(mXSize, mYSize, mX, mY);
	mProjectionPlane.reset();
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
        mCorrectionMesh = mCM.readAndGenerateMesh(mMeshFilename.c_str(), this);
    }
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
