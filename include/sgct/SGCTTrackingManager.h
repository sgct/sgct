/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__TRACKING_DEVICE__H__
#define __SGCT__TRACKING_DEVICE__H__

#include <vector>
#include <set>
#include <thread>
#include <sgct/SGCTTracker.h>
#include <sgct/SGCTUser.h>

namespace sgct {

/*!
Class that manages tracking systems
*/
class SGCTTrackingManager {
public:
    ~SGCTTrackingManager();
    
    void startSampling();
    void updateTrackingDevices();
    void addTracker(std::string name);
    void addDeviceToCurrentTracker(std::string name);
    void addSensorToCurrentDevice(const char* address, int id);
    void addButtonsToCurrentDevice(const char* address, size_t numOfButtons);
    void addAnalogsToCurrentDevice(const char* address, size_t numOfAxes);
    
    size_t getNumberOfTrackers();
    size_t getNumberOfDevices();
    SGCTTrackingDevice * getHeadDevicePtr();

    SGCTTracker* getLastTrackerPtr();
    SGCTTracker* getTrackerPtr(size_t index);
    SGCTTracker* getTrackerPtr(const char* name);

    void setEnabled(bool state);
    void setSamplingTime(double t);
    double getSamplingTime();

    bool isRunning();

private:
    void setHeadTracker(const char * trackerName, const char * deviceName);

private:
    std::thread* mSamplingThread = nullptr;
    std::vector<SGCTTracker*> mTrackers;
    std::set<std::string> mAddresses;
    double mSamplingTime = 0.0;
    bool mRunning = true;

    sgct_core::SGCTUser* mHeadUser = nullptr;
    SGCTTrackingDevice* mHead = nullptr;
    size_t mNumberOfDevices = 0;
};

} // namespace sgct

#endif // __SGCT__TRACKING_DEVICE__H__
