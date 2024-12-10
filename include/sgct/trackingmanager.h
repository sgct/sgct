/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__TRACKINGMANAGER__H__
#define __SGCT__TRACKINGMANAGER__H__

#include <sgct/sgctexports.h>
#include <sgct/tracker.h>
#include <memory>
#include <set>
#include <string_view>
#include <thread>
#include <vector>

namespace sgct {

namespace config {
    struct Device;
    struct Tracker;
} // namespace config

class TrackingDevice;
class User;

/**
 * Class that manages tracking systems.
 */
class SGCT_EXPORT TrackingManager {
public:
    static TrackingManager& instance();
    static void destroy();

    void applyDevice(const config::Device& device);
    void applyTracker(const config::Tracker& tracker);

    void startSampling();

    /**
     * Update the user position if headtracking is used. The engine calls this function.
     */
    void updateTrackingDevices();
    void addTracker(std::string name);

    TrackingDevice* headDevice() const;

    const std::vector<std::unique_ptr<Tracker>>& trackers() const;

    void setEnabled(bool state);
    void setSamplingTime(double t);
    double samplingTime() const;

    bool isRunning() const;

private:
    static TrackingManager* _instance;

    TrackingManager() = default;
    ~TrackingManager();

    Tracker* tracker(std::string_view name) const;

    void addDeviceToCurrentTracker(std::string name);
    void addSensorToCurrentDevice(std::string address, int id);
    void addButtonsToCurrentDevice(std::string address, int nButtons);
    void addAnalogsToCurrentDevice(std::string address, int nAxes);

    std::unique_ptr<std::thread> _samplingThread;
    std::vector<std::unique_ptr<Tracker>> _trackers;
    std::set<std::string> _addresses;
    double _samplingTime = 0.0;
    bool _isRunning = true;

    User* _headUser = nullptr;
    TrackingDevice* _head = nullptr;
};

} // namespace sgct

#endif // __SGCT__TRACKINGMANAGER__H__
