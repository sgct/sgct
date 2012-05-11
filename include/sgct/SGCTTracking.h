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

#ifndef _SGCT_TRACKING_H_
#define _SGCT_TRACKING_H_

#include <glm/gtc/matrix_transform.hpp>

namespace core_sgct
{

class SGCTTracking
{
public:
	SGCTTracking();
	~SGCTTracking();
	void connect(const char * name);
	void update();
	void setHeadSensorIndex(int index);
	void setEnabled(bool state);
	void setOrientation(double xRot, double yRot, double zRot);
	void setOffset(double x, double y, double z);
	inline int getHeadSensorIndex() { return mHeadSensorIndex; }
	inline glm::dmat4 getXform() { return mXform; }
	inline glm::dmat4 getOrientation() { return mOrientation; }
	inline bool isEnabled() { return mEnabled; }

private:
	void calculateXform();

private:
	//Miro ToDo: Add a vector of sensors 
	int mHeadSensorIndex;
	glm::dmat4 mXform;
	glm::dmat4 mOrientation;
	glm::dvec3 mOffset;
	double mXrot;
	double mYrot;
	double mZrot;
	bool mEnabled;
};

}

#endif