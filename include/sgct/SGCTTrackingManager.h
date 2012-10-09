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

#include <vector>
#include <set>
#include "SGCTTracker.h"

namespace sgct
{

class SGCTTrackingManager
{
public:
	SGCTTrackingManager();
	~SGCTTrackingManager();
	
	void startSampling();
	void updateTrackingDevices();
	void addTracker(std::string name);
	void addDeviceToCurrentTracker(std::string name);
	void addSensorToCurrentDevice(const char * address, int id);
	void addButtonsToCurrentDevice(const char * address, size_t numOfButtons);
	void addAnalogsToCurrentDevice(const char * address, size_t numOfAxes);
	
	inline size_t getNumberOfTrackers() { return mTrackers.size(); }
	inline size_t getNumberOfDevices() { return mNumberOfDevices; }
	inline SGCTTrackingDevice * getHeadDevicePtr() { return mHead; }

	SGCTTracker * getLastTrackerPtr();
	SGCTTracker * getTrackerPtr(size_t index);
	SGCTTracker * getTrackerPtr(const char * name);

	void setEnabled(bool state);
	void setSamplingTime(double t);
	double getSamplingTime();

	bool isRunning();

private:
	void setHeadTracker(const char * trackerName, const char * deviceName);

private:
	int mSamplingThreadId;
	std::vector<SGCTTracker *> mTrackers;
	std::set< std::string > mAddresses;
	double mSamplingTime;
	bool mRunning;

	SGCTTrackingDevice * mHead;
	size_t mNumberOfDevices;
};

}

#endif