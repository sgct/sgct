/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/SGCTTrackingManager.h>

#include <sgct/Engine.h>
#include <sgct/ClusterManager.h>
#include <sgct/MessageHandler.h>
#include <sgct/SGCTMutexManager.h>
#include <sgct/SGCTTracker.h>
#include <sgct/SGCTTrackingDevice.h>
#include <sgct/SGCTUser.h>
#include "../include/vrpn/vrpn_Tracker.h"
#include "../include/vrpn/vrpn_Button.h"
#include "../include/vrpn/vrpn_Analog.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace {
    struct VRPNPointer {
        vrpn_Tracker_Remote* mSensorDevice;
        vrpn_Analog_Remote* mAnalogDevice;
        vrpn_Button_Remote* mButtonDevice;
    };

    struct VRPNTracker {
        std::vector<VRPNPointer> mDevices;
    };

    std::vector<VRPNTracker> gTrackers;

    void VRPN_CALLBACK update_tracker_cb(void* userdata, const vrpn_TRACKERCB t) {
        using namespace sgct;

        if (userdata == nullptr) {
            return;
        }

        SGCTTracker* trackerPtr = reinterpret_cast<SGCTTracker*>(userdata);
        SGCTTrackingDevice* devicePtr = trackerPtr->getDevicePtrBySensorId(t.sensor);

        if (devicePtr == nullptr) {
            return;
        }

        glm::dvec3 posVec = glm::dvec3(t.pos[0], t.pos[1], t.pos[2]);
        posVec *= trackerPtr->getScale();

        glm::dquat rotation(t.quat[3], t.quat[0], t.quat[1], t.quat[2]);
        devicePtr->setSensorTransform(posVec, rotation);
    }

    void VRPN_CALLBACK update_button_cb(void* userdata, const vrpn_BUTTONCB b) {
        using namespace sgct;
        SGCTTrackingDevice* devicePtr = reinterpret_cast<SGCTTrackingDevice*>(userdata);

        //fprintf(stderr, "Button: %d, state: %d\n", b.button, b.state);

        devicePtr->setButtonVal(b.state != 0, b.button);
    }

    void VRPN_CALLBACK update_analog_cb(void* userdata, const vrpn_ANALOGCB a) {
        using namespace sgct;
        SGCTTrackingDevice* tdPtr = reinterpret_cast<SGCTTrackingDevice*>(userdata);
        tdPtr->setAnalogVal(a.channel, static_cast<size_t>(a.num_channel));
    }

    void samplingLoop(void* arg) {
        using namespace sgct;
        SGCTTrackingManager* tmPtr = reinterpret_cast<SGCTTrackingManager*>(arg);

        while (true) {
            double t = sgct::Engine::getTime();
            for (size_t i = 0; i < tmPtr->getNumberOfTrackers(); i++) {
                sgct::SGCTTracker* trackerPtr = tmPtr->getTrackerPtr(i);

                if (trackerPtr != nullptr) {
                    for (size_t j = 0; j < trackerPtr->getNumberOfDevices(); j++) {
                        if (trackerPtr->getDevicePtr(j)->isEnabled()) {
                            if (gTrackers[i].mDevices[j].mSensorDevice != nullptr) {
                                gTrackers[i].mDevices[j].mSensorDevice->mainloop();
                            }

                            if (gTrackers[i].mDevices[j].mAnalogDevice != nullptr) {
                                gTrackers[i].mDevices[j].mAnalogDevice->mainloop();
                            }

                            if (gTrackers[i].mDevices[j].mButtonDevice != nullptr) {
                                gTrackers[i].mDevices[j].mButtonDevice->mainloop();
                            }
                        }
                    }
                }
            }

            bool isRunning = tmPtr->isRunning();

            tmPtr->setSamplingTime(sgct::Engine::getTime() - t);

            // Sleep for 1ms so we don't eat the CPU
            vrpn_SleepMsecs(1);

            if (!isRunning) {
                break;
            }
        }
    }

} // namespace

namespace sgct {

SGCTTrackingManager::~SGCTTrackingManager() {
    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Disconnecting VRPN...\n"
    );

#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Destructing, setting running to false...\n");
#endif
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    mRunning = false;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

    //destroy thread
    if (mSamplingThread != nullptr) {
        mSamplingThread->join();
        delete mSamplingThread;
        mSamplingThread = nullptr;
    }

