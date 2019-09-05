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
#include <algorithm>

namespace {
    struct VRPNPointer {
        std::unique_ptr<vrpn_Tracker_Remote> mSensorDevice;
        std::unique_ptr<vrpn_Analog_Remote> mAnalogDevice;
        std::unique_ptr<vrpn_Button_Remote> mButtonDevice;
    };
    std::vector<std::vector<VRPNPointer>> gTrackers;

    void VRPN_CALLBACK updateTracker(void* userdata, const vrpn_TRACKERCB t) {
        using namespace sgct;

        if (userdata == nullptr) {
            return;
        }

        SGCTTracker* trackerPtr = reinterpret_cast<SGCTTracker*>(userdata);
        SGCTTrackingDevice* devicePtr = trackerPtr->getDeviceBySensorId(t.sensor);

        if (devicePtr == nullptr) {
            return;
        }

        glm::dvec3 posVec(t.pos[0], t.pos[1], t.pos[2]);
        posVec *= trackerPtr->getScale();

        glm::dquat rotation(t.quat[3], t.quat[0], t.quat[1], t.quat[2]);
        devicePtr->setSensorTransform(posVec, rotation);
    }

    void VRPN_CALLBACK updateButton(void* userdata, const vrpn_BUTTONCB b) {
        using namespace sgct;
        SGCTTrackingDevice* devicePtr = reinterpret_cast<SGCTTrackingDevice*>(userdata);

        devicePtr->setButtonVal(b.state != 0, b.button);
    }

    void VRPN_CALLBACK updateAnalog(void* userdata, const vrpn_ANALOGCB a) {
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
                sgct::SGCTTracker* trackerPtr = tmPtr->getTracker(i);

                if (trackerPtr == nullptr) {
                    continue;
                }
                for (size_t j = 0; j < trackerPtr->getNumberOfDevices(); j++) {
                    if (!trackerPtr->getDevice(j)->isEnabled()) {
                        continue;
                    }

                    const VRPNPointer& ptr = gTrackers[i][j];

                    if (ptr.mSensorDevice) {
                        ptr.mSensorDevice->mainloop();
                    }
                    if (ptr.mAnalogDevice) {
                        ptr.mAnalogDevice->mainloop();
                    }
                    if (ptr.mButtonDevice) {
                        ptr.mButtonDevice->mainloop();
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
        MessageHandler::Level::Info, "Disconnecting VRPN\n"
    );

#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Destructing, setting running to false\n");
#endif
    {
        std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
        mRunning = false;
    }

    // destroy thread
    if (mSamplingThread) {
        mSamplingThread->join();
        mSamplingThread = nullptr;
    }

    mTrackers.clear();
    gTrackers.clear();

    MessageHandler::instance()->print(MessageHandler::Level::Debug, "Done.\n");
}


bool SGCTTrackingManager::isRunning() const {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Checking if tracking is running...\n");
#endif
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return mRunning;
}

void SGCTTrackingManager::startSampling() {
    if (mTrackers.empty()) {
        return;
    }
    // find user with headtracking
    mHeadUser = sgct_core::ClusterManager::instance()->getTrackedUser();

    // if tracked user not found
    if (mHeadUser == nullptr) {
        mHeadUser = &sgct_core::ClusterManager::instance()->getDefaultUser();
    }
        
    // link the head tracker
    const std::string& trackerName = mHeadUser->getHeadTrackerName();
    SGCTTracker* trackerPtr = getTracker(trackerName);

    const std::string& deviceName = mHeadUser->getHeadTrackerDeviceName();
    if (trackerPtr) {
        mHead = trackerPtr->getDevice(deviceName);
    }

    if (mHead == nullptr && !trackerName.empty() && !deviceName.empty()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Tracking: Failed to set head tracker to %s@%s\n",
            deviceName.c_str(), trackerName.c_str()
        );
        return;
    }

    mSamplingThread = std::make_unique<std::thread>(samplingLoop, this);
}

void SGCTTrackingManager::updateTrackingDevices() {
    for (const std::unique_ptr<SGCTTracker>& tracker : mTrackers) {
        for (size_t j = 0; j < tracker->getNumberOfDevices(); j++) {
            SGCTTrackingDevice* tdPtr = tracker->getDevice(j);
            if (tdPtr->isEnabled() && tdPtr == mHead && mHeadUser) {
                mHeadUser->setTransform(tdPtr->getWorldTransform());
            }
        }
    }
}

void SGCTTrackingManager::addTracker(std::string name) {
    if (!getTracker(name)) {
        mTrackers.push_back(std::make_unique<SGCTTracker>(name));
        gTrackers.push_back(std::vector<VRPNPointer>());

        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Tracking: Tracker '%s' added succesfully\n", name.c_str()
        );
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Tracking: Tracker '%s' already exists\n", name.c_str()
        );
    }
}

