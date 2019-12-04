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
#include <sgct/logger.h>
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
        sgct::TrackingDevice* device = tracker->deviceBySensorId(t.sensor);

        if (device == nullptr) {
            return;
        }

        glm::dvec3 pos = glm::dvec3(t.pos[0], t.pos[1], t.pos[2]) * tracker->scale();
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
            for (int i = 0; i < tm->numberOfTrackers(); ++i) {
                sgct::Tracker* tracker = tm->tracker(i);

                if (tracker == nullptr) {
                    continue;
                }
                for (int j = 0; j < tracker->numberOfDevices(); ++j) {
                    if (!tracker->device(j)->isEnabled()) {
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
    Logger::Info("Disconnecting VRPN");

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
    Logger::Debug("Done");
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
        lastTracker()->lastDevice()->setOffset(*device.offset);
    }
    if (device.transformation) {
        lastTracker()->lastDevice()->setTransform(*device.transformation);
    }
}

void TrackingManager::applyTracker(const config::Tracker& tracker) {
    addTracker(tracker.name);

    for (const config::Device& device : tracker.devices) {
        applyDevice(device);
    }
    if (tracker.offset) {
        lastTracker()->setOffset(*tracker.offset);
    }
    if (tracker.scale) {
        lastTracker()->setScale(*tracker.scale);
    }
    if (tracker.transformation) {
        lastTracker()->setTransform(*tracker.transformation);
    }
}

bool TrackingManager::isRunning() const {
#ifdef SGCT_HAS_VRPN
    std::unique_lock lock(core::mutex::Tracking);
    return _isRunning;
#else
    Logger::Warning("SGCT compiled without VRPN support");
    return false;
#endif // SGCT_HAS_VRPN
}

void TrackingManager::startSampling() {
#ifdef SGCT_HAS_VRPN
    if (_trackers.empty()) {
        return;
    }
    // find user with headtracking
    _headUser = core::ClusterManager::instance().trackedUser();

    // if tracked user not found
    if (_headUser == nullptr) {
        _headUser = &core::ClusterManager::instance().defaultUser();
    }

    // link the head tracker
    const std::string& trackerName = _headUser->headTrackerName();
    Tracker* tr = tracker(trackerName);

    const std::string& deviceName = _headUser->headTrackerDeviceName();
    if (tr) {
        _head = tr->device(deviceName);
    }

    if (_head == nullptr && !trackerName.empty() && !deviceName.empty()) {
        Logger::Error(
            "Failed to set head tracker to %s@%s", deviceName.c_str(), trackerName.c_str()
        );
        return;
    }

    _samplingThread = std::make_unique<std::thread>(samplingLoop, this);
#else
    Logger::Warning("SGCT compiled without VRPN support");
#endif // SGCT_HAS_VRPN
}

void TrackingManager::updateTrackingDevices() {
#ifdef SGCT_HAS_VRPN
    for (const std::unique_ptr<Tracker>& tracker : _trackers) {
        for (int j = 0; j < tracker->numberOfDevices(); ++j) {
            TrackingDevice* tdPtr = tracker->device(j);
            if (tdPtr->isEnabled() && tdPtr == _head && _headUser) {
                _headUser->setTransform(tdPtr->worldTransform());
            }
        }
    }
#else
    Logger::Warning("SGCT compiled without VRPN support");
#endif // SGCT_HAS_VRPN
}

void TrackingManager::addTracker(std::string name) {
#ifdef SGCT_HAS_VRPN
    if (!tracker(name)) {
        _trackers.push_back(std::make_unique<Tracker>(name));
        gTrackers.emplace_back(std::vector<VRPNPointer>());
        Logger::Info("Tracker '%s' added successfully", name.c_str());
    }
    else {
        Logger::Warning("Tracker '%s' already exists", name.c_str());
    }
#else
    Logger::Warning("SGCT compiled without VRPN support");
#endif // SGCT_HAS_VRPN
}

void TrackingManager::addDeviceToCurrentTracker(std::string name) {
#ifdef SGCT_HAS_VRPN
    _trackers.back()->addDevice(std::move(name), static_cast<int>(_trackers.size() - 1));
    gTrackers.back().emplace_back(VRPNPointer());
#else
    Logger::Warning("SGCT compiled without VRPN support");
#endif // SGCT_HAS_VRPN
}

void TrackingManager::addSensorToCurrentDevice(std::string address, int id) {
#ifdef SGCT_HAS_VRPN
    if (gTrackers.empty() || gTrackers.back().empty()) {
        return;
    }

    std::pair<std::set<std::string>::iterator, bool> retVal = _addresses.insert(address);

    VRPNPointer& ptr = gTrackers.back().back();
    TrackingDevice* device = _trackers.back()->lastDevice();

    if (device) {
        device->setSensorId(id);

        if (retVal.second && ptr.mSensorDevice == nullptr) {
            Logger::Info("Connecting to sensor '%s'", address.c_str());
            ptr.mSensorDevice = std::make_unique<vrpn_Tracker_Remote>(address.c_str());
            ptr.mSensorDevice->register_change_handler(
                _trackers.back().get(),
                updateTracker
            );
        }
    }
    else {
        Logger::Error("Failed to connect to sensor '%s'", address.c_str());
    }
#else
    (void)address;
    (void)id;
    Logger::Warning("SGCT compiled without VRPN support");
#endif // SGCT_HAS_VRPN
}

void TrackingManager::addButtonsToCurrentDevice(std::string address, int nButtons) {
#ifdef SGCT_HAS_VRPN
    if (gTrackers.empty() || gTrackers.back().empty()) {
        return;
    }

    VRPNPointer& ptr = gTrackers.back().back();
    TrackingDevice* device = _trackers.back()->lastDevice();

    if (ptr.mButtonDevice == nullptr && device) {
        Logger::Info(
            "Connecting to buttons '%s' on device %s",
            address.c_str(), device->name().c_str()
        );
        ptr.mButtonDevice = std::make_unique<vrpn_Button_Remote>(address.c_str());
        ptr.mButtonDevice->register_change_handler(device, updateButton);
        device->setNumberOfButtons(nButtons);
    }
    else {
        Logger::Error("Failed to connect to buttons '%s'", address.c_str());
    }
#else
    (void)address;
    (void)nButtons;
    Logger::Warning("SGCT compiled without VRPN support");
#endif // SGCT_HAS_VRPN
}

void TrackingManager::addAnalogsToCurrentDevice(std::string address, int nAxes) {
#ifdef SGCT_HAS_VRPN
    if (gTrackers.empty() || gTrackers.back().empty()) {
        return;
    }

    VRPNPointer& ptr = gTrackers.back().back();
    TrackingDevice* device = _trackers.back()->lastDevice();

    if (ptr.mAnalogDevice == nullptr && device) {
        Logger::Info(
            "Connecting to analogs '%s' on device %s",
            address.c_str(), device->name().c_str()
        );

        ptr.mAnalogDevice = std::make_unique<vrpn_Analog_Remote>(address.c_str());
        ptr.mAnalogDevice->register_change_handler(device, updateAnalog);
        device->setNumberOfAxes(nAxes);
    }
    else {
        Logger::Error("Failed to connect to analogs '%s'", address.c_str());
    }
#else
    (void)address;
    (void)nAxes;
    Logger::Warning("SGCT compiled without VRPN support");
#endif // SGCT_HAS_VRPN
}

int TrackingManager::numberOfTrackers() const {
    return static_cast<int>(_trackers.size());
}

TrackingDevice* TrackingManager::headDevice() const {
    return _head;
}

Tracker* TrackingManager::lastTracker() const {
    return !_trackers.empty() ? _trackers.back().get() : nullptr;
}

Tracker* TrackingManager::tracker(int index) const {
    return index < static_cast<int>(_trackers.size()) ? _trackers[index].get() : nullptr;
}

Tracker* TrackingManager::tracker(const std::string& name) const {
    const auto it = std::find_if(
        _trackers.cbegin(),
        _trackers.cend(),
        [name](const std::unique_ptr<Tracker>& tr) { return tr->name() == name; }
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
    Logger::Warning("SGCT compiled without VRPN support");
#endif // SGCT_HAS_VRPN
}

void TrackingManager::setSamplingTime(double t) {
#ifdef SGCT_HAS_VRPN
    std::unique_lock lock(core::mutex::Tracking);
    _samplingTime = t;
#else
    (void)t;
    Logger::Warning("SGCT compiled without VRPN support");
#endif // SGCT_HAS_VRPN
}

double TrackingManager::samplingTime() const {
#ifdef SGCT_HAS_VRPN
    std::unique_lock lock(core::mutex::Tracking);
    return _samplingTime;
#else
    Logger::Warning("SGCT compiled without VRPN support");
    return 0.0;
#endif // SGCT_HAS_VRPN
}

} // namespace sgct
