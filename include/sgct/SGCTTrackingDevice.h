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
#include <glm/gtx/quaternion.hpp>

typedef void * GLFWmutex;

namespace sgct
{

class SGCTTrackingDevice
{
public:
	enum DataLoc { CURRENT = 0, PREVIOUS };

	SGCTTrackingDevice(size_t parentIndex, std::string name);
	~SGCTTrackingDevice();

	void setEnabled(bool state);
	void setSensorId(int id);
	void setNumberOfButtons(size_t numOfButtons);
	void setNumberOfAxes(size_t numOfAxes);
	void setSensorTransform( glm::dmat4 mat );
	void setButtonVal(const bool val, size_t index);
	void setAnalogVal(const double * array, size_t size);
	void setOrientation(double xRot, double yRot, double zRot);
	void setOffset(double x, double y, double z);

	inline const std::string & getName() { return mName; }
	inline size_t getNumberOfButtons() { return mNumberOfButtons; }
	inline size_t getNumberOfAxes() { return mNumberOfAxes; }
	bool getButton(size_t index, DataLoc i = CURRENT);
	double getAnalog(size_t index, DataLoc i = CURRENT);
	bool isEnabled();
	inline bool hasSensor() { return mSensorId != -1; }
	inline bool hasButtons() { return mNumberOfButtons > 0; }
	inline bool hasAnalogs() { return mNumberOfAxes > 0; }
	int getSensorId();

	glm::dvec3 getPosition(DataLoc i = CURRENT);
	glm::dvec3 getEulerAngles(DataLoc i = CURRENT);
	glm::dmat4 getTransformMat(DataLoc i = CURRENT);

	double getTrackerTime();
	double getAnalogTime();

private:
	void calculateTransform();
	void setTrackerTime();
	void setAnalogTime();

private:
	bool mEnabled;
	std::string mName;
	size_t mParentIndex; //the index of parent SGCTTracker
	size_t mNumberOfButtons;
	size_t mNumberOfAxes;
	int mSensorId;

	glm::dmat4 mPostTransform;
	glm::dmat4 mWorldTransform[2];
	glm::dmat4 mOrientation;
	glm::dvec3 mOffset;

	double mTrackerTime[2];
	double mAnalogTime[2];
	bool * mButtons;
	double * mAxes;
};

}

#endif