void SGCTTrackingManager::addDeviceToCurrentTracker(std::string name) {
    mNumberOfDevices++;

    mTrackers.back()->addDevice(std::move(name), mTrackers.size() - 1);
    gTrackers.back().push_back(VRPNPointer());
}

void SGCTTrackingManager::addSensorToCurrentDevice(std::string address, int id) {
    if (gTrackers.empty() || gTrackers.back().empty()) {
        return;
    }

    std::pair<std::set<std::string>::iterator, bool> retVal = mAddresses.insert(address);

    VRPNPointer& ptr = gTrackers.back().back();
    SGCTTrackingDevice* devicePtr = mTrackers.back()->getLastDevice();

    if (devicePtr) {
        devicePtr->setSensorId(id);

        if (retVal.second && ptr.mSensorDevice == nullptr) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Info,
                "Tracking: Connecting to sensor '%s'\n", address.c_str()
            );
            ptr.mSensorDevice = std::make_unique<vrpn_Tracker_Remote>(address.c_str());
            ptr.mSensorDevice->register_change_handler(
                mTrackers.back().get(),
                updateTracker
            );
        }
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Tracking: Failed to connect to sensor '%s'\n", address.c_str()
        );
    }
}

void SGCTTrackingManager::addButtonsToCurrentDevice(std::string address, int nButtons) {
    if (gTrackers.empty() || gTrackers.back().empty()) {
        return;
    }

    VRPNPointer& ptr = gTrackers.back().back();
    SGCTTrackingDevice* devicePtr = mTrackers.back()->getLastDevice();

    if (ptr.mButtonDevice == nullptr && devicePtr != nullptr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Tracking: Connecting to buttons '%s' on device %s\n",
            address.c_str(), devicePtr->getName().c_str()
        );

        ptr.mButtonDevice = std::make_unique<vrpn_Button_Remote>(address.c_str());
        ptr.mButtonDevice->register_change_handler(devicePtr, updateButton);
        devicePtr->setNumberOfButtons(nButtons);
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Tracking: Failed to connect to buttons '%s'\n", address.c_str()
        );
    }
}

void SGCTTrackingManager::addAnalogsToCurrentDevice(std::string address, int nAxes) {
    if (gTrackers.empty() || gTrackers.back().empty()) {
        return;
    }

    VRPNPointer& ptr = gTrackers.back().back();
    SGCTTrackingDevice* devicePtr = mTrackers.back()->getLastDevice();

    if (ptr.mAnalogDevice == nullptr && devicePtr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Tracking: Connecting to analogs '%s' on device %s\n",
                address.c_str(), devicePtr->getName().c_str()
        );

        ptr.mAnalogDevice = std::make_unique<vrpn_Analog_Remote>(address.c_str());
        ptr.mAnalogDevice->register_change_handler(devicePtr, updateAnalog);
        devicePtr->setNumberOfAxes(nAxes);
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Tracking: Failed to connect to analogs '%s'\n",
            address.c_str()
        );
    }
}

int SGCTTrackingManager::getNumberOfTrackers() const {
    return static_cast<int>(mTrackers.size());
}

int SGCTTrackingManager::getNumberOfDevices() const {
    return mNumberOfDevices;
}

SGCTTrackingDevice* SGCTTrackingManager::getHeadDevicePtr() const {
    return mHead;
}

SGCTTracker* SGCTTrackingManager::getLastTracker() const {
    return !mTrackers.empty() ? mTrackers.back().get() : nullptr;
}

SGCTTracker* SGCTTrackingManager::getTracker(size_t index) const {
    return index < mTrackers.size() ? mTrackers[index].get() : nullptr;
}

SGCTTracker* SGCTTrackingManager::getTracker(const std::string& name) const {
    auto it = std::find_if(
        mTrackers.cbegin(),
        mTrackers.cend(),
        [name](const std::unique_ptr<SGCTTracker>& tracker) {
            return tracker->getName() == name;
        }
    );
    if (it != mTrackers.cend()) {
        return it->get();
    }
    else {
        return nullptr;
    }
}

void SGCTTrackingManager::setEnabled(bool state) {
    for (std::unique_ptr<SGCTTracker>& tracker : mTrackers) {
        tracker->setEnabled(state);
    }
}

void SGCTTrackingManager::setSamplingTime(double t) {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Set sampling time for vrpn loop\n");
#endif
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    mSamplingTime = t;
}

double SGCTTrackingManager::getSamplingTime() const {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get sampling time for vrpn loop\n");
#endif
    std::unique_lock lock(SGCTMutexManager::instance()->mTrackingMutex);
    return mSamplingTime;
}

} // namespace sgct
