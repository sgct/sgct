/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/config.h>

#include <sgct/messagehandler.h>
#include <glm/vector_relational.hpp>
#include <algorithm>
#include <variant>

namespace {
    // Helper structs for the visitor pattern of the std::variant on projections
    template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
} // namespace

namespace sgct::config {

bool validateUser(const User& user) {
    bool success = true;

    if (user.tracking && user.tracking->device.empty()) {
        MessageHandler::printError("Tracking device name must not be empty");
        success = false;
    }
    if (user.tracking && user.tracking->tracker.empty()) {
        MessageHandler::printError("Tracking tracker name must not be empty");
        success = false;
    }

    return success;
}

bool validateCapture(const Capture& capture) {
    bool success = true;

    if (capture.monoPath && capture.monoPath->empty()) {
        MessageHandler::printError("Mono path must not be empty");
        success = false;
    }
    if (capture.monoPath && capture.leftPath->empty()) {
        MessageHandler::printError("Left path must not be empty");
        success = false;
    }
    if (capture.monoPath && capture.rightPath->empty()) {
        MessageHandler::printError("Right path must not be empty");
        success = false;
    }

    return success;
}

bool validateScene(const Scene&) {
    return true;
}

bool validateSettings(const Settings& settings) {
    bool success = true;

    if (settings.display && settings.display->swapInterval &&
        *settings.display->swapInterval < 0)
    {
        MessageHandler::printError("Swap interval must not be negative");
        success = false;
    }
    if (settings.display && settings.display->refreshRate &&
        *settings.display->refreshRate < 0)
    {
        MessageHandler::printError("Refresh rate must not be negative");
        success = false;
    }

    if (settings.osdText && settings.osdText->name && settings.osdText->name->empty()) {
        MessageHandler::printError("OSDText font name must not be negative");
        success = false;
    }

    if (settings.osdText && settings.osdText->path && settings.osdText->path->empty()) {
        MessageHandler::printError("OSDText font path must not be negative");
        success = false;
    }

    if (settings.osdText && settings.osdText->size && *settings.osdText->size < 0) {
        MessageHandler::printError("OSDText font size must not be negative");
        success = false;
    }

    if (settings.fxaa && settings.fxaa->trim && *settings.fxaa->trim <= 0.f) {
        MessageHandler::printError("FXAA trim must be postive");
        success = false;
    }
    return success;
}

bool validateDevice(const Device& device) {
    bool success = true;

    if (device.name.empty()) {
        MessageHandler::printError("Device name must not be empty");
        success = false;
    }

    auto validateAddress = [](const auto& v) -> bool {
        if (v.vrpnAddress.empty()) {
            MessageHandler::printError("VRPN address must not be empty");
            return false;
        }
        else {
            return true;
        }
    };

    success &= std::all_of(device.sensors.begin(), device.sensors.end(), validateAddress);
    success &= std::all_of(device.buttons.begin(), device.buttons.end(), validateAddress);
    success &= std::all_of(device.axes.begin(), device.axes.end(), validateAddress);

    return success;
}

bool validateTracker(const Tracker& tracker) {
    bool success = true;

    if (tracker.name.empty()) {
        MessageHandler::printError("Tracker name must not be empty");
        success = false;
    }

    success &= std::all_of(
        tracker.devices.begin(),
        tracker.devices.end(),
        validateDevice
    );

    return success;
}

bool validatePlanarProjection(const PlanarProjection& proj) {
    bool success = true;

    if (proj.fov.up == proj.fov.down) {
        MessageHandler::printError("Up and down field of views can not be the same");
        success = false;
    }
    if (proj.fov.left == proj.fov.right) {
        MessageHandler::printError("Left and right field of views can not be the same");
        success = false;
    }
    return success;
}

bool validateFisheyeProjection(const FisheyeProjection& proj) {
    bool success = true;

    if (proj.fov && *proj.fov <= 0.f) {
        MessageHandler::printError("Field of view setting must be positive");
        success = false;
    }

    if (proj.crop && proj.crop->left > proj.crop->right) {
        MessageHandler::printError("Left and right crop must not overlap");
        success = false;
    }

    if (proj.crop && proj.crop->bottom > proj.crop->top) {
        MessageHandler::printError("Bottom and top crop must not overlap");
        success = false;
    }

    if (proj.quality && *proj.quality <= 0) {
        MessageHandler::printError("Quality value must be positive");
        success = false;
    }
    
    if (proj.quality && glm::fract(glm::log(*proj.quality)) == 0.f) {
        MessageHandler::printError("Quality setting only allows powers of two");
        success = false;
    }

    if (proj.diameter && *proj.diameter <= 0.f) {
        MessageHandler::printError("Diameter must be positive");
        success = false;
    }

    if (proj.background && glm::any(glm::lessThan(*proj.background, glm::vec4(0.f)))) {
        MessageHandler::printError("Every background color component has to be positive");
        success = false;
    }

    return success;
}

bool validateSphericalMirrorProjection(const SphericalMirrorProjection& proj) {
    bool success = true;

    if (proj.quality && *proj.quality <= 0) {
        MessageHandler::printError("Quality value must be positive");
        success = false;
    }

    if (proj.quality && glm::fract(glm::log(*proj.quality)) == 0.f) {
        MessageHandler::printError("Quality setting only allows powers of two");
        success = false;
    }

    if (proj.background && glm::any(glm::lessThan(*proj.background, glm::vec4(0.f)))) {
        MessageHandler::printError("Every background color component has to be positive");
        success = false;
    }

    return success;
}

bool validateSpoutOutputProjection(const SpoutOutputProjection& proj) {
    bool success = true;

    if (proj.mappingSpoutName.empty()) {
        MessageHandler::printError("Mapping name must not be empty");
        success = false;
    }

    if (proj.quality && *proj.quality <= 0) {
        MessageHandler::printError("Quality value must be positive");
        success = false;
    }

    if (proj.quality && glm::fract(glm::log(*proj.quality)) == 0.f) {
        MessageHandler::printError("Quality setting only allows powers of two");
        success = false;
    }

    if (proj.background && glm::any(glm::lessThan(*proj.background, glm::vec4(0.f)))) {
        MessageHandler::printError("Every background color component has to be positive");
        success = false;
    }

    return success;
}

bool validateProjectionPlane(const ProjectionPlane&) {
    return true;
}

bool validateMpcdiProjection(const MpcdiProjection&) {
    return true;
}

bool validateViewport(const Viewport& viewport) {
    bool success = true;
    
    if (viewport.user && viewport.user->empty()) {
        MessageHandler::printError("User must not be empty");
        success = false;
    }

    if (viewport.overlayTexture && viewport.overlayTexture->empty()) {
        MessageHandler::printError("Overlay texture path must not be empty");
        success = false;
    }

    if (viewport.blendMaskTexture && viewport.blendMaskTexture->empty()) {
        MessageHandler::printError("Blendmask texture path must not be empty");
        success = false;
    }

    if (viewport.blendLevelMaskTexture && viewport.blendLevelMaskTexture->empty()) {
        MessageHandler::printError("Blendmask level texture path must not be empty");
        success = false;
    }

    if (viewport.correctionMeshTexture && viewport.correctionMeshTexture->empty()) {
        MessageHandler::printError("Correction mesh texture path must not be empty");
        success = false;
    }

    if (viewport.meshHint && viewport.meshHint->empty()) {
        MessageHandler::printError("Mesh hint must not be empty");
        success = false;
    }

    std::visit(overloaded {
        [](const NoProjection&) { return true; },
        [](const PlanarProjection& p) { return validatePlanarProjection(p); },
        [](const FisheyeProjection& p) { return validateFisheyeProjection(p); },
        [](const SphericalMirrorProjection& p) {
            return validateSphericalMirrorProjection(p);
        },
        [](const SpoutOutputProjection& p) { return validateSpoutOutputProjection(p); },
        [](const ProjectionPlane& p) { return validateProjectionPlane(p); }
    }, viewport.projection);


    return success;
}

bool validateWindow(const Window& window) {
    bool success = true;

    if (window.name && window.name->empty()) {
        MessageHandler::printError("Window name must not be empty");
        success = false;
    }

    for (const std::string& t : window.tags) {
        if (t.empty()) {
            MessageHandler::printError("Empty tags are not allowed for windows");
            success = false;
        }
    }

    if (window.gamma && *window.gamma <= 0.1f) {
        MessageHandler::printError("Gamma value must be at least 0.1");
        success = false;
    }

    if (window.contrast && *window.contrast <= 0.f) {
        MessageHandler::printError("Contrast value must be postive");
        success = false;
    }

    if (window.brightness && *window.brightness <= 0.f) {
        MessageHandler::printError("Brightness value must be positive");
        success = false;
    }

    if (window.msaa && *window.msaa < 0) {
        MessageHandler::printError("Number of MSAA samples must be non-negative");
        success = false;
    }

    if (window.monitor && *window.monitor < 0) {
        MessageHandler::printError("Monitor index must be non-negative");
        success = false;
    }

    if (window.mpcdi && window.mpcdi->empty()) {
        MessageHandler::printError("MPCDI file must not be empty");
        success = false;
    }

    success &= std::all_of(
        window.viewports.begin(),
        window.viewports.end(),
        validateViewport
    );

    return success;
}

bool validateNode(const Node& node) {
    bool success = true;

    if (node.address.empty()) {
        MessageHandler::printError("Node address must not be empty");
        success = false;
    }

    if (node.port <= 0) {
        MessageHandler::printError("Node port must be non-negative");
        success = false;
    }

    if (node.name && node.name->empty()) {
        MessageHandler::printError("Node name must not be empty");
        success = false;
    }

    if (node.dataTransferPort && *node.dataTransferPort <= 0) {
        MessageHandler::printError("Node data transfer port must be non-negative");
        success = false;
    }

    success &= std::all_of(node.windows.begin(), node.windows.end(), validateWindow);
    return success;
}

bool validateCluster(const Cluster& cluster) {
    bool success = true;

    if (cluster.masterAddress.empty()) {
        MessageHandler::printError("Cluster master address must not be empty");
        success = false;
    }

    if (cluster.externalControlPort && *cluster.externalControlPort <= 0) {
        MessageHandler::printError("Cluster external control port must be non-negative");
        success = false;
    }

    if (cluster.scene) {
        success &= validateScene(*cluster.scene);
    }
    if (cluster.user) {
        success &= validateUser(*cluster.user);
    }
    if (cluster.capture) {
        success &= validateCapture(*cluster.capture);
    }
    if (cluster.tracker) {
        success &= validateTracker(*cluster.tracker);
    }
    if (cluster.settings) {
        success &= validateSettings(*cluster.settings);
    }
    success &= std::all_of(cluster.nodes.begin(), cluster.nodes.end(), validateNode);

    return success;
}

} // namespace sgct::config
