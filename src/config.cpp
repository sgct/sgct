/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/config.h>

#include <sgct/error.h>
#include <sgct/format.h>
#include <sgct/log.h>
#include <sgct/profiling.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#define JSON_HAS_CPP_11
#define JSON_HAS_CPP_14
#define JSON_HAS_CPP_17
#define JSON_HAS_CPP_20
#define JSON_USE_IMPLICIT_CONVERSIONS 0
#include <nlohmann/json.hpp>

#include <nlohmann/json-schema.hpp>
#include <algorithm>
#include <cassert>
#include <fstream>
#include <functional>
#include <iterator>
#include <numeric>

#define Error(code, msg) sgct::Error(sgct::Error::Component::Config, code, msg)

namespace {
    // Helper structs for the visitor pattern of the std::variant on projections
    template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

    // Taken from Ghoul::stringhelper
    std::vector<std::string> tokenizeString(const std::string& input, char separator) {
        size_t separatorPos = input.find(separator);
        if (separatorPos == std::string::npos) {
            return { input };
        }
        else {
            std::vector<std::string> res;
            size_t prevSeparator = 0;
            while (separatorPos != std::string::npos) {
                res.push_back(input.substr(prevSeparator, separatorPos - prevSeparator));
                prevSeparator = separatorPos + 1;
                separatorPos = input.find(separator, separatorPos + 1);
            }
            res.push_back(input.substr(prevSeparator));
            return res;
        }
    }
} // namespace

namespace sgct::config {

void simplifyIpAddress(std::string& address) {
    // First convert the address into lowercase characters
    std::transform(
        address.cbegin(),
        address.cend(),
        address.begin(),
        [](char c) { return static_cast<char>(::tolower(c)); }
    );

    // Then we should check if we have a true IP address that has extra leading 0s, for
    // example: 192.168.001.050 which corresponds to 192.168.1.50

    std::vector<std::string> tokens = tokenizeString(address, '.');
    if (tokens.size() != 4) {
        // In this case we are not dealing with a IPv4 address
        return;
    }

    // Delete 0s from the beginning of each token until there are none left. The check
    // against an empty string is just a measure to prevent a crash from an invalid
    // address and should never happen
    for (std::string& token : tokens) {
        while (!token.empty() && token.front() == '0') {
            token.erase(token.begin());
        }

        // We might be overzealous and remove all of the characters if an IP address is
        // just a 0 (for example in 127.0.0.1). So in case a portion ends up empty, we
        // know that it was just a 0, 00, or 000 to be begin with
        if (token.empty()) {
            token = "0";
        }
    }

    // Reassemble the IP address
    address = std::format("{}.{}.{}.{}", tokens[0], tokens[1], tokens[2], tokens[3]);
}

void validateUser(const User& u) {
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
    if (c.path && c.path->empty()) {
        throw Error(1010, "Capture path must not be empty");
    }

    if (c.range) {
        if (c.range->first >= c.range->last && c.range->last != -1) {
            throw Error(1011, "Screenshot ranges beginning has to be before the end");
        }
    }
}

void validateScene(const Scene&) {}

void validateSettings(const Settings& s) {
    if (s.display && s.display->swapInterval && *s.display->swapInterval < 0) {
        throw Error(1020, "Swap interval must not be negative");
    }
    if (s.display && s.display->refreshRate && *s.display->refreshRate < 0) {
        throw Error(1021, "Refresh rate must not be negative");
    }
}

void validateTracker(const Tracker& t) {
    if (t.name.empty()) {
        throw Error(1040, "Tracker name must not be empty");
    }

    auto validateDevice = [](const Tracker::Device& d) {
        auto validateAddress = [](const auto& v) { return !v.vrpnAddress.empty(); };

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
    };
    std::for_each(t.devices.begin(), t.devices.end(), validateDevice);
}

void validateProjection(const PlanarProjection& p) {
    if (p.fov.up == p.fov.down) {
        throw Error(1050, "Up and down field of views can not be the same");
    }
    if (p.fov.left == p.fov.right) {
        throw Error(1051, "Left and right field of views can not be the same");
    }
}

void validateProjection(const TextureMappedProjection& p) {
    validateProjection(static_cast<const PlanarProjection&>(p));
}

void validateProjection(const FisheyeProjection& p) {
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
        const vec4 b = *p.background;
        if (b.x < 0.f || b.y < 0.f || b.z < 0.f || b.w < 0.f) {
            throw Error(1066, "All background color components have to be positive");
        }
    }
}

void validateProjection(const SphericalMirrorProjection& p) {
    if (p.quality && *p.quality <= 0) {
        throw Error(1070, "Quality value must be positive");
    }
    if (p.quality && ((*p.quality & (*p.quality - 1)) != 0)) {
        throw Error(1071, "Quality setting only allows powers of two");
    }
    if (p.background) {
        const vec4 b = *p.background;
        if (b.x < 0.f || b.y < 0.f || b.z < 0.f || b.w < 0.f) {
            throw Error(1072, "All background color components have to be positive");
        }
    }
}

void validateProjection(const CubemapProjection& p) {
    if (p.quality && *p.quality <= 0) {
        throw Error(1080, "Quality value must be positive");
    }
}

void validateProjection(const CylindricalProjection&) {}

void validateProjection(const EquirectangularProjection&) {}

void validateProjection(const ProjectionPlane&) {}

void validateProjection(const NoProjection&) {}

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
    if (v.blackLevelMaskTexture && v.blackLevelMaskTexture->empty()) {
        throw Error(1093, "Blendmask level texture path must not be empty");
    }
    if (v.correctionMeshTexture && v.correctionMeshTexture->empty()) {
        throw Error(1094, "Correction mesh texture path must not be empty");
    }

    std::visit([](const auto& p) { validateProjection(p); }, v.projection);
}

void validateWindow(const Window& w) {
    if (w.name && w.name->empty()) {
        throw Error(1100, "Window name must not be empty");
    }
    if (std::any_of(w.tags.begin(), w.tags.end(), std::mem_fn(&std::string::empty))) {
        throw Error(1101, "Empty tags are not allowed for windows");
    }

#ifndef SGCT_HAS_SCALABLE
    if (w.scalable.has_value()) {
        throw Error(1004, "Tried to load ScalableMesh without SGCT support");
    }
#endif // SGCT_HAS_SCALABLE

    for (const Viewport& vp : w.viewports) {
        validateViewport(vp);
    }
}

void validateNode(const Node& n) {
    if (n.address.empty()) {
        throw Error(1110, "Node address must not be empty");
    }
    if (n.port <= 0) {
        throw Error(1111, "Node port must be a positive number");
    }
    if (n.dataTransferPort && *n.dataTransferPort <= 0) {
        throw Error(1112, "Node data transfer port must be a positive number");
    }
    if (n.windows.empty()) {
        throw Error(1113, "Every node must contain at least one window");
    }
    std::vector<int> usedIds;
    for (const Window& win : n.windows) {
        validateWindow(win);

        if (win.id < 0) {
            throw Error(1107, "Window id must be non-negative and unique");
        }

        if (std::find(usedIds.begin(), usedIds.end(), win.id) != usedIds.end()) {
            throw Error(
                1107,
                std::format(
                    "Window id must be non-negative and unique. {} used multiple times",
                    win.id
                )
            );
        }
        usedIds.push_back(win.id);

        if (win.blitWindowId) {
            if (*win.blitWindowId < 0) {
                throw Error(1108, "BlitWindowId must be between 0 and 127");
            }
            auto it = std::find_if(
                n.windows.cbegin(), n.windows.cend(),
                [id = *win.blitWindowId](const Window& w) { return w.id == id; }
            );
            if (it == n.windows.cend()) {
                throw Error(
                    1109,
                    std::format(
                        "Tried to configure window {} to be blitted from window {}, but "
                        "no such window was specified", win.id, *win.blitWindowId
                    )
                );
            }
            if (win.id == *win.blitWindowId) {
                throw Error(
                    1110,
                    std::format(
                        "Window {} tried to blit from itself, which cannot work", win.id
                    )
                );
            }
        }
    }
}

