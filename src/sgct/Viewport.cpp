/*************************************************************************
Copyright (c) 2012 Miroslav Andel, Linköping University.
All rights reserved.

Original Authors:
Miroslav Andel, Alexander Fridlund

For any questions or information about the SGCT project please contact: miroslav.andel@liu.se

This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to
Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*************************************************************************/

#include "../include/sgct/Viewport.h"
#include "../include/sgct/TextureManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <string.h>

core_sgct::Viewport::Viewport()
{
	set(0.0, 0.0, 1.0, 1.0);

	for(int i=0; i<3; i++)
	{
		mViewMatrix[i]			= glm::mat4(1.0f);
		mProjectionMatrix[i]	= glm::mat4(1.0f);
		mFrustumMat[i]			= glm::mat4(1.0f);
	}
}

core_sgct::Viewport::Viewport(double x, double y, double xSize, double ySize)
{
	set(x, y, xSize, ySize);

	for(int i=0; i<3; i++)
	{
		mViewMatrix[i]			= glm::mat4(1.0f);
		mProjectionMatrix[i]	= glm::mat4(1.0f);
		mFrustumMat[i]			= glm::mat4(1.0f);
	}
}

void core_sgct::Viewport::set(double x, double y, double xSize, double ySize)
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
	mCM.setViewportPointers(mXSize, mYSize, mX, mY);
}

void core_sgct::Viewport::setPos(double x, double y)
{
	mX = x;
	mY = y;
	mCM.setViewportPointers(mXSize, mYSize, mX, mY);
}

void core_sgct::Viewport::setSize(double x, double y)
{
	mXSize = x;
	mYSize = y;
	mCM.setViewportPointers(mXSize, mYSize, mX, mY);
}

void core_sgct::Viewport::setEye(core_sgct::Frustum::FrustumMode eye)
{
	mEye = eye;
}

void core_sgct::Viewport::setOverlayTexture(const char * texturePath)
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

void core_sgct::Viewport::setCorrectionMesh(const char * meshPath)
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

void core_sgct::Viewport::setTracked(bool state)
{
	mTracked = state;
}

void core_sgct::Viewport::loadData()
{
	if( mOverlayFilename != NULL )
		mOverlayTexture = sgct::TextureManager::Instance()->loadTexure(mTextureIndex, "ViewportOverlayTexture", mOverlayFilename, true, 1);

	if( mMeshFilename != NULL )
		mCorrectionMesh = mCM.readAndGenerateMesh(mMeshFilename);
}

void core_sgct::Viewport::calculateFrustum(const core_sgct::Frustum::FrustumMode &frustumMode, glm::vec3 * eyePos, float near, float far)
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

void core_sgct::Viewport::setViewPlaneCoords(const unsigned int cornerIndex, glm::vec3 cornerPos)
{
	mViewPlaneCoords[cornerIndex] = cornerPos;
}

void core_sgct::Viewport::renderMesh()
{
	mCM.render();
}
