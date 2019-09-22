/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/config.h>

#include <sgct/messagehandler.h>

namespace sgct::config {

bool validateUser(const User& user) {
    bool success = true;

    if (!user.position) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "User is missing 'Position'\n"
        );

        success = false;
    }

    return success;
}

bool validateCapture(const Capture& capture) {
    bool success = true;

    return success;
}

bool validateScene(const Scene& scene) {
    bool success = true;

    return success;
}

bool validateSettings(const Settings& settings) {
    bool success = true;

    // if (trim > 0.f) {
    return success;
}

bool validateDevice(const Device& device) {
    bool success = true;

    return success;
}

bool validateTracker(const Tracker& tracker) {
    bool success = true;

    return success;
}

bool validatePlanarProjection(const PlanarProjection& proj) {
    bool success = true;

    return success;
}

bool validateFisheyeProjection(const FisheyeProjection& proj) {
    bool success = true;
    // quality power of 2 and > 0
    return success;
}

bool validateSphericalMirrorProjection(const SphericalMirrorProjection& proj) {
    bool success = true;
    // quality power of 2 and > 0
    return success;
}

bool validateSpoutOutputProjection(const SpoutOutputProjection& proj) {
    bool success = true;
    // quality power of 2 and > 0
    return success;
}

bool validateProjectionPlane(const ProjectionPlane& proj) {
    bool success = true;
    return success;
}

bool validateMpcdiProjection(const MpcdiProjection& proj) {
    bool success = true;
    // quality power of 2 and > 0
    return success;
}

bool validateViewport(const Viewport& viewport) {
    bool success = true;

    return success;
}

bool validateWindow(const Window& viewport) {
    bool success = true;

    //if (gamma && gamma > 0.1f) {
    //    win.setGamma(*gamma);
    //}

    //std::optional<float> contrast = parseValue<float>(*element, "contrast");
    //if (contrast && contrast > 0.f) {
    //    win.setContrast(*contrast);
    //}

    //std::optional<float> brightness = parseValue<float>(*element, "brightness");
    //if (brightness && brightness > 0.f) {
    //    win.setBrightness(*brightness);
    //}

    return success;
}

bool validateNodes(const Node& node) {
    bool success = true;
    return success;
}

bool validateCluster(const Cluster& cluster) {
    bool success = true;
    return success;
}



} // namespace sgct::config