void validateCluster(const Cluster& c) {
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

    const int nDefaultUsers = static_cast<int>(
        std::count_if(
            c.users.cbegin(), c.users.cend(),
            [](const User& user) { return !user.name.has_value(); }
        )
    );

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
        [](const User& user) { return user.name.value_or(""); }
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
                [&user](const Tracker::Device& dev) {
                    return dev.name == user.tracking->device;
                }
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
}

void validateGeneratorVersion(const GeneratorVersion&) {}

bool GeneratorVersion::versionCheck(GeneratorVersion check) const {
    if (check.name != name) {
        return false;
    }
    else if (major > check.major) {
        return true;
    }
    else {
        return ((major == check.major) && (minor >= check.minor));
    }
}

std::string GeneratorVersion::versionString() const {
    return std::format("{} {}.{}", name, major, minor);
}

} // namespace sgct::config

#undef Error
#define Err(code, msg) sgct::Error(sgct::Error::Component::ReadConfig, code, msg)

namespace {
    sgct::config::Window::StereoMode parseStereoType(std::string_view t) {
        using M = sgct::config::Window::StereoMode;
        if (t == "none" || t == "no_stereo") { return M::NoStereo; }
        if (t == "active" || t == "quadbuffer") { return M::Active; }
        if (t == "checkerboard") { return M::Checkerboard; }
        if (t == "checkerboard_inverted") { return M::CheckerboardInverted; }
        if (t == "anaglyph_red_cyan") { return M::AnaglyphRedCyan; }
        if (t == "anaglyph_amber_blue") { return M::AnaglyphAmberBlue; }
        if (t == "anaglyph_wimmer") { return M::AnaglyphRedCyanWimmer; }
        if (t == "vertical_interlaced") { return M::VerticalInterlaced; }
        if (t == "vertical_interlaced_inverted") { return M::VerticalInterlacedInverted; }
        if (t == "test" || t == "dummy") { return M::Dummy; }
        if (t == "side_by_side") { return M::SideBySide; }
        if (t == "side_by_side_inverted") { return M::SideBySideInverted; }
        if (t == "top_bottom") { return M::TopBottom; }
        if (t == "top_bottom_inverted") { return M::TopBottomInverted; }

        throw Err(6085, std::format("Unknown stereo mode {}", t));
    }

    std::string_view toString(sgct::config::Window::StereoMode mode) {
        switch (mode) {
            case sgct::config::Window::StereoMode::NoStereo:
                return "none";
            case sgct::config::Window::StereoMode::Active:
                return "active";
            case sgct::config::Window::StereoMode::Checkerboard:
                return "checkerboard";
            case sgct::config::Window::StereoMode::CheckerboardInverted:
                return "checkerboard_inverted";
            case sgct::config::Window::StereoMode::AnaglyphRedCyan:
                return "anaglyph_red_cyan";
            case sgct::config::Window::StereoMode::AnaglyphAmberBlue:
                return "anaglyph_amber_blue";
            case sgct::config::Window::StereoMode::AnaglyphRedCyanWimmer:
                return "anaglyph_wimmer";
            case sgct::config::Window::StereoMode::VerticalInterlaced:
                return "vertical_interlaced";
            case sgct::config::Window::StereoMode::VerticalInterlacedInverted:
                return "vertical_interlaced_inverted";
            case sgct::config::Window::StereoMode::Dummy:
                return "dummy";
            case sgct::config::Window::StereoMode::SideBySide:
                return "side_by_side";
            case sgct::config::Window::StereoMode::SideBySideInverted:
                return "side_by_side_inverted";
            case sgct::config::Window::StereoMode::TopBottom:
                return "top_bottom";
            case sgct::config::Window::StereoMode::TopBottomInverted:
                return "top_bottom_inverted";
            default:
                throw std::logic_error("Missing case exception");
        }
    }

    sgct::config::Window::ColorBitDepth parseBufferColorBitDepth(std::string_view type) {
        if (type == "8") { return sgct::config::Window::ColorBitDepth::Depth8; }
        if (type == "16") { return sgct::config::Window::ColorBitDepth::Depth16; }
        if (type == "16f") { return sgct::config::Window::ColorBitDepth::Depth16Float; }
        if (type == "32f") { return sgct::config::Window::ColorBitDepth::Depth32Float; }
        if (type == "16i") { return sgct::config::Window::ColorBitDepth::Depth16Int; }
        if (type == "32i") { return sgct::config::Window::ColorBitDepth::Depth32Int; }
        if (type == "16ui") { return sgct::config::Window::ColorBitDepth::Depth16UInt; }
        if (type == "32ui") { return sgct::config::Window::ColorBitDepth::Depth32UInt; }

        throw Err(6086, std::format("Unknown color bit depth {}", type));
    }

    int cubeMapResolutionForQuality(std::string_view quality) {
        if (quality == "low" || quality == "256") { return 256; }
        if (quality == "medium" || quality == "512") { return 512; }
        if (quality == "high" || quality == "1k" || quality == "1024") { return 1024; }
        if (quality == "1.5k" || quality == "1536") { return 1536; }
        if (quality == "2k" || quality == "2048") { return 2048; }
        if (quality == "4k" || quality == "4096") { return 4096; }
        if (quality == "8k" || quality == "8192") { return 8192; }
        if (quality == "16k" || quality == "16384") { return 16384; }
        if (quality == "32k" || quality == "32768") { return 32768; }
        if (quality == "64k" || quality == "65536") { return 65536; }

        throw Err(6087, std::format("Unknown resolution {} for cube map", quality));
    }

    sgct::config::Viewport::Eye parseEye(std::string_view eye) {
        if (eye == "center") { return sgct::config::Viewport::Eye::Mono; }
        if (eye == "left")   { return sgct::config::Viewport::Eye::StereoLeft; }
        if (eye == "right")  { return sgct::config::Viewport::Eye::StereoRight; }

        throw Err(6020, "Unrecognized eye position");
    }

    sgct::config::FisheyeProjection::Interpolation parseInterpolation(std::string_view i)
    {
        using namespace sgct::config;
        if (i == "cubic") { return FisheyeProjection::Interpolation::Cubic; }
        if (i == "linear") { return FisheyeProjection::Interpolation::Linear; }

        throw Err(6023, "Unregnozed interpolation");
    }

    std::string stringifyJsonFile(const std::filesystem::path& filename) {
        std::ifstream myfile = std::ifstream(filename);
        if (myfile.fail()) {
            throw Err(6082, std::format("Failed to open '{}'", filename.string()));
        }
        std::stringstream buffer;
        buffer << myfile.rdbuf();
        return buffer.str();
    }

    constexpr int8_t InvalidWindowIndex = std::numeric_limits<int8_t>::min();

    template <typename T> struct is_optional : std::false_type {};
    template <typename T> struct is_optional<std::optional<T>> : std::true_type {};

    template <typename T> struct is_vector : std::false_type {};
    template <typename T> struct is_vector<std::vector<T>> : std::true_type {};

    template <typename T>
    void parseValue(const nlohmann::json& j, std::string_view key, T& res) {
        if (auto it = j.find(key);  it != j.end()) {
            if constexpr (is_optional<T>::value) {
                res = it->get<typename T::value_type>();
            }
            else {
                it->get_to(res);
            }
        }
        else {
            if constexpr (is_optional<T>::value) {
                res = std::nullopt;
            }
            else if constexpr (is_vector<T>::value) {
                res = T();
            }
            else {
                throw std::runtime_error(std::format(
                    "Could not find required key '{}'", key)
                );
            }
        }
    }


} // namespace


