/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/config.h>

#include <sgct/error.h>
#include <sgct/messagehandler.h>
#include <glm/vector_relational.hpp>
#include <algorithm>
#include <variant>

#define Error(code, msg) sgct::Error(sgct::Error::Component::Config, code, msg)

namespace {
    // Helper structs for the visitor pattern of the std::variant on projections
    template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

} // namespace

namespace sgct::config {

void validateUser(const User& user) {
    if (user.tracking && user.tracking->device.empty()) {
        throw Error(1000, "Tracking device name must not be empty");
    }
    if (user.tracking && user.tracking->tracker.empty()) {
        throw Error(1001, "Tracking tracker name must not be empty");
    }
}

void validateCapture(const Capture& capture) {
    if (capture.monoPath && capture.monoPath->empty()) {
        throw Error(1010, "Mono path must not be empty");
    }
    if (capture.monoPath && capture.leftPath->empty()) {
        throw Error(1011, "Left path must not be empty");
    }
    if (capture.monoPath && capture.rightPath->empty()) {
        throw Error(1012, "Right path must not be empty");
    }
}

void validateScene(const Scene&) {
}

void validateSettings(const Settings& settings) {
    if (settings.display && settings.display->swapInterval &&
        *settings.display->swapInterval < 0)
    {
        throw Error(1020, "Swap interval must not be negative");
    }
    if (settings.display && settings.display->refreshRate &&
        *settings.display->refreshRate < 0)
    {
        throw Error(1021, "Refresh rate must not be negative");
    }

    if (settings.osdText && settings.osdText->name && settings.osdText->name->empty()) {
        throw Error(1022, "OSDText font name must not be negative");
    }

    if (settings.osdText && settings.osdText->path && settings.osdText->path->empty()) {
        throw Error(1023, "OSDText font path must not be empty");
    }

    if (settings.osdText && settings.osdText->size && *settings.osdText->size < 0) {
        throw Error(1024, "OSDText font size must not be negative");
    }

    if (settings.fxaa && settings.fxaa->trim && *settings.fxaa->trim <= 0.f) {
        throw Error(1025, "FXAA trim must be postive");
    }
}

void validateDevice(const Device& device) {
    if (device.name.empty()) {
        throw Error(1030, "Device name must not be empty");
    }

    auto validateAddress = [](const auto& v) -> bool { return !v.vrpnAddress.empty();};

    bool a = std::all_of(device.sensors.begin(), device.sensors.end(), validateAddress);
    if (!a) {
        throw Error(1031, "VRPN address for sensors must not be empty");

    }
    bool b = std::all_of(device.buttons.begin(), device.buttons.end(), validateAddress);
    if (!b) {
        throw Error(1032, "VRPN address for buttons must not be empty");

    }
    bool c = std::all_of(device.axes.begin(), device.axes.end(), validateAddress);
    if (!c) {
        throw Error(1033, "VRPN address for axes must not be empty");
    }
}

void validateTracker(const Tracker& tracker) {
    if (tracker.name.empty()) {
        throw Error(1040, "Tracker name must not be empty");
    }

    std::for_each(tracker.devices.begin(), tracker.devices.end(), validateDevice);
}

void validatePlanarProjection(const PlanarProjection& proj) {
    if (proj.fov.up == proj.fov.down) {
        throw Error(1050, "Up and down field of views can not be the same");
    }
    if (proj.fov.left == proj.fov.right) {
        throw Error(1051, "Left and right field of views can not be the same");
    }
}

void validateFisheyeProjection(const FisheyeProjection& proj) {
    if (proj.fov && *proj.fov <= 0.f) {
        throw Error(1060, "Field of view setting must be positive");
    }

    if (proj.crop && proj.crop->left > proj.crop->right) {
        throw Error(1061, "Left and right crop must not overlap");
    }

    if (proj.crop && proj.crop->bottom > proj.crop->top) {
        throw Error(1062, "Bottom and top crop must not overlap");
    }

    if (proj.quality && *proj.quality <= 0) {
        throw Error(1063, "Quality value must be positive");
    }
    
    if (proj.quality && glm::fract(glm::log(*proj.quality)) == 0.f) {
        throw Error(1064, "Quality setting only allows powers of two");
    }

    if (proj.diameter && *proj.diameter <= 0.f) {
        throw Error(1065, "Diameter must be positive");
    }

    if (proj.background && glm::any(glm::lessThan(*proj.background, glm::vec4(0.f)))) {
        throw Error(1066, "Every background color component has to be positive");
    }
}

void validateSphericalMirrorProjection(const SphericalMirrorProjection& proj) {
    if (proj.quality && *proj.quality <= 0) {
        throw Error(1070, "Quality value must be positive");
    }

    if (proj.quality && glm::fract(glm::log(*proj.quality)) == 0.f) {
        throw Error(1071, "Quality setting only allows powers of two");
    }

    if (proj.background && glm::any(glm::lessThan(*proj.background, glm::vec4(0.f)))) {
        throw Error(1072, "Every background color component has to be positive");
    }
}

void validateSpoutOutputProjection(const SpoutOutputProjection& proj) {
    if (proj.mappingSpoutName.empty()) {
        throw Error(1080, "Mapping name must not be empty");
    }

    if (proj.quality && *proj.quality <= 0) {
        throw Error(1081, "Quality value must be positive");
    }

    if (proj.quality && glm::fract(glm::log(*proj.quality)) == 0.f) {
        throw Error(1082, "Quality setting only allows powers of two");
    }

    if (proj.background && glm::any(glm::lessThan(*proj.background, glm::vec4(0.f)))) {
        throw Error(1083, "Every background color component has to be positive");
    }
}

void validateProjectionPlane(const ProjectionPlane&) {}

void validateMpcdiProjection(const MpcdiProjection&) {}

void validateViewport(const Viewport& viewport) {
    if (viewport.user && viewport.user->empty()) {
        throw Error(1090, "User must not be empty");
    }

    if (viewport.overlayTexture && viewport.overlayTexture->empty()) {
        throw Error(1091, "Overlay texture path must not be empty");
    }

    if (viewport.blendMaskTexture && viewport.blendMaskTexture->empty()) {
        throw Error(1092, "Blendmask texture path must not be empty");
    }

    if (viewport.blendLevelMaskTexture && viewport.blendLevelMaskTexture->empty()) {
        throw Error(1093, "Blendmask level texture path must not be empty");
    }

    if (viewport.correctionMeshTexture && viewport.correctionMeshTexture->empty()) {
        throw Error(1094, "Correction mesh texture path must not be empty");
    }

    if (viewport.meshHint && viewport.meshHint->empty()) {
        throw Error(1095, "Mesh hint must not be empty");
    }

    std::visit(overloaded {
        [](const NoProjection&) {},
        [](const PlanarProjection& p) { validatePlanarProjection(p); },
        [](const FisheyeProjection& p) { validateFisheyeProjection(p); },
        [](const SphericalMirrorProjection& p) { validateSphericalMirrorProjection(p); },
        [](const SpoutOutputProjection& p) { validateSpoutOutputProjection(p); },
        [](const ProjectionPlane& p) { validateProjectionPlane(p); }
    }, viewport.projection);
}

void validateWindow(const Window& window) {
    if (window.name && window.name->empty()) {
        throw Error(1100, "Window name must not be empty");
    }

    for (const std::string& t : window.tags) {
        if (t.empty()) {
            throw Error(1101, "Empty tags are not allowed for windows");
        }
    }

    if (window.gamma && *window.gamma <= 0.1f) {
        throw Error(1102, "Gamma value must be at least 0.1");
    }

    if (window.contrast && *window.contrast <= 0.f) {
        throw Error(1103, "Contrast value must be postive");
    }

    if (window.brightness && *window.brightness <= 0.f) {
        throw Error(1104, "Brightness value must be positive");
    }

    if (window.msaa && *window.msaa < 0) {
        throw Error(1105, "Number of MSAA samples must be non-negative");
    }

    if (window.monitor && *window.monitor < 0) {
        throw Error(1106, "Monitor index must be non-negative");
    }

    if (window.mpcdi && window.mpcdi->empty()) {
        throw Error(1107, "MPCDI file must not be empty");
    }

    std::for_each(window.viewports.begin(), window.viewports.end(), validateViewport);
}

void validateNode(const Node& node) {
    if (node.address.empty()) {
        throw Error(1110, "Node address must not be empty");
    }

    if (node.port <= 0) {
        throw Error(1111, "Node port must be non-negative");
    }

    if (node.name && node.name->empty()) {
        throw Error(1112, "Node name must not be empty");
    }

    if (node.dataTransferPort && *node.dataTransferPort <= 0) {
        throw Error(1113, "Node data transfer port must be non-negative");
    }

    std::for_each(node.windows.begin(), node.windows.end(), validateWindow);
}

void validateCluster(const Cluster& cluster) {
    if (cluster.masterAddress.empty()) {
        throw Error(1120, "Cluster master address must not be empty");
    }

    if (cluster.externalControlPort && *cluster.externalControlPort <= 0) {
        throw Error(1121, "Cluster external control port must be non-negative");
    }

    if (cluster.scene) {
        validateScene(*cluster.scene);
    }
    if (cluster.user) {
        validateUser(*cluster.user);
    }
    if (cluster.capture) {
        validateCapture(*cluster.capture);
    }
    if (cluster.tracker) {
        validateTracker(*cluster.tracker);
    }
    if (cluster.settings) {
        validateSettings(*cluster.settings);
    }
    std::for_each(cluster.nodes.begin(), cluster.nodes.end(), validateNode);
}

} // namespace sgct::config
