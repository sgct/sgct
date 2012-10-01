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

#ifndef _SGCT_TRACKING_DEVICE_H_
#define _SGCT_TRACKING_DEVICE_H_

#include <string>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace core_sgct
{

class SGCTTrackingDevice
{
public:
	SGCTTrackingDevice(size_t index, const char * name);
	~SGCTTrackingDevice();
	
	void setEnabled(bool state);
	void setNumberOfButtons(size_t numOfButtons);
	void setNumberOfAxes(size_t numOfAxes);
	void setPosition(const double &x, const double &y, const double &z);
	void setRotation(const double &w, const double &x, const double &y, const double &z);
	void setButtonVal(const bool val, size_t index);
	void setAnalogVal(const double &val, size_t index);

	inline size_t getNumberOfButtons() { return mNumberOfButtons; }
	inline size_t getNumberOfAxes() { return mNumberOfAxes; }
	inline bool * getButtonVals() { return mButtons; }
	inline double * getAnalogVals() { return mAxes; }
	inline bool isEnabled() { return mEnabled; }
	inline bool hasButtons() { return mNumberOfButtons > 0; }
	inline bool hasAnalogs() { return mNumberOfAxes > 0; }
	inline glm::dvec4 getPosition() { return mTrackedPos; }
	inline glm::dquat getRotation() { return mTrackedRot; }

private:
	bool mEnabled;
	std::string mName;
	size_t mIndex;
	size_t mNumberOfButtons;
	size_t mNumberOfAxes;

	glm::dvec4 mTrackedPos;
	glm::dquat mTrackedRot;

	bool * mButtons;
	double * mAxes;
};

}

#endif