namespace sgct {

static void from_json(const nlohmann::json& j, sgct::ivec2& v) {
    j.at("x").get_to(v.x);
    j.at("y").get_to(v.y);
}

static void to_json(nlohmann::json& j, const sgct::ivec2& v) {
    j = nlohmann::json::object();
    j["x"] = v.x;
    j["y"] = v.y;
}

static void from_json(const nlohmann::json& j, sgct::vec2& v) {
    j.at("x").get_to(v.x);
    j.at("y").get_to(v.y);
}

static void to_json(nlohmann::json& j, const sgct::vec2& v) {
    j = nlohmann::json::object();
    j["x"] = v.x;
    j["y"] = v.y;
}

static void from_json(const nlohmann::json& j, sgct::vec3& v) {
    j.at("x").get_to(v.x);
    j.at("y").get_to(v.y);
    j.at("z").get_to(v.z);
}

static void to_json(nlohmann::json& j, const sgct::vec3& v) {
    j = nlohmann::json::object();
    j["x"] = v.x;
    j["y"] = v.y;
    j["z"] = v.z;
}

static void from_json(const nlohmann::json& j, sgct::vec4& v) {
    auto itX = j.find("x");
    auto itY = j.find("y");
    auto itZ = j.find("z");
    auto itW = j.find("w");

    if (itX != j.end() && itY != j.end() && itZ != j.end() && itW != j.end()) {
        itX->get_to(v.x);
        itY->get_to(v.y);
        itZ->get_to(v.z);
        itW->get_to(v.w);
        return;
    }

    auto itR = j.find("r");
    auto itG = j.find("g");
    auto itB = j.find("b");
    auto itA = j.find("a");

    if (itR != j.end() && itG != j.end() && itB != j.end() && itA != j.end()) {
        itR->get_to(v.x);
        itG->get_to(v.y);
        itB->get_to(v.z);
        itA->get_to(v.w);
        return;
    }

    throw std::runtime_error("Incomplete vec4");
}

static void to_json(nlohmann::json& j, const sgct::vec4& v) {
    j = nlohmann::json::object();
    j["x"] = v.x;
    j["y"] = v.y;
    j["z"] = v.z;
    j["w"] = v.w;
}

static void from_json(const nlohmann::json& j, sgct::mat4& m) {
    std::array<double, 16> vs = j.get<std::array<double, 16>>();
    for (int i = 0; i < 16; i += 1) {
        m.values[i] = static_cast<float>(vs[i]);
    }
}

static void to_json(nlohmann::json& j, const sgct::mat4& m) {
    std::array<double, 16> vs;
    for (int i = 0; i < 16; i += 1) {
        vs[i] = m.values[i];
    }
    j = vs;
}

static void from_json(const nlohmann::json& j, sgct::quat& q) {
    auto itPitch = j.find("pitch");
    auto itYaw = j.find("yaw");
    auto itRoll = j.find("roll");
    if (itPitch != j.end() && itYaw != j.end() && itRoll != j.end()) {
        const float x = itPitch->get<float>();
        const float y = -itYaw->get<float>();
        const float z = -itRoll->get<float>();

        glm::quat quat = glm::dquat(1.0, 0.0, 0.0, 0.0);
        quat = glm::rotate(quat, glm::radians(y), glm::vec3(0.0, 1.0, 0.0));
        quat = glm::rotate(quat, glm::radians(x), glm::vec3(1.0, 0.0, 0.0));
        quat = glm::rotate(quat, glm::radians(z), glm::vec3(0.0, 0.0, 1.0));
        q = sgct::quat(quat.x, quat.y, quat.z, quat.w);
    }

    auto itX = j.find("x");
    auto itY = j.find("y");
    auto itZ = j.find("z");
    auto itW = j.find("w");
    if (itX != j.end() && itY != j.end() && itZ != j.end() && itW != j.end()) {
        itX->get_to(q.x);
        itY->get_to(q.y);
        itZ->get_to(q.z);
        itW->get_to(q.w);
    }
}

static void to_json(nlohmann::json& j, const sgct::quat& q) {
    j = nlohmann::json::object();
    j["x"] = q.x;
    j["y"] = q.y;
    j["z"] = q.z;
    j["w"] = q.w;
}

} // namespace sgct