    //delete all instances
    for (size_t i = 0; i < mTrackers.size(); i++) {
        if (mTrackers[i] != nullptr) {
            delete mTrackers[i];
            mTrackers[i] = nullptr;
        }

        //clear vrpn pointers
        for (size_t j = 0; j < gTrackers[i].mDevices.size(); j++) {
            //clear sensor pointers
            if (gTrackers[i].mDevices[j].mSensorDevice != nullptr) {
                delete gTrackers[i].mDevices[j].mSensorDevice;
                gTrackers[i].mDevices[j].mSensorDevice = nullptr;
            }

            //clear analog pointers
            if (gTrackers[i].mDevices[j].mAnalogDevice != nullptr) {
                delete gTrackers[i].mDevices[j].mAnalogDevice;
                gTrackers[i].mDevices[j].mAnalogDevice = nullptr;
            }

            //clear button pointers
            if (gTrackers[i].mDevices[j].mButtonDevice != nullptr) {
                delete gTrackers[i].mDevices[j].mButtonDevice;
                gTrackers[i].mDevices[j].mButtonDevice = nullptr;
            }
        }
        gTrackers[i].mDevices.clear();
    }

    mTrackers.clear();
    gTrackers.clear();

    MessageHandler::instance()->print(MessageHandler::Level::Debug, "Done.\n");
}


bool SGCTTrackingManager::isRunning() {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Checking if tracking is running...\n");
#endif
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    bool tmpVal = mRunning;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

    return tmpVal;
}

void SGCTTrackingManager::startSampling() {
    if (!mTrackers.empty()) {
        //find user with headtracking
        mHeadUser = sgct_core::ClusterManager::instance()->getTrackedUserPtr();

        //if tracked user not found
        if (mHeadUser == nullptr) {
            mHeadUser = sgct_core::ClusterManager::instance()->getDefaultUserPtr();
        }
        
        //link the head tracker
        setHeadTracker(
            mHeadUser->getHeadTrackerName(),
            mHeadUser->getHeadTrackerDeviceName()
        );

        mSamplingThread = new std::thread(samplingLoop, this);
    }
}

/*
    Update the user position if headtracking is used. This function is called from the engine.
*/
void SGCTTrackingManager::updateTrackingDevices() {
    for (size_t i = 0; i < mTrackers.size(); i++) {
        for (size_t j = 0; j < mTrackers[i]->getNumberOfDevices(); j++) {
            SGCTTrackingDevice* tdPtr = mTrackers[i]->getDevicePtr(j);
            if (tdPtr->isEnabled() && tdPtr == mHead && mHeadUser != nullptr) {
                mHeadUser->setTransform(tdPtr->getWorldTransform());
            }
        }
    }
}

void SGCTTrackingManager::addTracker(std::string name) {
    if (!getTrackerPtr(name.c_str())) {
        mTrackers.push_back(new SGCTTracker(name));

        VRPNTracker tmpVRPNTracker;
        gTrackers.push_back(tmpVRPNTracker);

        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Tracking: Tracker '%s' added succesfully.\n", name.c_str()
        );
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Tracking: Tracker '%s' already exists!\n", name.c_str()
        );
    }
}

void SGCTTrackingManager::addDeviceToCurrentTracker(std::string name) {
    mNumberOfDevices++;

    mTrackers.back()->addDevice(std::move(name), mTrackers.size() - 1);

    VRPNPointer tmpPtr;
    tmpPtr.mSensorDevice = nullptr;
    tmpPtr.mAnalogDevice = nullptr;
    tmpPtr.mButtonDevice = nullptr;

    gTrackers.back().mDevices.push_back(tmpPtr);
}

void SGCTTrackingManager::addSensorToCurrentDevice(const char* address, int id) {
    if (gTrackers.empty() || gTrackers.back().mDevices.empty()) {
        return;
    }

    std::pair<std::set<std::string>::iterator, bool> retVal =
        mAddresses.insert(std::string(address));

    VRPNPointer& ptr = gTrackers.back().mDevices.back();
    SGCTTrackingDevice* devicePtr = mTrackers.back()->getLastDevicePtr();

    if (devicePtr != nullptr) {
        devicePtr->setSensorId(id);

        if (retVal.second && ptr.mSensorDevice == nullptr) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Info,
                "Tracking: Connecting to sensor '%s'...\n", address
            );
            ptr.mSensorDevice = new vrpn_Tracker_Remote(address);

            if (ptr.mSensorDevice != nullptr) {
                ptr.mSensorDevice->register_change_handler(
                    mTrackers.back(),
                    update_tracker_cb
                );
            }
            else {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Error,
                    "Tracking: Failed to connect to sensor '%s' on device %s!\n",
                    address, devicePtr->getName().c_str()
                );
            }
        }
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Tracking: Failed to connect to sensor '%s'!\n",
            address
        );
    }
}

