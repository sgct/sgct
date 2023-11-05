/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/config.h>

#include <sgct/fmt.h>
#include <sgct/error.h>
#include <sgct/profiling.h>
#include <algorithm>
#include <assert.h>
#include <functional>
#include <iterator>
#include <numeric>


#define Error(code, msg) sgct::Error(sgct::Error::Component::Config, code, msg)

namespace {
    // Helper structs for the visitor pattern of the std::variant on projections
    template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
} // namespace

namespace sgct::config {

void validateUser(const User& u) {
    ZoneScoped;

    if (u.eyeSeparation && *u.eyeSeparation < 0.f) {
        throw Error(1000, "Eye separation must be zero or a positive number");
    }

    if (u.tracking && u.tracking->device.empty()) {
        throw Error(1001, "Tracking device name must not be empty");
    }
    if (u.tracking && u.tracking->tracker.empty()) {
        throw Error(1002, "Tracking tracker name must not be empty");
    }
    if (u.name && *u.name == "default") {
        throw Error(1003, "Name 'default' is not permitted for a user");
    }
}

void validateCapture(const Capture& c) {
    ZoneScoped;

    if (c.path && c.path->empty()) {
        throw Error(1010, "Capture path must not be empty");
    }

    if (c.range.has_value()) {
        if (c.range->first >= c.range->last && c.range->last != -1) {
            throw Error(1011, "Screenshot ranges beginning has to be before the end");
        }
    }
}

void validateScene(const Scene&) {}

void validateSettings(const Settings& s) {
    ZoneScoped;

    if (s.display && s.display->swapInterval && *s.display->swapInterval < 0) {
        throw Error(1020, "Swap interval must not be negative");
    }
    if (s.display && s.display->refreshRate && *s.display->refreshRate < 0) {
        throw Error(1021, "Refresh rate must not be negative");
    }
}

void validateDevice(const Device& d) {
    ZoneScoped;

    auto validateAddress = [](const auto& v) -> bool { return !v.vrpnAddress.empty(); };

    if (d.name.empty()) {
        throw Error(1030, "Device name must not be empty");
    }
    if (!std::all_of(d.sensors.begin(), d.sensors.end(), validateAddress)) {
        throw Error(1031, "VRPN address for sensors must not be empty");

    }
    if (!std::all_of(d.buttons.begin(), d.buttons.end(), validateAddress)) {
        throw Error(1032, "VRPN address for buttons must not be empty");

    }
    if (!std::all_of(d.axes.begin(), d.axes.end(), validateAddress)) {
        throw Error(1033, "VRPN address for axes must not be empty");
    }
}

void validateTracker(const Tracker& t) {
    ZoneScoped;

    if (t.name.empty()) {
        throw Error(1040, "Tracker name must not be empty");
    }
    std::for_each(t.devices.begin(), t.devices.end(), validateDevice);
}

void validatePlanarProjection(const PlanarProjection& p) {
    ZoneScoped;

    if (p.fov.up == p.fov.down) {
        throw Error(1050, "Up and down field of views can not be the same");
    }
    if (p.fov.left == p.fov.right) {
        throw Error(1051, "Left and right field of views can not be the same");
    }
}

void validateFisheyeProjection(const FisheyeProjection& p) {
    ZoneScoped;

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
    if (p.quality && ((*p.quality & (*p.quality - 1)) != 0 && *p.quality != 1536)) {
        throw Error(1064, "Quality setting only allows powers of two and 1536");
    }
    if (p.diameter && *p.diameter <= 0.f) {
        throw Error(1065, "Diameter must be positive");
    }
    if (p.background) {
        vec4 b = *p.background;
        if (b.x < 0.f || b.y < 0.f || b.z < 0.f || b.w < 0.f) {
            throw Error(1066, "All background color components have to be positive");
        }
    }
}

void validateSphericalMirrorProjection(const SphericalMirrorProjection& p) {
    ZoneScoped;

    if (p.quality && *p.quality <= 0) {
        throw Error(1070, "Quality value must be positive");
    }
    if (p.quality && ((*p.quality & (*p.quality - 1)) != 0)) {
        throw Error(1071, "Quality setting only allows powers of two");
    }
    if (p.background) {
        vec4 b = *p.background;
        if (b.x < 0.f || b.y < 0.f || b.z < 0.f || b.w < 0.f) {
            throw Error(1072, "All background color components have to be positive");
        }
    }
}

void validateSpoutOutputProjection(const SpoutOutputProjection& p) {
    ZoneScoped;

    if (p.mappingSpoutName.empty()) {
        throw Error(1080, "Spout Mapping name must not be empty");
    }
    if (p.quality && *p.quality <= 0) {
        throw Error(1081, "Quality value must be positive");
    }
    if (p.background) {
        vec4 b = *p.background;
        if (b.x < 0.f || b.y < 0.f || b.z < 0.f || b.w < 0.f) {
            throw Error(1083, "All background color components have to be positive");
        }
    }
}

void validateSpoutFlatProjection(const SpoutFlatProjection& p) {
    ZoneScoped;

    validatePlanarProjection(p.proj);
    if (p.mappingSpoutName.empty()) {
        throw Error(1084, "Spout Mapping name must not be empty");
    }
    if (p.width && *p.width <= 0) {
        throw Error(1085, "Width value must be positive");
    }
    if (p.height && *p.height <= 0) {
        throw Error(1086, "Height value must be positive");
    }
    if (p.background) {
        vec4 b = *p.background;
        if (b.x < 0.f || b.y < 0.f || b.z < 0.f || b.w < 0.f) {
            throw Error(1088, "All background color components have to be positive");
        }
    }
}

void validateCylindricalProjection(const CylindricalProjection&) {}

void validateEquirectangularProjection(const EquirectangularProjection&) {}

void validateProjectionPlane(const ProjectionPlane&) {}

void validateMpcdiProjection(const MpcdiProjection&) {}

void validateViewport(const Viewport& v, bool /*draw3D*/) {
    ZoneScoped;

    if (v.user && v.user->empty()) {
        throw Error(1090, "User must not be empty");
    }
    if (v.overlayTexture && v.overlayTexture->empty()) {
        throw Error(1091, "Overlay texture path must not be empty");
    }
    if (v.blendMaskTexture && v.blendMaskTexture->empty()) {
        throw Error(1092, "Blendmask texture path must not be empty");
    }
    if (v.blackLevelMaskTexture && v.blackLevelMaskTexture->empty()) {
        throw Error(1093, "Blendmask level texture path must not be empty");
    }
    if (v.correctionMeshTexture && v.correctionMeshTexture->empty()) {
        throw Error(1094, "Correction mesh texture path must not be empty");
    }

    std::visit(overloaded {
        [](const PlanarProjection& p) { validatePlanarProjection(p); },
        [](const FisheyeProjection& p) { validateFisheyeProjection(p); },
        [](const SphericalMirrorProjection& p) { validateSphericalMirrorProjection(p); },
        [](const SpoutOutputProjection& p) { validateSpoutOutputProjection(p); },
        [](const SpoutFlatProjection& p) { validateSpoutFlatProjection(p); },
        [](const CylindricalProjection& p) { validateCylindricalProjection(p); },
        [](const EquirectangularProjection& p) {
            validateEquirectangularProjection(p);
        },
        [](const ProjectionPlane& p) { validateProjectionPlane(p); },

        [v/*, draw3D*/](const NoProjection&) {
            // This is currently commented out due to the fact that some of the meshes
            // that we support can provide FOV information, too. Particularly the SCISS
            // and the scalable meshes set a PlanarProjection as well and it would be
            // weird to specify both at the moment. I think they should probably be
            // handled differently to not hide the FOV setting in the `mesh` variable of
            // the viewport

            //if (draw3D) {
            //    // We only need a projection if we actually want to render a 3D scene
            //    throw Error(1095, "No valid projection provided");
            //}
        }
        },
        v.projection
    );
}

void validateWindow(const Window& w) {
    ZoneScoped;

    if (w.name && w.name->empty()) {
        throw Error(1100, "Window name must not be empty");
    }
    if (std::any_of(w.tags.begin(), w.tags.end(), std::mem_fn(&std::string::empty))) {
        throw Error(1101, "Empty tags are not allowed for windows");
    }
    if (w.msaa && *w.msaa < 0) {
        throw Error(1102, "Number of MSAA samples must be non-negative");
    }
    if (w.monitor && *w.monitor < -1) {
        throw Error(1103, "Monitor index must be non-negative or -1");
    }
    if (w.mpcdi && w.mpcdi->empty()) {
        throw Error(1104, "MPCDI file must not be empty");
    }
    if (!w.mpcdi && w.viewports.empty()) {
        // A viewport must exist except when we load an MPCDI file
        throw Error(1105, "Window must contain at least one viewport");
    }
    if (w.mpcdi && !w.viewports.empty()) {
        throw Error(
            1106,
            "Cannot use an MPCDI file and explicitly add viewports simultaneously"
        );
    }

    for (const Viewport& vp : w.viewports) {
        const bool draw3D = w.draw3D.value_or(true);
        validateViewport(vp, draw3D);
    }
}

void validateNode(const Node& n) {
    ZoneScoped;

    if (n.address.empty()) {
        throw Error(1110, "Node address must not be empty");
    }
    if (n.port <= 0) {
        throw Error(1111, "Node port must be non-negative");
    }
    if (n.dataTransferPort && *n.dataTransferPort <= 0) {
        throw Error(1112, "Node data transfer port must be non-negative");
    }
    if (n.windows.empty()) {
        throw Error(1113, "Every node must contain at least one window");
    }
    std::vector<int> usedIds;
    for (size_t i = 0; i < n.windows.size(); ++i) {
        const Window& win = n.windows[i];
        validateWindow(win);

        if (win.id < 0) {
            throw Error(1107, "Window id must be non-negative and unique");
        }

        if (std::find(usedIds.begin(), usedIds.end(), win.id) != usedIds.end()) {
            throw Error(
                1107,
                fmt::format(
                    "Window id must be non-negative and unique. {} used multiple times",
                    win.id
                )
            );
        }
        usedIds.push_back(win.id);

        if (win.blitWindowId.has_value()) {
            auto it = std::find_if(
                n.windows.cbegin(), n.windows.cend(),
                [id = *win.blitWindowId](const Window& w) { return w.id == id; }
            );
            if (it == n.windows.cend()) {
                throw Error(
                    1108,
                    fmt::format(
                        "Tried to configure window {} to be blitted from window {}, but "
                        "no such window was specified", win.id, *win.blitWindowId
                    )
                );
            }
            if (win.id == *win.blitWindowId) {
                throw Error(
                    1109,
                    fmt::format(
                        "Window {} tried to blit from itself, which cannot work", win.id
                    )
                );
            }
        }
    }
}

void validateCluster(const Cluster& c) {
    ZoneScoped;

    if (c.masterAddress.empty()) {
        throw Error(1120, "Cluster master address must not be empty");
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

    if (c.users.empty()) {
        throw Error(1122, "There must be at least one user in the cluster");
    }

    const int nDefaultUsers = static_cast<int>(std::count_if(
        c.users.cbegin(), c.users.cend(),
        [](const User& user) { return !user.name.has_value(); }
    ));

    if (nDefaultUsers > 1) {
        throw Error(1123, "More than one unnamed users specified");
    }

    std::for_each(c.users.cbegin(), c.users.cend(), validateUser);
    // Check for mutually exclusive user names
    std::vector<std::string> usernames;
    std::transform(
        c.users.cbegin(),
        c.users.cend(),
        std::back_inserter(usernames),
        [](const User& user) { return user.name ? *user.name : ""; }
    );
    if (std::unique(usernames.begin(), usernames.end()) != usernames.end()) {
        throw Error(1124, "No two users can have the same name");
    }


    std::for_each(c.trackers.cbegin(), c.trackers.cend(), validateTracker);

    // Check that all trackers specified in the users are valid tracker names
    const bool foundAllTrackers = std::all_of(
        c.users.cbegin(),
        c.users.cend(),
        [ts = c.trackers](const User& user) {
            if (!user.tracking) {
                return true;
            }
            return std::find_if(
                ts.cbegin(),
                ts.cend(),
                [t = *user.tracking](const Tracker& tr) { return tr.name == t.tracker; }
            ) != ts.cend();
        }
    );
    if (!foundAllTrackers) {
        throw Error(
            1125,
            "All trackers specified in the User's have to be valid tracker names"
        );
    }

    // Check that all devices specified in the users are valid device names
    const bool allDevicesValid = std::all_of(
        c.users.cbegin(),
        c.users.cend(),
        [ts = c.trackers](const User& user) {
            if (!user.tracking) {
                // If we don't have tracking configured, this user is perfectly valid
                return true;
            }
            // Lets find the tracker that this user is linked to. We know that it has to
            // exist as we checked the correct mapping between user and Tracker in the
            // previous step in this validation. A little assert doesn't hurt though
            const auto it = std::find_if(
                ts.cbegin(),
                ts.cend(),
                [t = *user.tracking](const Tracker& tr) { return tr.name == t.tracker; }
            );
            assert(it != ts.cend());

            const Tracker& tr = *it;
            // See if the device that was specified is acually part of the tracker that
            // was specified for the user
            return std::find_if(
                tr.devices.cbegin(),
                tr.devices.cend(),
                [t = *user.tracking](const Device& dev) { return dev.name == t.device; }
            ) != tr.devices.cend();
        }
    );
    if (!allDevicesValid) {
        throw Error(1126, "All devices in the 'User's have to be valid devices");
    }

    if (c.nodes.empty()) {
        throw Error(1127, "Configuration must contain at least one node");
    }
    std::for_each(c.nodes.cbegin(), c.nodes.cend(), validateNode);


    // Check for mutually exclusive ports
    std::vector<int> ports;
    std::transform(
        c.nodes.cbegin(),
        c.nodes.cend(),
        std::back_inserter(ports),
        [](const Node& node) { return node.port; }
    );
    if (std::unique(ports.begin(), ports.end()) != ports.end()) {
        throw Error(1128, "Two or more nodes are using the same port");
    }
}

void validateGeneratorVersion(const GeneratorVersion&) {}

} // namespace sgct::config
