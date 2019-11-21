/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/trackingmanager.h>

#include <sgct/config.h>
#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/mutexes.h>
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
        if (userdata == nullptr) {
            return;
        }

        sgct::Tracker* tracker = reinterpret_cast<sgct::Tracker*>(userdata);
        sgct::TrackingDevice* device = tracker->getDeviceBySensorId(t.sensor);

        if (device == nullptr) {
            return;
        }

        glm::dvec3 pos = glm::dvec3(t.pos[0], t.pos[1], t.pos[2]) * tracker->getScale();
        glm::dquat rotation(t.quat[3], t.quat[0], t.quat[1], t.quat[2]);
        device->setSensorTransform(pos, rotation);
    }

    void VRPN_CALLBACK updateButton(void* userdata, const vrpn_BUTTONCB b) {
        sgct::TrackingDevice* device = reinterpret_cast<sgct::TrackingDevice*>(userdata);
        device->setButtonValue(b.state != 0, b.button);
    }

    void VRPN_CALLBACK updateAnalog(void* userdata, const vrpn_ANALOGCB a) {
        sgct::TrackingDevice* tdPtr = reinterpret_cast<sgct::TrackingDevice*>(userdata);
        tdPtr->setAnalogValue(a.channel, static_cast<int>(a.num_channel));
    }

    void samplingLoop(void* arg) {
        sgct::TrackingManager* tm = reinterpret_cast<sgct::TrackingManager*>(arg);

        while (true) {
            const double t = sgct::Engine::getTime();
            for (int i = 0; i < tm->getNumberOfTrackers(); ++i) {
                sgct::Tracker* tracker = tm->getTracker(i);

                if (tracker == nullptr) {
                    continue;
                }
                for (int j = 0; j < tracker->getNumberOfDevices(); ++j) {
                    if (!tracker->getDevice(j)->isEnabled()) {
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

            const bool isRunning = tm->isRunning();
            tm->setSamplingTime(sgct::Engine::getTime() - t);

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

TrackingManager* TrackingManager::_instance = nullptr;

TrackingManager& TrackingManager::instance() {
    if (!_instance) {
        _instance = new TrackingManager;
    }
    return *_instance;
}

void TrackingManager::destroy() {
    delete _instance;
    _instance = nullptr;
}

TrackingManager::~TrackingManager() {
#ifdef SGCT_HAS_VRPN
    MessageHandler::printInfo("Disconnecting VRPN");

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
    _headUser = core::ClusterManager::instance().getTrackedUser();

    // if tracked user not found
    if (_headUser == nullptr) {
        _headUser = &core::ClusterManager::instance().getDefaultUser();
    }

    // link the head tracker
    const std::string& trackerName = _headUser->getHeadTrackerName();
    Tracker* tracker = getTracker(trackerName);

    const std::string& deviceName = _headUser->getHeadTrackerDeviceName();
    if (tracker) {
        _head = tracker->getDevice(deviceName);
    }

    if (_head == nullptr && !trackerName.empty() && !deviceName.empty()) {
        MessageHandler::printError(
            "Failed to set head tracker to %s@%s", deviceName.c_str(), trackerName.c_str()
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
        for (int j = 0; j < tracker->getNumberOfDevices(); ++j) {
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
        gTrackers.emplace_back(std::vector<VRPNPointer>());
        MessageHandler::printInfo("Tracker '%s' added successfully", name.c_str());
    }
    else {
        MessageHandler::printWarning("Tracker '%s' already exists", name.c_str());
    }
#else
    MessageHandler::printWarning("SGCT compiled without VRPN support");
#endif // SGCT_HAS_VRPN
}

void TrackingManager::addDeviceToCurrentTracker(std::string name) {
#ifdef SGCT_HAS_VRPN
    _trackers.back()->addDevice(std::move(name), static_cast<int>(_trackers.size() - 1));
    gTrackers.back().emplace_back(VRPNPointer());
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
    TrackingDevice* device = _trackers.back()->getLastDevice();

    if (device) {
        device->setSensorId(id);

        if (retVal.second && ptr.mSensorDevice == nullptr) {
            MessageHandler::printInfo("Connecting to sensor '%s'", address.c_str());
            ptr.mSensorDevice = std::make_unique<vrpn_Tracker_Remote>(address.c_str());
            ptr.mSensorDevice->register_change_handler(
                _trackers.back().get(),
                updateTracker
            );
        }
    }
    else {
        MessageHandler::printError("Failed to connect to sensor '%s'", address.c_str());
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
    TrackingDevice* device = _trackers.back()->getLastDevice();

    if (ptr.mButtonDevice == nullptr && device) {
        MessageHandler::printInfo(
            "Connecting to buttons '%s' on device %s",
            address.c_str(), device->getName().c_str()
        );
        ptr.mButtonDevice = std::make_unique<vrpn_Button_Remote>(address.c_str());
        ptr.mButtonDevice->register_change_handler(device, updateButton);
        device->setNumberOfButtons(nButtons);
    }
    else {
        MessageHandler::printError("Failed to connect to buttons '%s'", address.c_str());
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
    TrackingDevice* device = _trackers.back()->getLastDevice();

    if (ptr.mAnalogDevice == nullptr && device) {
        MessageHandler::printInfo(
            "Connecting to analogs '%s' on device %s",
            address.c_str(), device->getName().c_str()
        );

        ptr.mAnalogDevice = std::make_unique<vrpn_Analog_Remote>(address.c_str());
        ptr.mAnalogDevice->register_change_handler(device, updateAnalog);
        device->setNumberOfAxes(nAxes);
    }
    else {
        MessageHandler::printError("Failed to connect to analogs '%s'", address.c_str());
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

TrackingDevice* TrackingManager::getHeadDevice() const {
    return _head;
}

Tracker* TrackingManager::getLastTracker() const {
    return !_trackers.empty() ? _trackers.back().get() : nullptr;
}

Tracker* TrackingManager::getTracker(int index) const {
    return index < static_cast<int>(_trackers.size()) ? _trackers[index].get() : nullptr;
}

Tracker* TrackingManager::getTracker(const std::string& name) const {
    const auto it = std::find_if(
        _trackers.cbegin(),
        _trackers.cend(),
        [name](const std::unique_ptr<Tracker>& tr) { return tr->getName() == name; }
    );
    return it != _trackers.cend() ? it->get() : nullptr;
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
    std::unique_lock lock(core::mutex::Tracking);
    _samplingTime = t;
#else
    (void)t;
    MessageHandler::printWarning("SGCT compiled without VRPN support");
#endif // SGCT_HAS_VRPN
}

double TrackingManager::getSamplingTime() const {
#ifdef SGCT_HAS_VRPN
    std::unique_lock lock(core::mutex::Tracking);
    return _samplingTime;
#else
    MessageHandler::printWarning("SGCT compiled without VRPN support");
    return 0.0;
#endif // SGCT_HAS_VRPN
}

} // namespace sgct
