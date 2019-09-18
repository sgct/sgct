/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/trackingmanager.h>

#include <sgct/engine.h>
#include <sgct/clustermanager.h>
#include <sgct/messagehandler.h>
#include <sgct/mutexmanager.h>
#include <sgct/tracker.h>
#include <sgct/trackingdevice.h>
#include <sgct/user.h>
#include <vrpn/vrpn_Tracker.h>
#include <vrpn/vrpn_Button.h>
#include <vrpn/vrpn_Analog.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
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

        Tracker* trackerPtr = reinterpret_cast<Tracker*>(userdata);
        TrackingDevice* devicePtr = trackerPtr->getDeviceBySensorId(t.sensor);

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
        TrackingDevice* devicePtr = reinterpret_cast<TrackingDevice*>(userdata);

        devicePtr->setButtonVal(b.state != 0, b.button);
    }

    void VRPN_CALLBACK updateAnalog(void* userdata, const vrpn_ANALOGCB a) {
        using namespace sgct;
        TrackingDevice* tdPtr = reinterpret_cast<TrackingDevice*>(userdata);
        tdPtr->setAnalogVal(a.channel, static_cast<int>(a.num_channel));
    }

    void samplingLoop(void* arg) {
        using namespace sgct;
        TrackingManager* tmPtr = reinterpret_cast<TrackingManager*>(arg);

        while (true) {
            double t = sgct::Engine::getTime();
            for (int i = 0; i < tmPtr->getNumberOfTrackers(); i++) {
                sgct::Tracker* trackerPtr = tmPtr->getTracker(i);

                if (trackerPtr == nullptr) {
                    continue;
                }
                for (int j = 0; j < trackerPtr->getNumberOfDevices(); j++) {
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

TrackingManager::~TrackingManager() {
    MessageHandler::instance()->print(
        MessageHandler::Level::Info, "Disconnecting VRPN\n"
    );

#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Destructing, setting running to false\n");
#endif
    {
        std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
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


bool TrackingManager::isRunning() const {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Checking if tracking is running...\n");
#endif
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return mRunning;
}

void TrackingManager::startSampling() {
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
    Tracker* trackerPtr = getTracker(trackerName);

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

void TrackingManager::updateTrackingDevices() {
    for (const std::unique_ptr<Tracker>& tracker : mTrackers) {
        for (int j = 0; j < tracker->getNumberOfDevices(); j++) {
            TrackingDevice* tdPtr = tracker->getDevice(j);
            if (tdPtr->isEnabled() && tdPtr == mHead && mHeadUser) {
                mHeadUser->setTransform(tdPtr->getWorldTransform());
            }
        }
    }
}

void TrackingManager::addTracker(std::string name) {
    if (!getTracker(name)) {
        mTrackers.push_back(std::make_unique<Tracker>(name));
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

void TrackingManager::addDeviceToCurrentTracker(std::string name) {
    mNumberOfDevices++;

    mTrackers.back()->addDevice(std::move(name), mTrackers.size() - 1);
    gTrackers.back().push_back(VRPNPointer());
}

void TrackingManager::addSensorToCurrentDevice(std::string address, int id) {
    if (gTrackers.empty() || gTrackers.back().empty()) {
        return;
    }

    std::pair<std::set<std::string>::iterator, bool> retVal = mAddresses.insert(address);

    VRPNPointer& ptr = gTrackers.back().back();
    TrackingDevice* devicePtr = mTrackers.back()->getLastDevice();

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

void TrackingManager::addButtonsToCurrentDevice(std::string address, int nButtons) {
    if (gTrackers.empty() || gTrackers.back().empty()) {
        return;
    }

    VRPNPointer& ptr = gTrackers.back().back();
    TrackingDevice* devicePtr = mTrackers.back()->getLastDevice();

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

void TrackingManager::addAnalogsToCurrentDevice(std::string address, int nAxes) {
    if (gTrackers.empty() || gTrackers.back().empty()) {
        return;
    }

    VRPNPointer& ptr = gTrackers.back().back();
    TrackingDevice* devicePtr = mTrackers.back()->getLastDevice();

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

int TrackingManager::getNumberOfTrackers() const {
    return static_cast<int>(mTrackers.size());
}

int TrackingManager::getNumberOfDevices() const {
    return mNumberOfDevices;
}

TrackingDevice* TrackingManager::getHeadDevice() const {
    return mHead;
}

Tracker* TrackingManager::getLastTracker() const {
    return !mTrackers.empty() ? mTrackers.back().get() : nullptr;
}

Tracker* TrackingManager::getTracker(size_t index) const {
    return index < mTrackers.size() ? mTrackers[index].get() : nullptr;
}

Tracker* TrackingManager::getTracker(const std::string& name) const {
    auto it = std::find_if(
        mTrackers.cbegin(),
        mTrackers.cend(),
        [name](const std::unique_ptr<Tracker>& tracker) {
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

void TrackingManager::setEnabled(bool state) {
    for (std::unique_ptr<Tracker>& tracker : mTrackers) {
        tracker->setEnabled(state);
    }
}

void TrackingManager::setSamplingTime(double t) {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Set sampling time for vrpn loop\n");
#endif
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    mSamplingTime = t;
}

double TrackingManager::getSamplingTime() const {
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get sampling time for vrpn loop\n");
#endif
    std::unique_lock lock(MutexManager::instance()->mTrackingMutex);
    return mSamplingTime;
}

} // namespace sgct