namespace sgct::config {

static void from_json(const nlohmann::json& j, Scene& s) {
    parseValue(j, "offset", s.offset);
    parseValue(j, "orientation", s.orientation);
    parseValue(j, "scale", s.scale);
}

static void to_json(nlohmann::json& j, const Scene& s) {
    j = nlohmann::json::object();

    if (s.offset.has_value()) {
        j["offset"] = *s.offset;
    }
    if (s.orientation.has_value()) {
        j["orientation"] = *s.orientation;
    }
    if (s.scale.has_value()) {
        j["scale"] = *s.scale;
    }
}

static void from_json(const nlohmann::json& j, User& u) {
    parseValue(j, "name", u.name);
    parseValue(j, "eyeseparation", u.eyeSeparation);
    parseValue(j, "pos", u.position);
    parseValue(j, "matrix", u.transformation);

    if (auto it = j.find("orientation");  it != j.end()) {
        const quat q = it->get<quat>();
        glm::mat4 m = glm::mat4_cast(glm::make_quat(&q.x));
        sgct::mat4 o;
        std::memcpy(&o, glm::value_ptr(m), 16 * sizeof(float));
        u.transformation = o;
    }

    if (auto it = j.find("tracking");  it != j.end()) {
        User::Tracking tracking;

        auto trackerIt = it->find("tracker");
        if (trackerIt == it->end()) {
            throw std::runtime_error("Missing key 'tracker' in User");
        }

        auto deviceIt = it->find("device");
        if (deviceIt == it->end()) {
            throw std::runtime_error("Missing key 'device' in User");
        }

        trackerIt->get_to(tracking.tracker);
        deviceIt->get_to(tracking.device);
        u.tracking = tracking;
    }
}

static void to_json(nlohmann::json& j, const User& u) {
    j = nlohmann::json::object();

    if (u.name.has_value()) {
        j["name"] = *u.name;
    }

    if (u.eyeSeparation.has_value()) {
        j["eyeseparation"] = *u.eyeSeparation;
    }

    if (u.position.has_value()) {
        j["pos"] = *u.position;
    }

    if (u.transformation.has_value()) {
        j["matrix"] = *u.transformation;
    }

    if (u.tracking.has_value()) {
        nlohmann::json tracking = nlohmann::json::object();
        tracking["tracker"] = u.tracking->tracker;
        tracking["device"] = u.tracking->device;
        j["tracking"] = tracking;
    }
}

static void from_json(const nlohmann::json& j, Settings& s) {
    parseValue(j, "depthbuffertexture", s.useDepthTexture);
    parseValue(j, "normaltexture", s.useNormalTexture);
    parseValue(j, "positiontexture", s.usePositionTexture);

    if (auto it = j.find("precision");  it != j.end()) {
        float precision = it->get<float>();
        if (precision == 16.f) {
            s.bufferFloatPrecision = Settings::BufferFloatPrecision::Float16Bit;
        }
        else if (precision == 32.f) {
            s.bufferFloatPrecision = Settings::BufferFloatPrecision::Float32Bit;
        }
        else {
            throw Err(6050, std::format("Wrong buffer precision value {}", precision));
        }
    }

    if (auto it = j.find("display");  it != j.end()) {
        Settings::Display display;
        parseValue(*it, "swapinterval", display.swapInterval);
        parseValue(*it, "refreshrate", display.refreshRate);
        s.display = display;
    }
}

static void to_json(nlohmann::json& j, const Settings& s) {
    j = nlohmann::json::object();

    if (s.useDepthTexture.has_value()) {
        j["depthbuffertexture"] = *s.useDepthTexture;
    }

    if (s.useNormalTexture.has_value()) {
        j["normaltexture"] = *s.useNormalTexture;
    }

    if (s.usePositionTexture.has_value()) {
        j["positiontexture"] = *s.usePositionTexture;
    }

    if (s.bufferFloatPrecision.has_value()) {
        switch (*s.bufferFloatPrecision) {
            case Settings::BufferFloatPrecision::Float16Bit:
                j["precision"] = 16.0;
                break;
            case Settings::BufferFloatPrecision::Float32Bit:
                j["precision"] = 32.0;
                break;
        }
    }

    if (s.display.has_value()) {
        nlohmann::json display = nlohmann::json::object();
        if (s.display->swapInterval.has_value()) {
            display["swapinterval"] = *s.display->swapInterval;
        }
        if (s.display->refreshRate.has_value()) {
            display["refreshrate"] = *s.display->refreshRate;
        }
        j["display"] = display;
    }
}

static void from_json(const nlohmann::json& j, Capture& c) {
    parseValue(j, "path", c.path);

    std::optional<int> rangeBeg;
    parseValue(j, "rangebegin", rangeBeg);
    std::optional<int> rangeEnd;
    parseValue(j, "rangeend", rangeEnd);

    if (rangeBeg || rangeEnd) {
        c.range = Capture::ScreenShotRange();
    }

    if (rangeBeg) {
        c.range->first = *rangeBeg;
    }
    if (rangeEnd) {
        c.range->last = *rangeEnd;
    }

    if (rangeBeg && rangeEnd && *rangeBeg > *rangeEnd) {
        throw Err(6051, "End of range must be greater than beginning of range");
    }
}

static void to_json(nlohmann::json& j, const Capture& c) {
    j = nlohmann::json::object();

    if (c.path.has_value()) {
        j["path"] = *c.path;
    }

    if (c.range.has_value()) {
        j["rangebegin"] = c.range->first;
        j["rangeend"] = c.range->last;
    }
}

static void from_json(const nlohmann::json& j, Tracker::Device::Sensor& s) {
    j.at("vrpnaddress").get_to(s.vrpnAddress);
    j.at("id").get_to(s.identifier);
}

static void to_json(nlohmann::json& j, const Tracker::Device::Sensor& s) {
    j = nlohmann::json::object();

    j["vrpnaddress"] = s.vrpnAddress;
    j["id"] = s.identifier;
}

static void from_json(const nlohmann::json& j, Tracker::Device::Button& b) {
    j.at("vrpnaddress").get_to(b.vrpnAddress);
    j.at("count").get_to(b.count);
}

static void to_json(nlohmann::json& j, const Tracker::Device::Button& b) {
    j = nlohmann::json::object();

    j["vrpnaddress"] = b.vrpnAddress;
    j["count"] = b.count;
}

static void from_json(const nlohmann::json& j, Tracker::Device::Axis& a) {
    j.at("vrpnaddress").get_to(a.vrpnAddress);
    j.at("count").get_to(a.count);
}

static void to_json(nlohmann::json& j, const Tracker::Device::Axis& a) {
    j = nlohmann::json::object();

    j["vrpnaddress"] = a.vrpnAddress;
    j["count"] = a.count;
}

static void from_json(const nlohmann::json& j, Tracker::Device& d) {
    parseValue(j, "name", d.name);
    parseValue(j, "sensors", d.sensors);
    parseValue(j, "buttons", d.buttons);
    parseValue(j, "axes", d.axes);
    parseValue(j, "offset", d.offset);
    parseValue(j, "matrix", d.transformation);
}

static void to_json(nlohmann::json& j, const Tracker::Device& d) {
    j = nlohmann::json::object();

    j["name"] = d.name;
    j["sensors"] = d.sensors;
    j["buttons"] = d.buttons;
    j["axes"] = d.axes;

    if (d.offset.has_value()) {
        j["offset"] = *d.offset;
    }

    if (d.transformation.has_value()) {
        j["matrix"] = *d.transformation;
    }
}

static void from_json(const nlohmann::json& j, Tracker& t) {
    if (auto it = j.find("name");  it != j.end()) {
        it->get_to(t.name);
    }
    else {
        throw Err(6070, "Tracker is missing 'name'");
    }
    parseValue(j, "devices", t.devices);
    parseValue(j, "offset", t.offset);

    parseValue(j, "scale", t.scale);
    parseValue(j, "matrix", t.transformation);

    if (auto it = j.find("orientation");  it != j.end()) {
        const quat q = it->get<quat>();
        glm::mat4 m = glm::mat4_cast(glm::make_quat(&q.x));
        sgct::mat4 o;
        std::memcpy(&o, glm::value_ptr(m), 16 * sizeof(float));
        t.transformation = o;
    }
}

static void to_json(nlohmann::json& j, const Tracker& t) {
    j = nlohmann::json::object();

    j["name"] = t.name;
    j["devices"] = t.devices;

    if (t.offset.has_value()) {
        j["offset"] = *t.offset;
    }

    if (t.transformation.has_value()) {
        j["matrix"] = *t.transformation;
    }

    if (t.scale.has_value()) {
        j["scale"] = *t.scale;
    }
}

static void from_json(const nlohmann::json& j, PlanarProjection::FOV& f) {
    auto itHFov = j.find("hfov");
    auto itVFov = j.find("vfov");

    auto itDown = j.find("down");
    auto itLeft = j.find("left");
    auto itRight = j.find("right");
    auto itUp = j.find("up");

    const bool hasHorizontal =
        itHFov != j.end() || (itLeft != j.end() && itRight != j.end());
    const bool hasVertical = itVFov != j.end() || (itDown != j.end() && itUp != j.end());
    if (!hasHorizontal || !hasVertical) {
        throw Err(6000, "Missing specification of field-of-view values");
    }

    // First we extract the potentially existing hFov and vFov values and **then** the
    // more specific left/right/up/down ones which would overwrite the first set
    if (itHFov != j.end()) {
        const float hFov = itHFov->get<float>();
        f.left = hFov / 2.f;
        f.right = hFov / 2.f;
    }

    if (itVFov != j.end()) {
        const float vFov = itVFov->get<float>();
        f.down = vFov / 2.f;
        f.up = vFov / 2.f;
    }

    if (itDown != j.end()) {
        itDown->get_to(f.down);
    }

    if (itLeft != j.end()) {
        itLeft->get_to(f.left);
    }

    if (itRight != j.end()) {
        itRight->get_to(f.right);
    }

    if (itUp != j.end()) {
        itUp->get_to(f.up);
    }

    // The negative signs here were lifted up from the viewport class. I think it is nicer
    // to store them in negative values and consider the fact that the down and left fovs
    // are inverted to be a detail of the JSON configuration specification
    f.down *= -1.f;
    f.left *= -1.f;


    parseValue(j, "distance", f.distance);
}

static void to_json(nlohmann::json& j, const PlanarProjection::FOV& f) {
    j = nlohmann::json::object();

    if (f.left == f.right) {
        j["hfov"] = -f.left + f.right;
    }
    else {
        j["left"] = -f.left;
        j["right"] = f.right;
    }

    if (f.down == f.up) {
        j["vfov"] = -f.down + f.up;
    }
    else {
        j["down"] = -f.down;
        j["up"] = f.up;
    }
}

static void from_json(const nlohmann::json& j, PlanarProjection& p) {
    if (auto it = j.find("fov");  it == j.end()) {
        throw Err(6000, "Missing specification of field-of-view values");
    }

    parseValue(j, "fov", p.fov);

    parseValue(j, "distance", p.fov.distance);
    parseValue(j, "orientation", p.orientation);
    parseValue(j, "offset", p.offset);
}

static void to_json(nlohmann::json& j, const PlanarProjection& p) {
    j = nlohmann::json::object();

    j["fov"] = p.fov;

    if (p.fov.distance.has_value()) {
        j["distance"] = *p.fov.distance;
    }

    if (p.orientation.has_value()) {
        j["orientation"] = *p.orientation;
    }

    if (p.offset.has_value()) {
        j["offset"] = *p.offset;
    }
}

static void from_json(const nlohmann::json& j, TextureMappedProjection& p) {
    from_json(j, static_cast<PlanarProjection&>(p));
}

static void to_json(nlohmann::json& j, const TextureMappedProjection& p) {
    to_json(j, static_cast<const PlanarProjection&>(p));
}

static void from_json(const nlohmann::json& j, FisheyeProjection& p) {
    parseValue(j, "fov", p.fov);

    if (auto it = j.find("quality");  it != j.end()) {
        const std::string quality = it->get<std::string>();
        p.quality = cubeMapResolutionForQuality(quality);
    }

    if (auto it = j.find("interpolation");  it != j.end()) {
        const std::string interpolation = it->get<std::string>();
        p.interpolation = parseInterpolation(interpolation);
    }
    parseValue(j, "diameter", p.diameter);
    parseValue(j, "tilt", p.tilt);

    if (auto it = j.find("crop");  it != j.end()) {
        FisheyeProjection::Crop crop;
        if (auto jt = it->find("left");  jt == it->end()) {
            throw std::runtime_error("Missing key 'left' in FisheyeProjection/Crop");
        }
        crop.left = it->value("left", crop.left);

        if (auto jt = it->find("right");  jt == it->end()) {
            throw std::runtime_error("Missing key 'right' in FisheyeProjection/Crop");
        }
        crop.right = it->value("right", crop.right);

        if (auto jt = it->find("bottom");  jt == it->end()) {
            throw std::runtime_error("Missing key 'bottom' in FisheyeProjection/Crop");
        }
        crop.bottom = it->value("bottom", crop.bottom);

        if (auto jt = it->find("top");  jt == it->end()) {
            throw std::runtime_error("Missing key 'top' in FisheyeProjection/Crop");
        }
        crop.top = it->value("top", crop.top);
        p.crop = crop;
    }

    parseValue(j, "keepaspectratio", p.keepAspectRatio);
    parseValue(j, "offset", p.offset);
    parseValue(j, "background", p.background);
}

static void to_json(nlohmann::json& j, const FisheyeProjection& p) {
    j = nlohmann::json::object();

    if (p.fov.has_value()) {
        j["fov"] = *p.fov;
    }

    if (p.quality.has_value()) {
        j["quality"] = std::to_string(*p.quality);
    }

    if (p.interpolation.has_value()) {
        switch (*p.interpolation) {
            case FisheyeProjection::Interpolation::Cubic:
                j["interpolation"] = "cubic";
                break;
            case FisheyeProjection::Interpolation::Linear:
                j["interpolation"] = "linear";
                break;
        }
    }

    if (p.diameter.has_value()) {
        j["diameter"] = *p.diameter;
    }

    if (p.tilt.has_value()) {
        j["tilt"] = *p.tilt;
    }

    if (p.crop.has_value()) {
        nlohmann::json crop = nlohmann::json::object();
        crop["left"] = p.crop->left;
        crop["right"] = p.crop->right;
        crop["bottom"] = p.crop->bottom;
        crop["top"] = p.crop->top;
        j["crop"] = crop;
    }

    if (p.keepAspectRatio.has_value()) {
        j["keepaspectratio"] = *p.keepAspectRatio;
    }

    if (p.offset.has_value()) {
        j["offset"] = *p.offset;
    }

    if (p.background.has_value()) {
        j["background"] = *p.background;
    }
}

static void from_json(const nlohmann::json& j, SphericalMirrorProjection& p) {
    if (auto it = j.find("quality");  it != j.end()) {
        const std::string quality = it->get<std::string>();
        p.quality = cubeMapResolutionForQuality(quality);
    }

    parseValue(j, "tilt", p.tilt);
    parseValue(j, "background", p.background);

    if (auto it = j.find("geometry");  it != j.end()) {
        SphericalMirrorProjection::Mesh mesh;
        it->at("bottom").get_to(mesh.bottom);
        it->at("left").get_to(mesh.left);
        it->at("right").get_to(mesh.right);
        it->at("top").get_to(mesh.top);
        p.mesh = mesh;
    }
    else {
        throw Err(6100, "Missing geometry paths");
    }
}

static void to_json(nlohmann::json& j, const SphericalMirrorProjection& p) {
    j = nlohmann::json::object();

    if (p.quality.has_value()) {
        j["quality"] = std::to_string(*p.quality);
    }

    if (p.tilt.has_value()) {
        j["tilt"] = *p.tilt;
    }

    if (p.background.has_value()) {
        j["background"] = *p.background;
    }

    nlohmann::json mesh = nlohmann::json::object();
    mesh["bottom"] = p.mesh.bottom;
    mesh["left"] = p.mesh.left;
    mesh["right"] = p.mesh.right;
    mesh["top"] = p.mesh.top;
    j["geometry"] = mesh;
}


static void from_json(const nlohmann::json& j, CubemapProjection::Channels& c) {
    parseValue(j, "right", c.right);
    parseValue(j, "zleft", c.zLeft);
    parseValue(j, "bottom", c.bottom);
    parseValue(j, "top", c.top);
    parseValue(j, "left", c.left);
    parseValue(j, "zright", c.zRight);
}

static void from_json(const nlohmann::json& j, CubemapProjection::Spout& s) {
    parseValue(j, "enabled", s.enabled);
    parseValue(j, "name", s.name);
}

static void from_json(const nlohmann::json& j, CubemapProjection::NDI& n) {
    parseValue(j, "enabled", n.enabled);
    parseValue(j, "name", n.name);
    parseValue(j, "groups", n.groups);
}

static void from_json(const nlohmann::json& j, CubemapProjection& p) {
    if (auto it = j.find("quality");  it != j.end()) {
        const std::string quality = it->get<std::string>();
        p.quality = cubeMapResolutionForQuality(quality);
    }

    parseValue(j, "spout", p.spout);
    parseValue(j, "ndi", p.ndi);
    parseValue(j, "channels", p.channels);

    if (auto it = j.find("orientation");  it != j.end()) {
        sgct::vec3 orientation;
        parseValue(*it, "pitch", orientation.x);
        parseValue(*it, "yaw", orientation.y);
        parseValue(*it, "roll", orientation.z);
        p.orientation = orientation;
    }
}

static void to_json(nlohmann::json& j, const CubemapProjection::Channels& c) {
    j["right"] = c.right;
    j["zleft"] = c.zLeft;
    j["bottom"] = c.bottom;
    j["top"] = c.top;
    j["left"] = c.left;
    j["zright"] = c.zRight;
}

static void to_json(nlohmann::json& j, const CubemapProjection::Spout& s) {
    j["enabled"] = s.enabled;
    if (s.name) {
        j["name"] = *s.name;
    }
}

static void to_json(nlohmann::json& j, const CubemapProjection::NDI& n) {
    j["enabled"] = n.enabled;
    if (n.name) {
        j["name"] = *n.name;
    }
    if (n.groups) {
        j["groups"] = *n.groups;
    }
}

static void to_json(nlohmann::json& j, const CubemapProjection& p) {
    j = nlohmann::json::object();

    if (p.quality.has_value()) {
        j["quality"] = std::to_string(*p.quality);
    }

    if (p.spout) {
        j["spout"] = *p.spout;
    }

    if (p.ndi) {
        j["ndi"] = *p.ndi;
    }

    if (p.channels.has_value()) {
        j["channels"] = *p.channels;
    }

    if (p.orientation.has_value()) {
        nlohmann::json orientation = nlohmann::json::object();
        orientation["pitch"] = p.orientation->x;
        orientation["yaw"] = p.orientation->y;
        orientation["roll"] = p.orientation->z;
        j["orientation"] = orientation;
    }
}

static void from_json(const nlohmann::json& j, CylindricalProjection& p) {
    if (auto it = j.find("quality");  it != j.end()) {
        const std::string quality = it->get<std::string>();
        p.quality = cubeMapResolutionForQuality(quality);
    }

    parseValue(j, "rotation", p.rotation);
    parseValue(j, "heightoffset", p.heightOffset);
    parseValue(j, "radius", p.radius);
}

static void to_json(nlohmann::json& j, const CylindricalProjection& p) {
    j = nlohmann::json::object();

    if (p.quality.has_value()) {
        j["quality"] = std::to_string(*p.quality);
    }

    if (p.rotation.has_value()) {
        j["rotation"] = *p.rotation;
    }

    if (p.heightOffset.has_value()) {
        j["heightoffset"] = *p.heightOffset;
    }

    if (p.radius.has_value()) {
        j["radius"] = *p.radius;
    }
}

static void from_json(const nlohmann::json& j, EquirectangularProjection& p) {
    if (auto it = j.find("quality");  it != j.end()) {
        const std::string quality = it->get<std::string>();
        p.quality = cubeMapResolutionForQuality(quality);
    }
}

static void to_json(nlohmann::json& j, const EquirectangularProjection& p) {
    if (p.quality.has_value()) {
        j["quality"] = std::to_string(*p.quality);
    }
}

static void from_json(const nlohmann::json& j, ProjectionPlane& p) {
    auto itLl = j.find("lowerleft");
    auto itUl = j.find("upperleft");
    auto itUr = j.find("upperright");

    if (itLl == j.end() || itUl == j.end() || itUr == j.end()) {
        throw Err(6010, "Failed parsing coordinates. Missing elements");
    }

    j.at("lowerleft").get_to(p.lowerLeft);
    j.at("upperleft").get_to(p.upperLeft);
    j.at("upperright").get_to(p.upperRight);
}

static void to_json(nlohmann::json& j, const ProjectionPlane& p) {
    j["lowerleft"] = p.lowerLeft;
    j["upperleft"] = p.upperLeft;
    j["upperright"] = p.upperRight;
}

static void from_json(const nlohmann::json& j, Viewport& v) {
    parseValue(j, "user", v.user);
    if (auto it = j.find("overlay");  it != j.end()) {
        v.overlayTexture = std::filesystem::absolute(it->get<std::string>());
    }
    if (auto it = j.find("blendmask");  it != j.end()) {
        v.blendMaskTexture = std::filesystem::absolute(it->get<std::string>());
    }
    if (auto it = j.find("blacklevelmask");  it != j.end()) {
        v.blackLevelMaskTexture =
            std::filesystem::absolute(it->get<std::string>());
    }
    if (auto it = j.find("mesh");  it != j.end()) {
        v.correctionMeshTexture =
            std::filesystem::absolute(it->get<std::string>());
    }

    parseValue(j, "tracked", v.isTracked);

    if (auto it = j.find("eye");  it != j.end()) {
        const std::string eye = it->get<std::string>();
        v.eye = parseEye(eye);
    }

    parseValue(j, "pos", v.position);
    parseValue(j, "size", v.size);

    std::string type;
    if (auto it = j.find("projection");  it != j.end()) {
        if (it->is_null()) {
            v.projection = NoProjection();
        }
        else {
            type = it->at("type").get<std::string>();
            if (type == "CubemapProjection") {
                v.projection = it->get<CubemapProjection>();
            }
            else if (type == "CylindricalProjection") {
                v.projection = it->get<CylindricalProjection>();
            }
            else if (type == "EquirectangularProjection") {
                v.projection = it->get<EquirectangularProjection>();
            }
            else if (type == "FisheyeProjection") {
                v.projection = it->get<FisheyeProjection>();
            }
            else if (type == "PlanarProjection") {
                v.projection = it->get<PlanarProjection>();
            }
            else if (type == "ProjectionPlane") {
                v.projection = it->get<ProjectionPlane>();
            }
            else if (type == "SphericalMirrorProjection") {
                v.projection = it->get<SphericalMirrorProjection>();
            }
            else if (type == "TextureMappedProjection") {
                v.projection = it->get<TextureMappedProjection>();
            }
            else {
                throw Err(6089, std::format("Unknown projection type '{}'", type));
            }
        }
    }

    if (type == "TextureMappedProjection") {
        if (!v.correctionMeshTexture) {
            throw Err(6110, "Missing correction mesh for TextureMappedProjection");
        }
    }
}

static void to_json(nlohmann::json& j, const Viewport& v) {
    if (v.user.has_value()) {
        j["user"] = *v.user;
    }

    if (v.overlayTexture.has_value()) {
        j["overlay"] = *v.overlayTexture;
    }

    if (v.blendMaskTexture.has_value()) {
        j["blendmask"] = *v.blendMaskTexture;
    }

    if (v.blackLevelMaskTexture.has_value()) {
        j["blacklevelmask"] = *v.blackLevelMaskTexture;
    }

    if (v.correctionMeshTexture.has_value()) {
        j["mesh"] = *v.correctionMeshTexture;
    }

    if (v.isTracked.has_value()) {
        j["tracked"] = *v.isTracked;
    }

    if (v.eye.has_value()) {
        switch (*v.eye) {
            case Viewport::Eye::Mono:
                j["eye"] = "center";
                break;
            case Viewport::Eye::StereoLeft:
                j["eye"] = "left";
                break;
            case Viewport::Eye::StereoRight:
                j["eye"] = "right";
                break;
        }
    }

    if (v.position.has_value()) {
        j["pos"] = *v.position;
    }

    if (v.size.has_value()) {
        j["size"] = *v.size;
    }

    j["projection"] = std::visit(overloaded{
        [](const config::NoProjection&) {
            return nlohmann::json();
        },
        [](const config::PlanarProjection& p) {
            nlohmann::json proj = p;
            proj["type"] = "PlanarProjection";
            return proj;
        },
        [](const config::TextureMappedProjection& p) {
            nlohmann::json proj = p;
            proj["type"] = "TextureMappedProjection";
            return proj;
        },
        [](const config::FisheyeProjection& p) {
            nlohmann::json proj = p;
            proj["type"] = "FisheyeProjection";
            return proj;
        },
        [](const config::SphericalMirrorProjection& p) {
            nlohmann::json proj = p;
            proj["type"] = "SphericalMirrorProjection";
            return proj;
        },
        [](const config::CubemapProjection& p) {
            nlohmann::json proj = p;
            proj["type"] = "CubemapProjection";
            return proj;
        },
        [](const config::CylindricalProjection& p) {
            nlohmann::json proj = p;
            proj["type"] = "CylindricalProjection";
            return proj;
        },
        [](const config::EquirectangularProjection& p) {
            nlohmann::json proj = p;
            proj["type"] = "EquirectangularProjection";
            return proj;
        },
        [](const config::ProjectionPlane& p) {
            nlohmann::json proj = p;
            proj["type"] = "ProjectionPlane";
            return proj;
        }
        }, v.projection);
}

static void from_json(const nlohmann::json& j, Window::Spout& n) {
    parseValue(j, "enabled", n.enabled);
    parseValue(j, "name", n.name);
}

static void from_json(const nlohmann::json& j, Window::NDI& n) {
    parseValue(j, "enabled", n.enabled);
    parseValue(j, "name", n.name);
    parseValue(j, "groups", n.groups);
}

static void from_json(const nlohmann::json& j, Window& w) {
    std::optional<int8_t> id;
    parseValue(j, "id", id);
    w.id = id.value_or(InvalidWindowIndex);

    parseValue(j, "name", w.name);
    parseValue(j, "tags", w.tags);
    parseValue(j, "hidemousecursor", w.hideMouseCursor);
    parseValue(j, "takescreenshot", w.takeScreenshot);
    parseValue(j, "alpha", w.alpha);
    parseValue(j, "draw2d", w.draw2D);
    parseValue(j, "draw3d", w.draw3D);

    std::optional<std::filesystem::path> mesh;
    parseValue(j, "scalablemesh", mesh);
    if (mesh) {
        w.scalable = Window::Scalable();
        w.scalable->mesh = std::filesystem::current_path() / *mesh;
        parseValue(j, "scalablemesh_ortho_quality", w.scalable->orthographicQuality);
        parseValue(j, "scalablemesh_ortho_res", w.scalable->orthographicResolution);

        // If we have a scalable mesh, we don't want to parse the rest of the values
        return;
    }


    if (auto it = j.find("bufferbitdepth");  it != j.end()) {
        const std::string bbd = it->get<std::string>();
        w.bufferBitDepth = parseBufferColorBitDepth(bbd);
    }

    parseValue(j, "fullscreen", w.isFullScreen);
    parseValue(j, "autoiconify", w.shouldAutoiconify);
    parseValue(j, "floating", w.isFloating);
    parseValue(j, "alwaysrender", w.alwaysRender);
    parseValue(j, "hidden", w.isHidden);

    parseValue(j, "msaa", w.msaa);
    parseValue(j, "fxaa", w.useFxaa);

    parseValue(j, "border", w.isDecorated);
    parseValue(j, "resizable", w.isResizable);
    parseValue(j, "noerror", w.noError);
    parseValue(j, "blitwindowid", w.blitWindowId);
    parseValue(j, "mirrorx", w.mirrorX);
    parseValue(j, "mirrory", w.mirrorY);
    parseValue(j, "monitor", w.monitor);

    if (auto it = j.find("stereo");  it != j.end()) {
        w.stereo = parseStereoType(it->get<std::string>());
    }

    parseValue(j, "spout", w.spout);
    parseValue(j, "ndi", w.ndi);

    parseValue(j, "pos", w.pos);
    parseValue(j, "size", w.size);
    parseValue(j, "res", w.resolution);

    parseValue(j, "viewports", w.viewports);
}

static void to_json(nlohmann::json& j, const Window::Spout& n) {
    j["enabled"] = n.enabled;
    if (n.name) {
        j["name"] = *n.name;
    }
}

static void to_json(nlohmann::json& j, const Window::NDI& n) {
    j["enabled"] = n.enabled;
    if (n.name) {
        j["name"] = *n.name;
    }
    if (n.groups) {
        j["groups"] = *n.groups;
    }
}

static void to_json(nlohmann::json& j, const Window& w) {
    j["id"] = w.id;

    if (w.name.has_value()) {
        j["name"] = *w.name;
    }

    if (!w.tags.empty()) {
        j["tags"] = w.tags;
    }

    if (w.hideMouseCursor.has_value()) {
        j["hidemousecursor"] = *w.hideMouseCursor;
    }

    if (w.takeScreenshot.has_value()) {
        j["takescreenshot"] = *w.takeScreenshot;
    }

    if (w.draw2D.has_value()) {
        j["draw2d"] = *w.draw2D;
    }

    if (w.draw3D.has_value()) {
        j["draw3d"] = *w.draw3D;
    }

    if (w.bufferBitDepth.has_value()) {
        switch (*w.bufferBitDepth) {
            case Window::ColorBitDepth::Depth8:
                j["bufferbitdepth"] = "8";
                break;
            case Window::ColorBitDepth::Depth16:
                j["bufferbitdepth"] = "16";
                break;
            case Window::ColorBitDepth::Depth16Float:
                j["bufferbitdepth"] = "16f";
                break;
            case Window::ColorBitDepth::Depth32Float:
                j["bufferbitdepth"] = "32f";
                break;
            case Window::ColorBitDepth::Depth16Int:
                j["bufferbitdepth"] = "16i";
                break;
            case Window::ColorBitDepth::Depth32Int:
                j["bufferbitdepth"] = "32i";
                break;
            case Window::ColorBitDepth::Depth16UInt:
                j["bufferbitdepth"] = "16ui";
                break;
            case Window::ColorBitDepth::Depth32UInt:
                j["bufferbitdepth"] = "32ui";
                break;
        }
    }

    if (w.isFullScreen.has_value()) {
        j["fullscreen"] = *w.isFullScreen;
    }

    if (w.shouldAutoiconify.has_value()) {
        j["autoiconify"] = *w.shouldAutoiconify;
    }

    if (w.isFloating.has_value()) {
        j["floating"] = *w.isFloating;
    }

    if (w.alwaysRender.has_value()) {
        j["alwaysrender"] = *w.alwaysRender;
    }

    if (w.isHidden.has_value()) {
        j["hidden"] = *w.isHidden;
    }

    if (w.msaa.has_value()) {
        j["msaa"] = *w.msaa;
    }

    if (w.useFxaa.has_value()) {
        j["fxaa"] = *w.useFxaa;
    }

    if (w.isDecorated.has_value()) {
        j["border"] = *w.isDecorated;
    }

    if (w.isResizable.has_value()) {
        j["resizable"] = *w.isResizable;
    }

    if (w.noError.has_value()) {
        j["noerror"] = *w.noError;
    }

    if (w.blitWindowId.has_value()) {
        j["blitwindowid"] = *w.blitWindowId;
    }

    if (w.monitor.has_value()) {
        j["monitor"] = *w.monitor;
    }

    if (w.mirrorX.has_value()) {
        j["mirrorx"] = *w.mirrorX;
    }

    if (w.mirrorY.has_value()) {
        j["mirrory"] = *w.mirrorY;
    }

    if (w.stereo.has_value()) {
        j["stereo"] = toString(*w.stereo);
    }

    if (w.spout.has_value()) {
        j["spout"] = *w.spout;
    }

    if (w.ndi.has_value()) {
        j["ndi"] = *w.ndi;
    }

    if (w.pos.has_value()) {
        j["pos"] = *w.pos;
    }

    j["size"] = w.size;

    if (w.resolution.has_value()) {
        j["res"] = *w.resolution;
    }

    if (w.scalable.has_value()) {
        // We add the current path to the mesh in the loading step, so here we need to
        // remove it again to make the paths patch
        j["scalablemesh"] = w.scalable->mesh;

        if (w.scalable->orthographicQuality.has_value()) {
            j["scalable_ortho_quality"] = *w.scalable->orthographicQuality;
        }
        if (w.scalable->orthographicResolution.has_value()) {
            j["scalable_ortho_resolution"] = *w.scalable->orthographicResolution;
        }
    }

    if (!w.viewports.empty()) {
        j["viewports"] = w.viewports;
    }
}

static void from_json(const nlohmann::json& j, Node& n) {
    if (auto it = j.find("address");  it != j.end()) {
        it->get_to(n.address);
    }
    else {
        throw Err(6040, "Missing field address in node");
    }

    simplifyIpAddress(n.address);

    if (auto it = j.find("port");  it != j.end()) {
        it->get_to(n.port);
    }
    else {
        throw Err(6041, "Missing field port in node");
    }

    parseValue(j, "datatransferport", n.dataTransferPort);
    parseValue(j, "swaplock", n.swapLock);

    parseValue(j, "windows", n.windows);
    if (n.windows.size() > std::numeric_limits<int8_t>::max()) {
        throw Err(6042, "Only 127 windows are supported");
    }
    for (size_t i = 0; i < n.windows.size(); i += 1) {
        if (n.windows[i].id == InvalidWindowIndex) {
            n.windows[i].id = static_cast<int8_t>(i);
        }
    }
}

static void to_json(nlohmann::json& j, const Node& n) {
    j["address"] = n.address;
    j["port"] = n.port;

    if (n.dataTransferPort.has_value()) {
        j["datatransferport"] = *n.dataTransferPort;
    }

    if (n.swapLock.has_value()) {
        j["swaplock"] = *n.swapLock;
    }

    if (!n.windows.empty()) {
        j["windows"] = n.windows;
    }
}

static void from_json(const nlohmann::json& j, GeneratorVersion& v) {
    parseValue(j, "name", v.name);
    parseValue(j, "major", v.major);
    parseValue(j, "minor", v.minor);
}

static void to_json(nlohmann::json& j, const GeneratorVersion& v) {
    j["name"] = v.name;
    j["major"] = v.major;
    j["minor"] = v.minor;
}

static void from_json(const nlohmann::json& j, Meta& m) {
    parseValue(j, "description", m.description);
    parseValue(j, "name", m.name);
    parseValue(j, "author", m.author);
    parseValue(j, "license", m.license);
    parseValue(j, "version", m.version);
}

static void to_json(nlohmann::json& j, const Meta& m) {
    if (m.description.has_value()) {
        j["description"] = *m.description;
    }
    if (m.name.has_value()) {
        j["name"] = *m.name;
    }
    if (m.author.has_value()) {
        j["author"] = *m.author;
    }
    if (m.license.has_value()) {
        j["license"] = *m.license;
    }
    if (m.version.has_value()) {
        j["version"] = *m.version;
    }
}

static void from_json(const nlohmann::json& j, Cluster& c) {
    if (auto it = j.find("masteraddress");  it != j.end()) {
        it->get_to(c.masterAddress);
    }
    else {
        throw Err(6084, "Cannot find master address");
    }

    simplifyIpAddress(c.masterAddress);

    parseValue(j, "debuglog", c.debugLog);
    parseValue(j, "threadaffinity", c.threadAffinity);
    if (c.threadAffinity && *c.threadAffinity < 0) {
        throw Err(6088, "Thread Affinity must be 0 or positive");
    }
    parseValue(j, "firmsync", c.firmSync);

    parseValue(j, "scene", c.scene);
    parseValue(j, "users", c.users);
    parseValue(j, "settings", c.settings);
    parseValue(j, "capture", c.capture);

    parseValue(j, "trackers", c.trackers);
    parseValue(j, "nodes", c.nodes);

    parseValue(j, "generator", c.generator);
    parseValue(j, "meta", c.meta);
}

static void to_json(nlohmann::json& j, const Cluster& c) {
    j["masteraddress"] = c.masterAddress;

    if (c.threadAffinity.has_value()) {
        j["threadaffinity"] = *c.threadAffinity;
    }

    if (c.debugLog.has_value()) {
        j["debuglog"] = *c.debugLog;
    }

    if (c.firmSync.has_value()) {
        j["firmsync"] = *c.firmSync;
    }

    if (c.scene.has_value()) {
        j["scene"] = *c.scene;
    }

    if (!c.users.empty()) {
        j["users"] = c.users;
    }

    if (c.settings.has_value()) {
        j["settings"] = *c.settings;
    }

    if (c.capture.has_value()) {
        j["capture"] = *c.capture;
    }

    if (!c.trackers.empty()) {
        j["trackers"] = c.trackers;
    }

    if (!c.nodes.empty()) {
        j["nodes"] = c.nodes;
    }

    if (c.generator) {
        j["generator"] = *c.generator;
    }

    if (c.meta) {
        j["meta"] = *c.meta;
    }
}

} // namespace sgct::config

