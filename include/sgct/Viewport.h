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

#ifndef _VIEWPORT_H
#define _VIEWPORT_H

#include <glm/glm.hpp>
#include "Frustum.h"
#include <stddef.h> //get definition for NULL

namespace core_sgct
{

class Viewport
{
public:
	Viewport();
	Viewport(float x, float y, float xSize, float ySize);

	void set(float x, float y, float xSize, float ySize);
	void setPos(float x, float y);
	void setSize(float x, float y);
	void setEye(Frustum::FrustumMode eye);
	void setOverlayTexture(const char * texturePath);
	void setTracked(bool state);
	void loadOverlayTexture();
	void calculateFrustum(const core_sgct::Frustum::FrustumMode &frustumMode, glm::vec3 * eyePos, float near, float far);
	void setViewPlaneCoords(const unsigned int cornerIndex, glm::vec3 cornerPos);

	inline float getX() { return mX; }
	inline float getY() { return mY; }
	inline float getXSize() { return mXSize; }
	inline float getYSize() { return mYSize; }
	inline Frustum::FrustumMode getEye() { return mEye; }
	inline Frustum * getFrustum(core_sgct::Frustum::FrustumMode frustumMode) { return &mFrustums[frustumMode]; }
	inline Frustum * getFrustum() { return &mFrustums[mEye]; }
	inline const glm::mat4 & getProjectionMatrix( core_sgct::Frustum::FrustumMode frustumMode ) { return mProjectionMatrix[frustumMode]; }
	inline const glm::mat4 & getFrustumMatrix( core_sgct::Frustum::FrustumMode frustumMode ) { return mFrustumMat[frustumMode]; }
	inline bool hasOverlayTexture() { return mOverlayTexture; }
	inline bool isTracked() { return mTracked; }
	inline unsigned int getOverlayTextureIndex() { return mTextureIndex; }

	enum corners { LowerLeft = 0, UpperLeft, UpperRight };

private:
	glm::vec3 mViewPlaneCoords[3];
	glm::mat4 mViewMatrix[3];
	glm::mat4 mProjectionMatrix[3];
	glm::mat4 mFrustumMat[3];

	float mX;
	float mY;
	float mXSize;
	float mYSize;
	Frustum mFrustums[3];
	Frustum::FrustumMode mEye;
	char * mFilename;
	bool mOverlayTexture;
	bool mTracked;
	unsigned int mTextureIndex;
};

}

#endif