void SGCTTrackingManager::addButtonsToCurrentDevice(const char* address,
                                                    size_t numOfButtons)
{
    if (gTrackers.empty() || gTrackers.back().mDevices.empty()) {
        return;
    }

    VRPNPointer& ptr = gTrackers.back().mDevices.back();
    SGCTTrackingDevice* devicePtr = mTrackers.back()->getLastDevicePtr();

    if (ptr.mButtonDevice == nullptr && devicePtr != nullptr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Tracking: Connecting to buttons '%s' on device %s...\n",
            address, devicePtr->getName().c_str()
        );

        ptr.mButtonDevice = new vrpn_Button_Remote(address);

        if (ptr.mButtonDevice != nullptr) {
            // connected
            ptr.mButtonDevice->register_change_handler(devicePtr, update_button_cb);
            devicePtr->setNumberOfButtons(numOfButtons);
        }
        else {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Tracking: Failed to connect to buttons '%s' on device %s!\n",
                address, devicePtr->getName().c_str()
            );
        }
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Tracking: Failed to connect to buttons '%s'!\n",
            address
        );
    }
}

void SGCTTrackingManager::addAnalogsToCurrentDevice(const char* address, size_t numOfAxes)
{
    if (gTrackers.empty() || gTrackers.back().mDevices.empty()) {
        return;
    }

    VRPNPointer& ptr = gTrackers.back().mDevices.back();
    SGCTTrackingDevice* devicePtr = mTrackers.back()->getLastDevicePtr();

    if (ptr.mAnalogDevice == nullptr && devicePtr != nullptr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Tracking: Connecting to analogs '%s' on device %s...\n",
                address, devicePtr->getName().c_str()
        );

        ptr.mAnalogDevice = new vrpn_Analog_Remote(address);

        if (ptr.mAnalogDevice != nullptr) {
            ptr.mAnalogDevice->register_change_handler(devicePtr, update_analog_cb);
            devicePtr->setNumberOfAxes(numOfAxes);
        }
        else {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Tracking: Failed to connect to analogs '%s' on device %s!\n",
                address, devicePtr->getName().c_str()
            );
        }
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Tracking: Failed to connect to analogs '%s'!\n",
            address
        );
    }
}

size_t SGCTTrackingManager::getNumberOfTrackers() {
    return mTrackers.size();
}

size_t SGCTTrackingManager::getNumberOfDevices() {
    return mNumberOfDevices;
}

SGCTTrackingDevice* SGCTTrackingManager::getHeadDevicePtr() {
    return mHead;
}

void SGCTTrackingManager::setHeadTracker(const std::string& trackerName, const std::string& deviceName)
{
    SGCTTracker* trackerPtr = getTrackerPtr(trackerName);

    if (trackerPtr != NULL) {
        mHead = trackerPtr->getDevicePtr(deviceName);
    }
    //else no head tracker found

    if (mHead == nullptr && !trackerName.empty() && !deviceName.empty()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Tracking: Failed to set head tracker to %s@%s!\n",
            deviceName.c_str(), trackerName.c_str()
        );
    }
}


SGCTTracker* SGCTTrackingManager::getLastTrackerPtr() {
    return mTrackers.size() > 0 ? mTrackers.back() : nullptr;
}

SGCTTracker* SGCTTrackingManager::getTrackerPtr(size_t index) {
    return index < mTrackers.size() ? mTrackers[index] : nullptr;
}

SGCTTracker* SGCTTrackingManager::getTrackerPtr(const std::string& name) {
    for (size_t i = 0; i < mTrackers.size(); i++) {
        if (mTrackers[i]->getName() == name) {
            return mTrackers[i];
        }
    }

    //MessageHandler::instance()->print(MessageHandler::Level::Error, "SGCTTrackingManager: Tracker '%s' not found!\n", name);

    //if not found
    return nullptr;
}

void SGCTTrackingManager::setEnabled(bool state) {
    for (size_t i = 0; i < mTrackers.size(); i++) {
        mTrackers[i]->setEnabled(state);
    }
}

void SGCTTrackingManager::setSamplingTime(double t) {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Set sampling time for vrpn loop...\n");
#endif
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
        mSamplingTime = t;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);
}

double SGCTTrackingManager::getSamplingTime() {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get sampling time for vrpn loop...\n");
#endif
    SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::TrackingMutex);
    double tmpVal = mSamplingTime;
    SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::TrackingMutex);

    return tmpVal;
}

} // namespace sgct