namespace sgct {

config::Cluster readConfig(const std::filesystem::path& filename) {
    if (filename.empty()) {
        throw Err(6080, "No configuration file provided");
    }

    std::filesystem::path name = std::filesystem::absolute(filename);
    if (!std::filesystem::exists(name)) {
        throw Err(
            6081,
            std::format("Could not find configuration file: {}", name.string())
        );
    }

    // First save the old current working directory, set the new one
    const std::filesystem::path oldPwd = std::filesystem::current_path();
    const std::filesystem::path configFolder = std::filesystem::path(name).parent_path();
    if (!configFolder.empty()) {
        std::filesystem::current_path(configFolder);
    }

    // Then load the cluster
    try {
        std::ifstream f = std::ifstream(name);
        const std::string contents = std::string(
            std::istreambuf_iterator<char>(f),
            std::istreambuf_iterator<char>()
        );
        const config::Cluster cluster = readJsonConfig(contents);
        // and reset the current working directory to the old value
        std::filesystem::current_path(oldPwd);
        return cluster;
    }
    catch (const nlohmann::json::exception& e) {
        // and reset the current working directory to the old value
        std::filesystem::current_path(oldPwd);
        throw Err(6082, e.what());
    }
}

config::Cluster readJsonConfig(std::string_view configuration) {
    nlohmann::json j = nlohmann::json::parse(configuration);

    auto it = j.find("version");
    if (it == j.end()) {
        throw std::runtime_error("Missing 'version' information");
    }

    config::Cluster cluster;
    from_json(j, cluster);
    cluster.success = true;
    return cluster;
}

config::Cluster defaultCluster() {
    config::PlanarProjection::FOV fov;
    fov.down = -(90.f / (16.f / 9.f)) / 2.f;
    fov.left = -90.f / 2.f;
    fov.right = 90.f / 2.f;
    fov.up = (90.f / (16.f / 9.f)) / 2.f;
    config::PlanarProjection proj;
    proj.fov = fov;

    config::Viewport viewport;
    viewport.projection = proj;

    config::Window window;
    window.id = 0;
    window.isFullScreen = false;
    window.size = ivec2{ 1280, 720 };
    window.viewports.push_back(viewport);

    config::Node node;
    node.address = "localhost";
    node.port = 20401;
    node.windows.push_back(window);

    config::User user;
    user.eyeSeparation = 0.06f;
    user.position = vec3{ 0.f, 0.f, 0.f };

    config::Cluster res;
    res.success = true;
    res.masterAddress = "localhost";
    res.nodes.push_back(node);
    res.users.push_back(user);

    return res;
}

std::string serializeConfig(const config::Cluster& cluster,
                            std::optional<config::GeneratorVersion> genVersion)
{
    nlohmann::json res;
    res["version"] = 1;
    if (genVersion) {
        res["generator"] = genVersion.value();
    }
    to_json(res, cluster);
    return res.dump(2);
}

std::string validateConfigAgainstSchema(std::string_view configuration,
                                        const std::filesystem::path& schema)
{
    using nlohmann::json;
    using nlohmann::json_uri;
    using nlohmann::json_schema::basic_error_handler;
    using nlohmann::json_schema::json_validator;

    const std::string schemaStr = stringifyJsonFile(schema);
    const json schemaInput = json::parse(schemaStr);
    const std::filesystem::path schemaDir = std::filesystem::path(schema).parent_path();
    const json_validator validator = json_validator(
        schemaInput,
        [&schemaDir](const json_uri& id, json& value) {
            std::string loadPath = std::format("{}/{}", schemaDir.string(), id.to_string());
            const size_t lbIndex = loadPath.find('#');
            if (lbIndex != std::string::npos) {
                loadPath = loadPath.substr(0, lbIndex);
            }
            // Remove trailing spaces
            if (!loadPath.empty()) {
                const size_t strEnd = loadPath.find_last_not_of(" #\t\r\n\0");
                loadPath = loadPath.substr(0, strEnd + 1);
            }
            if (std::filesystem::exists(loadPath)) {
                Log::Debug(std::format("Loading schema file '{}'", loadPath));
                const std::string newSchema = stringifyJsonFile(loadPath);
                value = json::parse(newSchema);
            }
            else {
                throw Err(
                    6081,
                    std::format("Could not find schema file to load: {}", loadPath)
                );
            }
        }
    );
    json config;
    // The configuration passed into us can either be a path to a file that we should load
    // or the raw string of a configuration
    if (std::filesystem::is_regular_file(configuration)) {
        std::string configStr = stringifyJsonFile(configuration);
        config = json::parse(configStr);
    }
    else {
        config = json::parse(configuration);
    }
    try {
        validator.validate(config);
        return "";
    }
    catch (const std::exception& e) {
        return e.what();
    }
}

} // namespace sgct
