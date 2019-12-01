/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/config.h>

#include <sgct/error.h>
#include <algorithm>
#include <numeric>

#define Error(code, msg) sgct::Error(sgct::Error::Component::Config, code, msg)

namespace {
    // Helper structs for the visitor pattern of the std::variant on projections
    template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
} // namespace

namespace sgct::config {

void validateUser(const User& u) {
    if (u.tracking && u.tracking->device.empty()) {
        throw Error(1000, "Tracking device name must not be empty");
    }
    if (u.tracking && u.tracking->tracker.empty()) {
        throw Error(1001, "Tracking tracker name must not be empty");
    }
}

void validateCapture(const Capture& c) {
    if (c.monoPath && c.monoPath->empty()) {
        throw Error(1010, "Mono path must not be empty");
    }
    if (c.monoPath && c.leftPath->empty()) {
        throw Error(1011, "Left path must not be empty");
    }
    if (c.monoPath && c.rightPath->empty()) {
        throw Error(1012, "Right path must not be empty");
    }
}

void validateScene(const Scene&) {}

void validateSettings(const Settings& s) {
    if (s.display && s.display->swapInterval &&
        *s.display->swapInterval < 0)
    {
        throw Error(1020, "Swap interval must not be negative");
    }
    if (s.display && s.display->refreshRate && *s.display->refreshRate < 0) {
        throw Error(1021, "Refresh rate must not be negative");
    }
}

void validateDevice(const Device& d) {
    if (d.name.empty()) {
        throw Error(1030, "Device name must not be empty");
    }

    auto validateAddress = [](const auto& v) -> bool { return !v.vrpnAddress.empty();};

    bool a = std::all_of(d.sensors.begin(), d.sensors.end(), validateAddress);
    if (!a) {
        throw Error(1031, "VRPN address for sensors must not be empty");

    }
    bool b = std::all_of(d.buttons.begin(), d.buttons.end(), validateAddress);
    if (!b) {
        throw Error(1032, "VRPN address for buttons must not be empty");

    }
    bool c = std::all_of(d.axes.begin(), d.axes.end(), validateAddress);
    if (!c) {
        throw Error(1033, "VRPN address for axes must not be empty");
    }
}

void validateTracker(const Tracker& t) {
    if (t.name.empty()) {
        throw Error(1040, "Tracker name must not be empty");
    }

    std::for_each(t.devices.begin(), t.devices.end(), validateDevice);
}

void validatePlanarProjection(const PlanarProjection& p) {
    if (p.fov.up == p.fov.down) {
        throw Error(1050, "Up and down field of views can not be the same");
    }
    if (p.fov.left == p.fov.right) {
        throw Error(1051, "Left and right field of views can not be the same");
    }
}

void validateFisheyeProjection(const FisheyeProjection& p) {
    if (p.fov && *p.fov <= 0.f) {
        throw Error(1060, "Field of view setting must be positive");
    }

    if (p.crop && p.crop->left > p.crop->right) {
        throw Error(1061, "Left and right crop must not overlap");
    }

    if (p.crop && p.crop->bottom > p.crop->top) {
        throw Error(1062, "Bottom and top crop must not overlap");
    }

    if (p.quality && *p.quality <= 0) {
        throw Error(1063, "Quality value must be positive");
    }
    
    if (p.quality && glm::fract(glm::log(*p.quality)) == 0.f) {
        throw Error(1064, "Quality setting only allows powers of two");
    }

    if (p.diameter && *p.diameter <= 0.f) {
        throw Error(1065, "Diameter must be positive");
    }

    if (p.background && glm::any(glm::lessThan(*p.background, glm::vec4(0.f)))) {
        throw Error(1066, "Every background color component has to be positive");
    }
}

void validateSphericalMirrorProjection(const SphericalMirrorProjection& p) {
    if (p.quality && *p.quality <= 0) {
        throw Error(1070, "Quality value must be positive");
    }

    if (p.quality && glm::fract(glm::log(*p.quality)) == 0.f) {
        throw Error(1071, "Quality setting only allows powers of two");
    }

    if (p.background && glm::any(glm::lessThan(*p.background, glm::vec4(0.f)))) {
        throw Error(1072, "Every background color component has to be positive");
    }
}

void validateSpoutOutputProjection(const SpoutOutputProjection& p) {
    if (p.mappingSpoutName.empty()) {
        throw Error(1080, "Mapping name must not be empty");
    }

    if (p.quality && *p.quality <= 0) {
        throw Error(1081, "Quality value must be positive");
    }

    if (p.quality && glm::fract(glm::log(*p.quality)) == 0.f) {
        throw Error(1082, "Quality setting only allows powers of two");
    }

    if (p.background && glm::any(glm::lessThan(*p.background, glm::vec4(0.f)))) {
        throw Error(1083, "Every background color component has to be positive");
    }
}

void validateProjectionPlane(const ProjectionPlane&) {}

void validateMpcdiProjection(const MpcdiProjection&) {}

void validateViewport(const Viewport& v) {
    if (v.user && v.user->empty()) {
        throw Error(1090, "User must not be empty");
    }

    if (v.overlayTexture && v.overlayTexture->empty()) {
        throw Error(1091, "Overlay texture path must not be empty");
    }

    if (v.blendMaskTexture && v.blendMaskTexture->empty()) {
        throw Error(1092, "Blendmask texture path must not be empty");
    }

    if (v.blendLevelMaskTexture && v.blendLevelMaskTexture->empty()) {
        throw Error(1093, "Blendmask level texture path must not be empty");
    }

    if (v.correctionMeshTexture && v.correctionMeshTexture->empty()) {
        throw Error(1094, "Correction mesh texture path must not be empty");
    }

    if (v.meshHint && v.meshHint->empty()) {
        throw Error(1095, "Mesh hint must not be empty");
    }

    std::visit(overloaded {
        [](const NoProjection&) {},
        [](const PlanarProjection& p) { validatePlanarProjection(p); },
        [](const FisheyeProjection& p) { validateFisheyeProjection(p); },
        [](const SphericalMirrorProjection& p) { validateSphericalMirrorProjection(p); },
        [](const SpoutOutputProjection& p) { validateSpoutOutputProjection(p); },
        [](const ProjectionPlane& p) { validateProjectionPlane(p); }
    }, v.projection);
}

void validateWindow(const Window& w) {
    if (w.name && w.name->empty()) {
        throw Error(1100, "Window name must not be empty");
    }

    for (const std::string& t : w.tags) {
        if (t.empty()) {
            throw Error(1101, "Empty tags are not allowed for windows");
        }
    }

    if (w.msaa && *w.msaa < 0) {
        throw Error(1102, "Number of MSAA samples must be non-negative");
    }

    if (w.monitor && *w.monitor < 0) {
        throw Error(1103, "Monitor index must be non-negative");
    }

    if (w.mpcdi && w.mpcdi->empty()) {
        throw Error(1104, "MPCDI file must not be empty");
    }

    std::for_each(w.viewports.begin(), w.viewports.end(), validateViewport);
}

void validateNode(const Node& n) {
    if (n.address.empty()) {
        throw Error(1110, "Node address must not be empty");
    }

    if (n.port <= 0) {
        throw Error(1111, "Node port must be non-negative");
    }

    if (n.dataTransferPort && *n.dataTransferPort <= 0) {
        throw Error(1112, "Node data transfer port must be non-negative");
    }

    std::for_each(n.windows.begin(), n.windows.end(), validateWindow);
}

void validateCluster(const Cluster& c) {
    if (c.masterAddress.empty()) {
        throw Error(1120, "Cluster master address must not be empty");
    }

    if (c.externalControlPort && *c.externalControlPort <= 0) {
        throw Error(1121, "Cluster external control port must be non-negative");
    }
    if (c.scene) {
        validateScene(*c.scene);
    }
    if (c.capture) {
        validateCapture(*c.capture);
    }
    if (c.settings) {
        validateSettings(*c.settings);
    }

    const int nDefaultUsers = std::accumulate(
        c.users.begin(),
        c.users.end(),
        0,
        [](int i, const User& user) { return user.name.has_value() ? i : i + 1; }
    );
    if (nDefaultUsers > 1) {
        throw Error(1122, "More than one unnamed users specified");
    }
    
    std::for_each(c.users.begin(), c.users.end(), validateUser);
    std::for_each(c.trackers.begin(), c.trackers.end(), validateTracker);
    std::for_each(c.nodes.begin(), c.nodes.end(), validateNode);
}

} // namespace sgct::config
