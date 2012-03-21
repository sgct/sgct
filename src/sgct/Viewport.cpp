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

core_sgct::Viewport::Viewport()
{
	mX = 0.0f;
	mY = 0.0f;
	mXSize = 1.0f;
	mYSize = 1.0f;
	mEye = Frustum::Mono;
}

core_sgct::Viewport::Viewport(float x, float y, float xSize, float ySize)
{
	mX = x;
	mY = y;
	mXSize = xSize;
	mYSize = ySize;
	mEye = Frustum::Mono;
}

void core_sgct::Viewport::set(float x, float y, float xSize, float ySize)
{
	mX = x;
	mY = y;
	mXSize = xSize;
	mYSize = ySize;
	mEye = Frustum::Mono;
}

void core_sgct::Viewport::setPos(float x, float y)
{
	mX = x;
	mY = y;
}

void core_sgct::Viewport::setSize(float x, float y)
{
	mXSize = x;
	mYSize = y;
}

void core_sgct::Viewport::setEye(core_sgct::Frustum::FrustumMode eye)
{
	mEye = eye;
}

void core_sgct::Viewport::calculateFrustum(int frustumMode, glm::vec3 * userPos, float near, float far)
{
	//nearFactor = near clipping plane / focus plane dist
	float nearFactor = near / (viewPlaneCoords[ LowerLeft ].z - userPos->z);
	if( nearFactor < 0 )
		nearFactor = -nearFactor;

	mFrustums[frustumMode].set(
		(viewPlaneCoords[ LowerLeft ].x - userPos->x)*nearFactor,
		(viewPlaneCoords[ UpperRight ].x - userPos->x)*nearFactor,
		(viewPlaneCoords[ LowerLeft ].y - userPos->y)*nearFactor,
		(viewPlaneCoords[ UpperRight ].y - userPos->y)*nearFactor,
		near,
		far);
}