/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/trackingmanager.h>

#include <sgct/engine.h>
#include <sgct/clustermanager.h>
#include <sgct/messagehandler.h>
#include <sgct/mutexes.h>
#include <sgct/tracker.h>
#include <sgct/trackingdevice.h>
#include <sgct/user.h>
#ifdef SGCT_HAS_VRPN
#include <vrpn_Tracker.h>
#include <vrpn_Button.h>
#include <vrpn_Analog.h>
#endif // SGCT_HAS_VRPN
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <algorithm>

namespace {
#ifdef SGCT_HAS_VRPN
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
            double t = Engine::getTime();
            for (int i = 0; i < tmPtr->getNumberOfTrackers(); i++) {
                Tracker* trackerPtr = tmPtr->getTracker(i);

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

            tmPtr->setSamplingTime(Engine::getTime() - t);

            // Sleep for 1ms so we don't eat the CPU
            vrpn_SleepMsecs(1);

            if (!isRunning) {
                break;
            }
        }
    }
#endif // SGCT_HAS_VRPN
} // namespace

namespace sgct {

TrackingManager::~TrackingManager() {
#ifdef SGCT_HAS_VRPN
    MessageHandler::printInfo("Disconnecting VRPN");

#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Destructing, setting running to false\n");
#endif
    {
        std::unique_lock lock(core::mutex::Tracking);
        _isRunning = false;
    }

    // destroy thread
    if (_samplingThread) {
        _samplingThread->join();
        _samplingThread = nullptr;
    }

    _trackers.clear();
    gTrackers.clear();
    MessageHandler::printDebug("Done");
#endif // SGCT_HAS_VRPN
}

void TrackingManager::applyDevice(const config::Device& device) {
    addDeviceToCurrentTracker(device.name);

    for (const config::Device::Sensors& s : device.sensors) {
        addSensorToCurrentDevice(s.vrpnAddress, s.identifier);
    }
    for (const config::Device::Buttons& b : device.buttons) {
        addButtonsToCurrentDevice(b.vrpnAddress, b.count);
    }
    for (const config::Device::Axes& a : device.axes) {
        addAnalogsToCurrentDevice(a.vrpnAddress, a.count);
    }
    if (device.offset) {
        getLastTracker()->getLastDevice()->setOffset(*device.offset);
    }
    if (device.transformation) {
        getLastTracker()->getLastDevice()->setTransform(*device.transformation);
    }
}

void TrackingManager::applyTracker(const config::Tracker& tracker) {
    addTracker(tracker.name);

    for (const config::Device& device : tracker.devices) {
        applyDevice(device);
    }
    if (tracker.offset) {
        getLastTracker()->setOffset(*tracker.offset);
    }
    if (tracker.scale) {
        getLastTracker()->setScale(*tracker.scale);
    }
    if (tracker.transformation) {
        getLastTracker()->setTransform(*tracker.transformation);
    }
}

bool TrackingManager::isRunning() const {
#ifdef SGCT_HAS_VRPN
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Checking if tracking is running...\n");
#endif
    std::unique_lock lock(core::mutex::Tracking);
    return _isRunning;
#else
    MessageHandler::printWarning("SGCT compiled without VRPN support");
    return false;
#endif // SGCT_HAS_VRPN
}

void TrackingManager::startSampling() {
#ifdef SGCT_HAS_VRPN
    if (_trackers.empty()) {
        return;
    }
    // find user with headtracking
    _headUser = core::ClusterManager::instance()->getTrackedUser();

    // if tracked user not found
    if (_headUser == nullptr) {
        _headUser = &core::ClusterManager::instance()->getDefaultUser();
    }
        
    // link the head tracker
    const std::string& trackerName = _headUser->getHeadTrackerName();
    Tracker* trackerPtr = getTracker(trackerName);

    const std::string& deviceName = _headUser->getHeadTrackerDeviceName();
    if (trackerPtr) {
        _head = trackerPtr->getDevice(deviceName);
    }

    if (_head == nullptr && !trackerName.empty() && !deviceName.empty()) {
        MessageHandler::printError(
            "Tracking: Failed to set head tracker to %s@%s",
            deviceName.c_str(), trackerName.c_str()
        );
        return;
    }

    _samplingThread = std::make_unique<std::thread>(samplingLoop, this);
#else
    MessageHandler::printWarning("SGCT compiled without VRPN support");
#endif // SGCT_HAS_VRPN
}

void TrackingManager::updateTrackingDevices() {
#ifdef SGCT_HAS_VRPN
    for (const std::unique_ptr<Tracker>& tracker : _trackers) {
        for (int j = 0; j < tracker->getNumberOfDevices(); j++) {
            TrackingDevice* tdPtr = tracker->getDevice(j);
            if (tdPtr->isEnabled() && tdPtr == _head && _headUser) {
                _headUser->setTransform(tdPtr->getWorldTransform());
            }
        }
    }
#else
    MessageHandler::printWarning("SGCT compiled without VRPN support");
#endif // SGCT_HAS_VRPN
}

void TrackingManager::addTracker(std::string name) {
#ifdef SGCT_HAS_VRPN
    if (!getTracker(name)) {
        _trackers.push_back(std::make_unique<Tracker>(name));
        gTrackers.push_back(std::vector<VRPNPointer>());

        MessageHandler::printInfo(
            "Tracking: Tracker '%s' added successfully", name.c_str()
        );
    }
    else {
        MessageHandler::printWarning(
            "Tracking: Tracker '%s' already exists", name.c_str()
        );
    }
#else
    MessageHandler::printWarning("SGCT compiled without VRPN support");
#endif // SGCT_HAS_VRPN
}

void TrackingManager::addDeviceToCurrentTracker(std::string name) {
#ifdef SGCT_HAS_VRPN
    _nDevices++;

    _trackers.back()->addDevice(std::move(name), _trackers.size() - 1);
    gTrackers.back().push_back(VRPNPointer());
#else
    MessageHandler::printWarning("SGCT compiled without VRPN support");
#endif // SGCT_HAS_VRPN
}

void TrackingManager::addSensorToCurrentDevice(std::string address, int id) {
#ifdef SGCT_HAS_VRPN
    if (gTrackers.empty() || gTrackers.back().empty()) {
        return;
    }

    std::pair<std::set<std::string>::iterator, bool> retVal = _addresses.insert(address);

    VRPNPointer& ptr = gTrackers.back().back();
    TrackingDevice* devicePtr = _trackers.back()->getLastDevice();

    if (devicePtr) {
        devicePtr->setSensorId(id);

        if (retVal.second && ptr.mSensorDevice == nullptr) {
            MessageHandler::printInfo(
                "Tracking: Connecting to sensor '%s'", address.c_str()
            );
            ptr.mSensorDevice = std::make_unique<vrpn_Tracker_Remote>(address.c_str());
            ptr.mSensorDevice->register_change_handler(
                _trackers.back().get(),
                updateTracker
            );
        }
    }
    else {
        MessageHandler::printError(
            "Tracking: Failed to connect to sensor '%s'", address.c_str()
        );
    }
#else
    (void)address;
    (void)id;
    MessageHandler::printWarning("SGCT compiled without VRPN support");
#endif // SGCT_HAS_VRPN
}

void TrackingManager::addButtonsToCurrentDevice(std::string address, int nButtons) {
#ifdef SGCT_HAS_VRPN
    if (gTrackers.empty() || gTrackers.back().empty()) {
        return;
    }

    VRPNPointer& ptr = gTrackers.back().back();
    TrackingDevice* devicePtr = _trackers.back()->getLastDevice();

    if (ptr.mButtonDevice == nullptr && devicePtr != nullptr) {
        MessageHandler::printInfo(
            "Tracking: Connecting to buttons '%s' on device %s",
            address.c_str(), devicePtr->getName().c_str()
        );

        ptr.mButtonDevice = std::make_unique<vrpn_Button_Remote>(address.c_str());
        ptr.mButtonDevice->register_change_handler(devicePtr, updateButton);
        devicePtr->setNumberOfButtons(nButtons);
    }
    else {
        MessageHandler::printError(
            "Tracking: Failed to connect to buttons '%s'", address.c_str()
        );
    }
#else
    (void)address;
    (void)nButtons;
    MessageHandler::printWarning("SGCT compiled without VRPN support");
#endif // SGCT_HAS_VRPN
}

void TrackingManager::addAnalogsToCurrentDevice(std::string address, int nAxes) {
#ifdef SGCT_HAS_VRPN
    if (gTrackers.empty() || gTrackers.back().empty()) {
        return;
    }

    VRPNPointer& ptr = gTrackers.back().back();
    TrackingDevice* devicePtr = _trackers.back()->getLastDevice();

    if (ptr.mAnalogDevice == nullptr && devicePtr) {
        MessageHandler::printInfo(
            "Tracking: Connecting to analogs '%s' on device %s",
            address.c_str(), devicePtr->getName().c_str()
        );

        ptr.mAnalogDevice = std::make_unique<vrpn_Analog_Remote>(address.c_str());
        ptr.mAnalogDevice->register_change_handler(devicePtr, updateAnalog);
        devicePtr->setNumberOfAxes(nAxes);
    }
    else {
        MessageHandler::printError(
            "Tracking: Failed to connect to analogs '%s'", address.c_str()
        );
    }
#else
    (void)address;
    (void)nAxes;
    MessageHandler::printWarning("SGCT compiled without VRPN support");
#endif // SGCT_HAS_VRPN
}

int TrackingManager::getNumberOfTrackers() const {
    return static_cast<int>(_trackers.size());
}

int TrackingManager::getNumberOfDevices() const {
    return _nDevices;
}

TrackingDevice* TrackingManager::getHeadDevice() const {
    return _head;
}

Tracker* TrackingManager::getLastTracker() const {
    return !_trackers.empty() ? _trackers.back().get() : nullptr;
}

Tracker* TrackingManager::getTracker(size_t index) const {
    return index < _trackers.size() ? _trackers[index].get() : nullptr;
}

Tracker* TrackingManager::getTracker(const std::string& name) const {
    auto it = std::find_if(
        _trackers.cbegin(),
        _trackers.cend(),
        [name](const std::unique_ptr<Tracker>& tracker) {
            return tracker->getName() == name;
        }
    );
    if (it != _trackers.cend()) {
        return it->get();
    }
    else {
        return nullptr;
    }
}

void TrackingManager::setEnabled(bool state) {
#ifdef SGCT_HAS_VRPN
    for (std::unique_ptr<Tracker>& tracker : _trackers) {
        tracker->setEnabled(state);
    }
#else
    (void)state;
    MessageHandler::printWarning("SGCT compiled without VRPN support");
#endif // SGCT_HAS_VRPN
}

void TrackingManager::setSamplingTime(double t) {
#ifdef SGCT_HAS_VRPN
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Set sampling time for vrpn loop\n");
#endif
    std::unique_lock lock(core::mutex::Tracking);
    _samplingTime = t;
#else
    (void)t;
    MessageHandler::printWarning("SGCT compiled without VRPN support");
#endif // SGCT_HAS_VRPN
}

double TrackingManager::getSamplingTime() const {
#ifdef SGCT_HAS_VRPN
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get sampling time for vrpn loop\n");
#endif
    std::unique_lock lock(core::mutex::Tracking);
    return _samplingTime;
#else
    MessageHandler::printWarning("SGCT compiled without VRPN support");
    return 0.0;
#endif // SGCT_HAS_VRPN
}

} // namespace sgct
