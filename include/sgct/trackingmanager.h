/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__TRACKING_MANAGER__H__
#define __SGCT__TRACKING_MANAGER__H__

#include <sgct/config.h>
#include <sgct/tracker.h>
#include <memory>
#include <set>
#include <thread>
#include <vector>

namespace sgct::core { class User; }

namespace sgct {

class TrackingDevice;

/**
 * Class that manages tracking systems
 */
class TrackingManager {
public:
    ~TrackingManager();
    
    void applyDevice(const config::Device& device);
    void applyTracker(const config::Tracker& tracker);

    void startSampling();

    /**
     * Update the user position if headtracking is used. This function is called from the
     * engine.
     */
    void updateTrackingDevices();
    void addTracker(std::string name);
    void addDeviceToCurrentTracker(std::string name);
    void addSensorToCurrentDevice(std::string address, int id);
    void addButtonsToCurrentDevice(std::string address, int numOfButtons);
    void addAnalogsToCurrentDevice(std::string address, int numOfAxes);
    
    int getNumberOfTrackers() const;
    int getNumberOfDevices() const;
    TrackingDevice* getHeadDevice() const;

    Tracker* getLastTracker() const;
    Tracker* getTracker(size_t index) const;
    Tracker* getTracker(const std::string& name) const;

    void setEnabled(bool state);
    void setSamplingTime(double t);
    double getSamplingTime() const;

    bool isRunning() const;

private:
    std::unique_ptr<std::thread> _samplingThread;
    std::vector<std::unique_ptr<Tracker>> _trackers;
    std::set<std::string> _addresses;
    double _samplingTime = 0.0;
    bool _isRunning = true;

    core::User* _headUser = nullptr;
    TrackingDevice* _head = nullptr;
    int _nDevices = 0;
};

} // namespace sgct

#endif // __SGCT__TRACKING_MANAGER__H__
