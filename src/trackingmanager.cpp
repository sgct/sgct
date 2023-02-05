/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/trackingmanager.h>

#include <sgct/config.h>
#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/fmt.h>
#include <sgct/log.h>
#include <sgct/mutexes.h>
#include <sgct/profiling.h>
#include <sgct/trackingdevice.h>
#include <sgct/user.h>
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wsuggest-override"
#endif // __GNUC__
#include <vrpn_Tracker.h>
#include <vrpn_Button.h>
#include <vrpn_Analog.h>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif // __GNUC__
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <algorithm>

namespace {
    struct VRPNPointer {
        std::unique_ptr<vrpn_Tracker_Remote> sensorDevice;
        std::unique_ptr<vrpn_Analog_Remote> analogDevice;
        std::unique_ptr<vrpn_Button_Remote> buttonDevice;
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

        sgct::vec3 pos = sgct::vec3{
            static_cast<float>(t.pos[0] * tracker->scale()),
            static_cast<float>(t.pos[1] * tracker->scale()),
            static_cast<float>(t.pos[2] * tracker->scale())
        };
        sgct::quat rotation = sgct::quat{
            static_cast<float>(t.quat[0]),
            static_cast<float>(t.quat[1]),
            static_cast<float>(t.quat[2]),
            static_cast<float>(t.quat[3])
        };
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
            for (size_t i = 0; i < tm->trackers().size(); ++i) {
                sgct::Tracker* tracker = tm->trackers()[i].get();
                if (tracker == nullptr) {
                    continue;
                }
                for (size_t j = 0; j < tracker->devices().size(); ++j) {
                    if (!tracker->devices()[j]->isEnabled()) {
                        continue;
                    }

                    const VRPNPointer& ptr = gTrackers[i][j];
                    if (ptr.sensorDevice) {
                        ptr.sensorDevice->mainloop();
                    }
                    if (ptr.analogDevice) {
                        ptr.analogDevice->mainloop();
                    }
                    if (ptr.buttonDevice) {
                        ptr.buttonDevice->mainloop();
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
    Log::Info("Disconnecting VRPN");

    {
        std::unique_lock lock(mutex::Tracking);
        _isRunning = false;
    }

    // destroy thread
    if (_samplingThread) {
        _samplingThread->join();
        _samplingThread = nullptr;
    }

    _trackers.clear();
    gTrackers.clear();
    Log::Debug("Done");
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
        _trackers.back()->devices().back()->setOffset(*device.offset);
    }
    if (device.transformation) {
        _trackers.back()->devices().back()->setTransform(*device.transformation);
    }
}

void TrackingManager::applyTracker(const config::Tracker& tracker) {
    ZoneScoped

    addTracker(tracker.name);

    for (const config::Device& device : tracker.devices) {
        applyDevice(device);
    }
    if (tracker.offset) {
        _trackers.back()->setOffset(*tracker.offset);
    }
    if (tracker.scale) {
        _trackers.back()->setScale(*tracker.scale);
    }
    if (tracker.transformation) {
        _trackers.back()->setTransform(*tracker.transformation);
    }
}

bool TrackingManager::isRunning() const {
    std::unique_lock lock(mutex::Tracking);
    return _isRunning;
}

void TrackingManager::startSampling() {
    if (_trackers.empty()) {
        return;
    }
    // find user with headtracking
    _headUser = ClusterManager::instance().trackedUser();

    // if tracked user not found
    if (_headUser == nullptr) {
        _headUser = &ClusterManager::instance().defaultUser();
    }

    // link the head tracker
    const std::string& trackerName = _headUser->headTrackerName();
    Tracker* tr = tracker(trackerName);

    const std::string& deviceName = _headUser->headTrackerDeviceName();
    if (tr) {
        _head = tr->device(deviceName);
    }

    if (_head == nullptr && !trackerName.empty() && !deviceName.empty()) {
        Log::Error(fmt::format(
            "Failed to set head tracker to {}@{}", deviceName, trackerName
        ));
        return;
    }

    _samplingThread = std::make_unique<std::thread>(samplingLoop, this);
}

void TrackingManager::updateTrackingDevices() {
    ZoneScoped

    for (const std::unique_ptr<Tracker>& tracker : _trackers) {
        for (const std::unique_ptr<TrackingDevice>& device : tracker->devices()) {
            if (device->isEnabled() && device.get() == _head && _headUser) {
                _headUser->setTransform(device->worldTransform());
            }
        }
    }
}

void TrackingManager::addTracker(std::string name) {
    if (!tracker(name)) {
        _trackers.push_back(std::make_unique<Tracker>(name));
        gTrackers.emplace_back(std::vector<VRPNPointer>());
        Log::Info(fmt::format("Tracker '{}' added successfully", name));
    }
    else {
        Log::Warning(fmt::format("Tracker '{}' already exists", name));
    }
}

void TrackingManager::addDeviceToCurrentTracker(std::string name) {
    _trackers.back()->addDevice(std::move(name), static_cast<int>(_trackers.size() - 1));
    gTrackers.back().emplace_back(VRPNPointer());
}

void TrackingManager::addSensorToCurrentDevice(std::string address, int id) {
    if (gTrackers.empty() || gTrackers.back().empty()) {
        return;
    }

    std::pair<std::set<std::string>::iterator, bool> retVal = _addresses.insert(address);

    VRPNPointer& ptr = gTrackers.back().back();
    TrackingDevice* device = _trackers.back()->devices().back().get();

    if (device) {
        device->setSensorId(id);

        if (retVal.second && ptr.sensorDevice == nullptr) {
            Log::Info(fmt::format("Connecting to sensor '{}'", address));
            ptr.sensorDevice = std::make_unique<vrpn_Tracker_Remote>(address.c_str());
            ptr.sensorDevice->register_change_handler(
                _trackers.back().get(),
                updateTracker
            );
        }
    }
    else {
        Log::Error(fmt::format("Failed to connect to sensor '{}'", address));
    }
}

void TrackingManager::addButtonsToCurrentDevice(std::string address, int nButtons) {
    if (gTrackers.empty() || gTrackers.back().empty()) {
        return;
    }

    VRPNPointer& ptr = gTrackers.back().back();
    TrackingDevice* device = _trackers.back()->devices().back().get();

    if (ptr.buttonDevice == nullptr && device) {
        Log::Info(fmt::format(
            "Connecting to buttons '{}' on device {}", address, device->name()
        ));
        ptr.buttonDevice = std::make_unique<vrpn_Button_Remote>(address.c_str());
        ptr.buttonDevice->register_change_handler(device, updateButton);
        device->setNumberOfButtons(nButtons);
    }
    else {
        Log::Error(fmt::format("Failed to connect to buttons '{}'", address));
    }
}

void TrackingManager::addAnalogsToCurrentDevice(std::string address, int nAxes) {
    if (gTrackers.empty() || gTrackers.back().empty()) {
        return;
    }

    VRPNPointer& ptr = gTrackers.back().back();
    TrackingDevice* device = _trackers.back()->devices().back().get();

    if (ptr.analogDevice == nullptr && device) {
        Log::Info(fmt::format(
            "Connecting to analog '{}' on device {}", address, device->name()
        ));

        ptr.analogDevice = std::make_unique<vrpn_Analog_Remote>(address.c_str());
        ptr.analogDevice->register_change_handler(device, updateAnalog);
        device->setNumberOfAxes(nAxes);
    }
    else {
        Log::Error(fmt::format("Failed to connect to analogs '{}'", address));
    }
}

const std::vector<std::unique_ptr<Tracker>>& TrackingManager::trackers() const {
    return _trackers;
}

TrackingDevice* TrackingManager::headDevice() const {
    return _head;
}

Tracker* TrackingManager::tracker(std::string_view name) const {
    const auto it = std::find_if(
        _trackers.cbegin(),
        _trackers.cend(),
        [name](const std::unique_ptr<Tracker>& tr) { return tr->name() == name; }
    );
    return it != _trackers.cend() ? it->get() : nullptr;
}

void TrackingManager::setEnabled(bool state) {
    for (std::unique_ptr<Tracker>& tracker : _trackers) {
        tracker->setEnabled(state);
    }
}

void TrackingManager::setSamplingTime(double t) {
    std::unique_lock lock(mutex::Tracking);
    _samplingTime = t;
}

double TrackingManager::samplingTime() const {
    std::unique_lock lock(mutex::Tracking);
    return _samplingTime;
}

} // namespace sgct
