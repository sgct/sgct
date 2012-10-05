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

#ifndef _SGCT_TRACKING_MANAGER_H_
#define _SGCT_TRACKING_MANAGER_H_

/*#include "../include/vrpn/vrpn_Tracker.h"
#include "../include/vrpn/vrpn_Button.h"
#include "../include/vrpn/vrpn_Analog.h"*/

#include <vector>
#include <set>
#include "SGCTTrackingDevice.h"

namespace core_sgct
{

class SGCTTrackingManager
{
public:
	SGCTTrackingManager();
	~SGCTTrackingManager();
	
	void startSampling();
	void updateTrackingDevices();
	void setEnabled(bool state);
	void addDevice(const char * name);
	void addTrackerToDevice(const char * address, int sensor);
	void addButtonsToDevice(const char * address, size_t numOfButtons);
	void addAnalogsToDevice(const char * address, size_t numOfAxes);	

	void setHeadSensorIndex(int index);
	void setOrientation(double xRot, double yRot, double zRot);
	void setOffset(double x, double y, double z);
	
	inline size_t getNumberOfDevices() { return mTrackingDevices.size(); }
	inline int getHeadSensorIndex() { return mHeadSensorIndex; }
	inline glm::dmat4 getTransform() { return mXform; }
	inline const glm::dvec4 & getQuatTransform() { return mQuatTransform; }
	
	SGCTTrackingDevice * getTrackingPtr(size_t index);
	SGCTTrackingDevice * getTrackingPtr(const char * name);
	SGCTTrackingDevice * getTrackingPtrBySensor(int sensor);

	void setSamplingTime(double t);
	double getSamplingTime();

	bool isRunning();

private:
	void calculateTransform();

private:
	int mSamplingThreadId;
	std::vector<SGCTTrackingDevice *> mTrackingDevices;
	glm::dmat4 mXform;
	glm::dmat4 mOrientation;
	glm::dvec3 mOffset;
	glm::dvec4 mQuatTransform;
	std::set< std::string > mAddresses;
	double mSamplingTime;
	bool mRunning;

	int	mHeadSensorIndex;
};

}

#endif