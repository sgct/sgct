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

#include "../include/sgct/SGCTNode.h"
#include "../include/sgct/SGCTSettings.h"
#include "../include/sgct/ClusterManager.h"
#include <glm/gtc/matrix_transform.hpp>

sgct_core::SGCTNode::SGCTNode()
{
	mCurrentViewportIndex = 0;
	numberOfSamples = 1;
	mFisheyeMode = false;

	//init optional parameters
	swapInterval = 1;
}

void sgct_core::SGCTNode::addViewport(float left, float right, float bottom, float top)
{
	Viewport tmpVP(left, right, bottom, top);
	mViewports.push_back(tmpVP);
}

void sgct_core::SGCTNode::addViewport(sgct_core::Viewport &vp)
{
	mViewports.push_back(vp);
}

/*!
	Clears the vector containing all viewport data.
*/
void sgct_core::SGCTNode::deleteAllViewports()
{
	mCurrentViewportIndex = 0;
	mViewports.clear();
}

/*!
	Returns if fisheye rendering is active.
*/
bool sgct_core::SGCTNode::isUsingFisheyeRendering()
{
	return mFisheyeMode;
}

/*!
	Generates six viewports that renders the inside of a cube. The method used in SGCT is to only use four faces by rotating the cube 45 degrees.
*/
void sgct_core::SGCTNode::generateCubeMapViewports()
{
	//clear the viewports since they will be replaced
	deleteAllViewports();
	glm::vec4 lowerLeft, upperLeft, upperRight;
	
	float radius = sgct_core::SGCTSettings::Instance()->getCubeMapSize()/2.0f;

	//+Z face
	lowerLeft.x = -1.0f * radius;
	lowerLeft.y = -1.0f * radius;
	lowerLeft.z = 1.0f * radius;
	lowerLeft.w = 1.0f;

	upperLeft.x = -1.0f * radius;
	upperLeft.y = 1.0f * radius;
	upperLeft.z = 1.0f * radius;
	upperLeft.w = 1.0f; 

	upperRight.x = 1.0f * radius;
	upperRight.y = 1.0f * radius;
	upperRight.z = 1.0f * radius;
	upperRight.w = 1.0f;

	//tilt
	float tilt = sgct_core::SGCTSettings::Instance()->getFisheyeTilt();
	glm::mat4 tiltMat = glm::rotate(glm::mat4(1.0f), 90.0f-tilt, glm::vec3(1.0f, 0.0f, 0.0f));

	//pan 45 deg
	glm::mat4 panRot = glm::rotate(tiltMat, 45.0f, glm::vec3(0.0f, 1.0f, 0.0f));

	//add viewports
	for(unsigned int i=0; i<6; i++)
	{
		Viewport tmpVP;
		
		glm::mat4 rotMat(1.0f);

		switch(i)
		{
		case 0: //+X face
			rotMat = glm::rotate(panRot, -90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
			break;

		case 1: //-X face
			tmpVP.setEnabled( false );
			rotMat = glm::rotate(panRot, 90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
			break;

		case 2: //+Y face
			rotMat = glm::rotate(panRot, -90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
			break;

		case 3: //-Y face
			rotMat = glm::rotate(panRot, 90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
			break;

		case 4: //+Z face
			rotMat = panRot;
			break;

		case 5: //-Z face
			tmpVP.setEnabled( false );
			rotMat = glm::rotate(panRot, 180.0f, glm::vec3(0.0f, 1.0f, 0.0f));
			break;
		}

		//Compensate for users pos
		glm::vec4 userVec = glm::vec4(
			ClusterManager::Instance()->getUserPtr()->getXPos(),
			ClusterManager::Instance()->getUserPtr()->getYPos(),
			ClusterManager::Instance()->getUserPtr()->getZPos(),
			1.0f );

		//add viewplane vertices
		tmpVP.setViewPlaneCoords(0, rotMat * lowerLeft + userVec);
		tmpVP.setViewPlaneCoords(1, rotMat * upperLeft + userVec);
		tmpVP.setViewPlaneCoords(2, rotMat * upperRight + userVec);

		addViewport( tmpVP );
	}
}

sgct_core::Viewport * sgct_core::SGCTNode::getCurrentViewport()
{
	return &mViewports[mCurrentViewportIndex];
}

sgct_core::Viewport * sgct_core::SGCTNode::getViewport(unsigned int index)
{
	return &mViewports[index];
}

void sgct_core::SGCTNode::getCurrentViewportPixelCoords(int &x, int &y, int &xSize, int &ySize)
{
	x = static_cast<int>(getCurrentViewport()->getX() *
		static_cast<double>(getWindowPtr()->getHResolution()));
	y = static_cast<int>(getCurrentViewport()->getY() *
		static_cast<double>(getWindowPtr()->getVResolution()));
	xSize = static_cast<int>(getCurrentViewport()->getXSize() *
		static_cast<double>(getWindowPtr()->getHResolution()));
	ySize = static_cast<int>(getCurrentViewport()->getYSize() *
		static_cast<double>(getWindowPtr()->getVResolution()));
}

unsigned int sgct_core::SGCTNode::getNumberOfViewports()
{
	return mViewports.size();
}
