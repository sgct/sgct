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

#ifndef _SGCT_USER_H_
#define _SGCT_USER_H_

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include "Frustum.h"

namespace core_sgct
{

/*!
Helper class for setting user variables
*/
class SGCTUser
{
public:
	SGCTUser();

	void setPos(float x, float y, float z);
	void setPos(glm::vec3 pos);
	void setPos(glm::dvec4 pos);
	void setPos(double * pos);
	void setHeadTracker(const char * trackerName, const char * deviceName);

	void setTransform(const glm::mat4 & transform);
	void setTransform(const glm::dmat4 & transform);
	void setOrientation(float xRot, float yRot, float zRot);
	void setEyeSeparation(float eyeSeparation);

	const glm::vec3 & getPos(Frustum::FrustumMode fm = Frustum::Mono);
	glm::vec3 * getPosPtr() { return &mPos[Frustum::Mono]; }
	glm::vec3 * getPosPtr(Frustum::FrustumMode fm) { return &mPos[fm]; }

	inline const float & getEyeSeparation() { return mEyeSeparation; }
	inline const float & getXPos() { return mPos[Frustum::Mono].x; }
	inline const float & getYPos() { return mPos[Frustum::Mono].y; }
	inline const float & getZPos() { return mPos[Frustum::Mono].z; }
	inline const char * getHeadTrackerName() { return mHeadTrackerName.c_str(); }
	inline const char * getHeadTrackerDeviceName() { return mHeadTrackerDeviceName.c_str(); }

private:
	void updateEyeSeparation();
	void updateEyeTransform();

private:
	glm::vec3 mPos[3];
	glm::mat4 mTransform;

	float mEyeSeparation;

	std::string mHeadTrackerDeviceName;
	std::string mHeadTrackerName;

};

} // core_sgct

#endif