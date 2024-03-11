/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/readconfig.h>

#include <sgct/error.h>
#include <sgct/log.h>
#include <sgct/fmt.h>
#include <sgct/log.h>
#include <sgct/math.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <iostream>

#define Err(code, msg) sgct::Error(sgct::Error::Component::ReadConfig, code, msg)

namespace {
    template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

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

        throw Err(6085, fmt::format("Unknown stereo mode {}", t));
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

        throw Err(6086, fmt::format("Unknown color bit depth {}", type));
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

        throw Err(6087, fmt::format("Unknown resolution {} for cube map", quality));
    }

    sgct::config::Capture::Format parseImageFormat(std::string_view format) {
        using namespace sgct::config;

        if (format == "png" || format == "PNG") { return Capture::Format::PNG; }
        if (format == "tga" || format == "TGA") { return Capture::Format::TGA; }
        if (format == "jpg" || format == "JPG") { return Capture::Format::JPG; }
        throw Err(6060, "Unknown capturing format");
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

    sgct::config::SpoutOutputProjection::Mapping parseMapping(std::string_view mapping) {
        using namespace sgct::config;
        if (mapping == "fisheye") { return SpoutOutputProjection::Mapping::Fisheye; }
        if (mapping == "equirectangular") {
            return SpoutOutputProjection::Mapping::Equirectangular;
        }
        if (mapping == "cubemap") { return SpoutOutputProjection::Mapping::Cubemap; }

        throw Err(6086, fmt::format("Unknown spout output mapping: {}", mapping));
    }
} // namespace

// Define the JSON version functions that will make our live easier
namespace sgct {

void from_json(const nlohmann::json& j, sgct::ivec2& v) {
    j.at("x").get_to(v.x);
    j.at("y").get_to(v.y);
}

void to_json(nlohmann::json& j, const sgct::ivec2& v) {
    j = nlohmann::json::object();
    j["x"] = v.x;
    j["y"] = v.y;
}

void from_json(const nlohmann::json& j, sgct::vec2& v) {
    j.at("x").get_to(v.x);
    j.at("y").get_to(v.y);
}

void to_json(nlohmann::json& j, const sgct::vec2& v) {
    j = nlohmann::json::object();
    j["x"] = v.x;
    j["y"] = v.y;
}

void from_json(const nlohmann::json& j, sgct::vec3& v) {
    j.at("x").get_to(v.x);
    j.at("y").get_to(v.y);
    j.at("z").get_to(v.z);
}

void to_json(nlohmann::json& j, const sgct::vec3& v) {
    j = nlohmann::json::object();
    j["x"] = v.x;
    j["y"] = v.y;
    j["z"] = v.z;
}

void from_json(const nlohmann::json& j, sgct::vec4& v) {
    auto itX = j.find("x");
    auto itY = j.find("y");
    auto itZ = j.find("z");
    auto itW = j.find("w");

    if (itX != j.end() && itY != j.end() && itZ != j.end() && itW != j.end()) {
        itX->get_to(v.x);
        itY->get_to(v.y);
        itZ->get_to(v.z);
        itW->get_to(v.w);
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
    }
}

void to_json(nlohmann::json& j, const sgct::vec4& v) {
    j = nlohmann::json::object();
    j["x"] = v.x;
    j["y"] = v.y;
    j["z"] = v.z;
    j["w"] = v.w;
}

void from_json(const nlohmann::json& j, sgct::mat4& m) {
    std::array<double, 16> vs = j.get<std::array<double, 16>>();
    for (int i = 0; i < 16; i += 1) {
        m.values[i] = static_cast<float>(vs[i]);
    }
}

void to_json(nlohmann::json& j, const sgct::mat4& m) {
    std::array<double, 16> vs;
    for (int i = 0; i < 16; i += 1) {
        vs[i] = m.values[i];
    }
    j = vs;
}

void from_json(const nlohmann::json& j, sgct::quat& q) {
    auto itPitch = j.find("pitch");
    auto itYaw = j.find("yaw");
    auto itRoll = j.find("roll");
    if (itPitch != j.end() && itYaw != j.end() && itRoll != j.end()) {
        float x = itPitch->get<float>();
        float y = -itYaw->get<float>();
        float z = -itRoll->get<float>();

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

void to_json(nlohmann::json& j, const sgct::quat& q) {
    j = nlohmann::json::object();
    j["x"] = q.x;
    j["y"] = q.y;
    j["z"] = q.z;
    j["w"] = q.w;
}

} // namespace sgct

namespace {

constexpr int InvalidWindowIndex = -128;

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
            throw std::runtime_error(fmt::format(
                "Could not find required key '{}'", key)
            );
        }
    }
}

} // namespace

namespace sgct::config {

void from_json(const nlohmann::json& j, Scene& s) {
    parseValue(j, "offset", s.offset);
    parseValue(j, "orientation", s.orientation);
    parseValue(j, "scale", s.scale);
}

void to_json(nlohmann::json& j, const Scene& s) {
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

void from_json(const nlohmann::json& j, User& u) {
    parseValue(j, "name", u.name);
    parseValue(j, "eyeseparation", u.eyeSeparation);
    parseValue(j, "pos", u.position);
    parseValue(j, "matrix", u.transformation);

    if (auto it = j.find("orientation");  it != j.end()) {
        quat q = it->get<quat>();
        glm::mat4 m = glm::mat4_cast(glm::make_quat(&q.x));
        sgct::mat4 o;
        std::memcpy(&o, glm::value_ptr(m), sizeof(float[16]));
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

void to_json(nlohmann::json& j, const User& u) {
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

void from_json(const nlohmann::json& j, Settings& s) {
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
            throw Err(6050, fmt::format("Wrong buffer precision value {}", precision));
        }
    }

    if (auto it = j.find("display");  it != j.end()) {
        Settings::Display display;
        parseValue(*it, "swapinterval", display.swapInterval);
        parseValue(*it, "refreshrate", display.refreshRate);
        s.display = display;
    }
}

void to_json(nlohmann::json& j, const Settings& s) {
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

void from_json(const nlohmann::json& j, Capture& c) {
    parseValue(j, "path", c.path);
    if (auto it = j.find("format");  it != j.end()) {
        std::string format = it->get<std::string>();
        c.format = parseImageFormat(format);
    }

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
}

void to_json(nlohmann::json& j, const Capture& c) {
    j = nlohmann::json::object();

    if (c.path.has_value()) {
        j["path"] = *c.path;
    }

    if (c.format.has_value()) {
        switch (*c.format) {
            case Capture::Format::PNG:
                j["format"] = "png";
                break;
            case Capture::Format::TGA:
                j["format"] = "tga";
                break;
            case Capture::Format::JPG:
                j["format"] = "jpg";
                break;
        }
    }

    if (c.range.has_value()) {
        j["rangebegin"] = c.range->first;
        j["rangeend"] = c.range->last;
    }
}

void from_json(const nlohmann::json& j, Device::Sensors& s) {
    j.at("vrpnaddress").get_to(s.vrpnAddress);
    j.at("id").get_to(s.identifier);
}

void to_json(nlohmann::json& j, const Device::Sensors& s) {
    j = nlohmann::json::object();

    j["vrpnaddress"] = s.vrpnAddress;
    j["id"] = s.identifier;
}

void from_json(const nlohmann::json& j, Device::Buttons& b) {
    j.at("vrpnaddress").get_to(b.vrpnAddress);
    j.at("count").get_to(b.count);
}

void to_json(nlohmann::json& j, const Device::Buttons& b) {
    j = nlohmann::json::object();

    j["vrpnaddress"] = b.vrpnAddress;
    j["count"] = b.count;
}

void from_json(const nlohmann::json& j, Device::Axes& a) {
    j.at("vrpnaddress").get_to(a.vrpnAddress);
    j.at("count").get_to(a.count);
}

void to_json(nlohmann::json& j, const Device::Axes& a) {
    j = nlohmann::json::object();

    j["vrpnaddress"] = a.vrpnAddress;
    j["count"] = a.count;
}

void from_json(const nlohmann::json& j, Device& d) {
    parseValue(j, "name", d.name);
    parseValue(j, "sensors", d.sensors);
    parseValue(j, "buttons", d.buttons);
    parseValue(j, "axes", d.axes);
    parseValue(j, "offset", d.offset);
    parseValue(j, "matrix", d.transformation);
}

void to_json(nlohmann::json& j, const Device& d) {
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

void from_json(const nlohmann::json& j, Tracker& t) {
    if (auto it = j.find("name");  it != j.end()) {
        it->get_to(t.name);
    }
    else {
        throw Err(6070, "Tracker is missing 'name'");
    }
    parseValue(j, "devices", t.devices);
    parseValue(j, "offset", t.offset);

    if (auto it = j.find("orientation");  it != j.end()) {
        quat q = it->get<quat>();
        glm::mat4 m = glm::mat4_cast(glm::make_quat(&q.x));
        sgct::mat4 o;
        std::memcpy(&o, glm::value_ptr(m), sizeof(float[16]));
        t.transformation = o;
    }
    parseValue(j, "scale", t.scale);
    parseValue(j, "matrix", t.transformation);
}

void to_json(nlohmann::json& j, const Tracker& t) {
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

void from_json(const nlohmann::json& j, PlanarProjection::FOV& f) {
    auto itHFov = j.find("hfov");
    auto itVFov = j.find("vfov");

    auto itDown = j.find("down");
    auto itLeft = j.find("left");
    auto itRight = j.find("right");
    auto itUp = j.find("up");

    bool hasHorizontal = itHFov != j.end() || (itLeft != j.end() && itRight != j.end());
    bool hasVertical = itVFov != j.end() || (itDown != j.end() && itUp != j.end());
    if (!hasHorizontal || !hasVertical) {
        throw Err(6000, "Missing specification of field-of-view values");
    }

    // First we extract the potentially existing hFov and vFov values and **then** the
    // more specific left/right/up/down ones which would overwrite the first set
    if (itHFov != j.end()) {
        float hFov = itHFov->get<float>();
        f.left = hFov / 2.f;
        f.right = hFov / 2.f;
    }

    if (itVFov != j.end()) {
        float vFov = itVFov->get<float>();
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

void to_json(nlohmann::json& j, const PlanarProjection::FOV& f) {
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

void from_json(const nlohmann::json& j, PlanarProjection& p) {
    if (auto it = j.find("fov");  it == j.end()) {
        throw Err(6000, "Missing specification of field-of-view values");
    }

    parseValue(j, "fov", p.fov);

    parseValue(j, "distance", p.fov.distance);
    parseValue(j, "orientation", p.orientation);
    parseValue(j, "offset", p.offset);
}

void to_json(nlohmann::json& j, const PlanarProjection& p) {
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

void from_json(const nlohmann::json& j, FisheyeProjection& p) {
    parseValue(j, "fov", p.fov);

    if (auto it = j.find("quality");  it != j.end()) {
        std::string quality = it->get<std::string>();
        p.quality = cubeMapResolutionForQuality(quality);
    }

    if (auto it = j.find("interpolation");  it != j.end()) {
        std::string interpolation = it->get<std::string>();
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

void to_json(nlohmann::json& j, const FisheyeProjection& p) {
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
        nlohmann::json background = nlohmann::json::object();
        background["r"] = p.background->x;
        background["g"] = p.background->y;
        background["b"] = p.background->z;
        background["a"] = p.background->w;
        j["background"] = background;
    }
}

void from_json(const nlohmann::json& j, SphericalMirrorProjection& p) {
    if (auto it = j.find("quality");  it != j.end()) {
        std::string quality = it->get<std::string>();
        p.quality = cubeMapResolutionForQuality(quality);
    }

    parseValue(j, "tilt", p.tilt);
    if (auto it = j.find("background");  it != j.end()) {
        sgct::vec4 background;
        it->at("r").get_to(background.x);
        it->at("g").get_to(background.y);
        it->at("b").get_to(background.z);
        it->at("a").get_to(background.w);
        p.background = background;
    }

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

void to_json(nlohmann::json& j, const SphericalMirrorProjection& p) {
    j = nlohmann::json::object();

    if (p.quality.has_value()) {
        j["quality"] = std::to_string(*p.quality);
    }

    if (p.tilt.has_value()) {
        j["tilt"] = *p.tilt;
    }

    if (p.background.has_value()) {
        nlohmann::json background = nlohmann::json::object();
        background["r"] = p.background->x;
        background["g"] = p.background->y;
        background["b"] = p.background->z;
        background["a"] = p.background->w;
        j["background"] = background;
    }

    nlohmann::json mesh = nlohmann::json::object();
    mesh["bottom"] = p.mesh.bottom;
    mesh["left"] = p.mesh.left;
    mesh["right"] = p.mesh.right;
    mesh["top"] = p.mesh.top;
    j["geometry"] = mesh;
}

void from_json(const nlohmann::json& j, SpoutOutputProjection& p) {
    if (auto it = j.find("quality");  it != j.end()) {
        std::string quality = it->get<std::string>();
        p.quality = cubeMapResolutionForQuality(quality);
    }

    if (auto it = j.find("drawmain");  it != j.end()) {
        p.drawMain = it->get<bool>();
    }

    if (auto it = j.find("mapping");  it != j.end()) {
        std::string mapping = it->get<std::string>();
        p.mapping = parseMapping(mapping);
    }

    parseValue(j, "mappingspoutname", p.mappingSpoutName);
    if (auto it = j.find("background");  it != j.end()) {
        sgct::vec4 background;
        it->at("r").get_to(background.x);
        it->at("g").get_to(background.y);
        it->at("b").get_to(background.z);
        it->at("a").get_to(background.w);
        p.background = background;
    }

    if (auto it = j.find("channels");  it != j.end()) {
        SpoutOutputProjection::Channels c;
        parseValue(*it, "right", c.right);
        parseValue(*it, "zleft", c.zLeft);
        parseValue(*it, "bottom", c.bottom);
        parseValue(*it, "top", c.top);
        parseValue(*it, "left", c.left);
        parseValue(*it, "zright", c.zRight);
        p.channels = c;
    }

    if (auto it = j.find("orientation");  it != j.end()) {
        sgct::vec3 orientation;
        parseValue(*it, "pitch", orientation.x);
        parseValue(*it, "yaw", orientation.y);
        parseValue(*it, "roll", orientation.z);
        p.orientation = orientation;
    }
}

void to_json(nlohmann::json& j, const SpoutOutputProjection& p) {
    j = nlohmann::json::object();

    if (p.quality.has_value()) {
        j["quality"] = std::to_string(*p.quality);
    }

    if (p.mapping.has_value()) {
        switch (*p.mapping) {
            case SpoutOutputProjection::Mapping::Fisheye:
                j["mapping"] = "fisheye";
                break;
            case SpoutOutputProjection::Mapping::Equirectangular:
                j["mapping"] = "equirectangular";
                break;
            case SpoutOutputProjection::Mapping::Cubemap:
                j["mapping"] = "cubemap";
                break;
        }
    }

    j["mappingspoutname"] = p.mappingSpoutName;

    if (p.background.has_value()) {
        nlohmann::json background = nlohmann::json::object();
        background["r"] = p.background->x;
        background["g"] = p.background->y;
        background["b"] = p.background->z;
        background["a"] = p.background->w;
        j["background"] = background;
    }

    if (p.channels.has_value()) {
        nlohmann::json channels = nlohmann::json::object();
        channels["right"] = p.channels->right;
        channels["zleft"] = p.channels->zLeft;
        channels["bottom"] = p.channels->bottom;
        channels["top"] = p.channels->top;
        channels["left"] = p.channels->left;
        channels["zright"] = p.channels->zRight;
        j["channels"] = channels;
    }

    if (p.orientation.has_value()) {
        nlohmann::json orientation = nlohmann::json::object();
        orientation["pitch"] = p.orientation->x;
        orientation["yaw"] = p.orientation->y;
        orientation["roll"] = p.orientation->z;
        j["orientation"] = orientation;
    }
}

void from_json(const nlohmann::json& j, SpoutFlatProjection& p) {
    if (auto it = j.find("width");  it != j.end()) {
        p.width = it->get<int>();
    }

    if (auto it = j.find("height");  it != j.end()) {
        p.height = it->get<int>();
    }

    if (auto it = j.find("mappingspoutname");  it != j.end()) {
        p.mappingSpoutName = it->get<std::string>();
    }

    if (auto it = j.find("drawmain");  it != j.end()) {
        p.drawMain = it->get<bool>();
    }

    if (auto it = j.find("background");  it != j.end()) {
        sgct::vec4 background;
        it->at("r").get_to(background.x);
        it->at("g").get_to(background.y);
        it->at("b").get_to(background.z);
        it->at("a").get_to(background.w);
        p.background = background;
    }

    if (auto it = j.find("planarprojection");  it != j.end()) {
        it->get_to(p.proj);
    }
}

void to_json(nlohmann::json& j, const SpoutFlatProjection& p) {
    j = nlohmann::json::object();

    if (p.width.has_value()) {
        j["width"] = std::to_string(*p.width);
    }

    if (p.height.has_value()) {
        j["height"] = std::to_string(*p.height);
    }

    j["mappingspoutname"] = p.mappingSpoutName;

    if (p.background.has_value()) {
        nlohmann::json background = nlohmann::json::object();
        background["r"] = p.background->x;
        background["g"] = p.background->y;
        background["b"] = p.background->z;
        background["a"] = p.background->w;
        j["background"] = background;
    }

    if (p.drawMain.has_value()) {
        j["drawMain"] = *p.drawMain;
    }

    j["PlanarProjection"] = p.proj;
}

void from_json(const nlohmann::json& j, CylindricalProjection& p) {
    if (auto it = j.find("quality");  it != j.end()) {
        std::string quality = it->get<std::string>();
        p.quality = cubeMapResolutionForQuality(quality);
    }

    parseValue(j, "rotation", p.rotation);
    parseValue(j, "heightoffset", p.heightOffset);
    parseValue(j, "radius", p.radius);
}

void to_json(nlohmann::json& j, const CylindricalProjection& p) {
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

void from_json(const nlohmann::json& j, EquirectangularProjection& p) {
    if (auto it = j.find("quality");  it != j.end()) {
        std::string quality = it->get<std::string>();
        p.quality = cubeMapResolutionForQuality(quality);
    }
}

void to_json(nlohmann::json& j, const EquirectangularProjection& p) {
    if (p.quality.has_value()) {
        j["quality"] = std::to_string(*p.quality);
    }
}

void from_json(const nlohmann::json& j, ProjectionPlane& p) {
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

void to_json(nlohmann::json& j, const ProjectionPlane& p) {
    j["lowerleft"] = p.lowerLeft;
    j["upperleft"] = p.upperLeft;
    j["upperright"] = p.upperRight;
}

void from_json(const nlohmann::json& j, Viewport& v) {
    parseValue(j, "user", v.user);
    if (auto it = j.find("overlay");  it != j.end()) {
        v.overlayTexture = std::filesystem::absolute(it->get<std::string>()).string();
    }
    if (auto it = j.find("blendmask");  it != j.end()) {
        v.blendMaskTexture = std::filesystem::absolute(it->get<std::string>()).string();
    }
    if (auto it = j.find("blacklevelmask");  it != j.end()) {
        v.blackLevelMaskTexture =
            std::filesystem::absolute(it->get<std::string>()).string();
    }
    if (auto it = j.find("mesh");  it != j.end()) {
        v.correctionMeshTexture =
            std::filesystem::absolute(it->get<std::string>()).string();
    }

    parseValue(j, "tracked", v.isTracked);

    if (auto it = j.find("eye");  it != j.end()) {
        std::string eye = it->get<std::string>();
        v.eye = parseEye(eye);
    }

    parseValue(j, "pos", v.position);
    parseValue(j, "size", v.size);

    std::string type;
    if (auto it = j.find("projection");  it != j.end()) {
        if (it->is_null()) {
            v.projection = sgct::config::NoProjection();
        }
        else {
            type = it->at("type").get<std::string>();
            if (type == "PlanarProjection") {
                v.projection = it->get<PlanarProjection>();
            }
            else if (type == "TextureMappedProjection") {
                v.projection = it->get<TextureMappedProjection>();
            }
            else if (type == "FisheyeProjection") {
                v.projection = it->get<FisheyeProjection>();
            }
            else if (type == "SphericalMirrorProjection") {
                v.projection = it->get<SphericalMirrorProjection>();
            }
            else if (type == "SpoutOutputProjection") {
                v.projection = it->get<SpoutOutputProjection>();
            }
            else if (type == "SpoutFlatProjection") {
                v.projection = it->get<SpoutFlatProjection>();
            }
            else if (type == "CylindricalProjection") {
                v.projection = it->get<CylindricalProjection>();
            }
            else if (type == "EquirectangularProjection") {
                v.projection = it->get<EquirectangularProjection>();
            }
            else if (type == "ProjectionPlane") {
                v.projection = it->get<ProjectionPlane>();
            }
            else {
                throw Err(6089, fmt::format("Unknown projection type '{}'", type));
            }
        }
    }

    if (type == "TextureMappedProjection") {
        if (!v.correctionMeshTexture) {
            throw Err(6110, "Missing correction mesh for TextureMappedProjection");
        }
    }
}

void to_json(nlohmann::json& j, const Viewport& v) {
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
        [](const config::SpoutOutputProjection& p) {
            nlohmann::json proj = p;
            proj["type"] = "SpoutOutputProjection";
            return proj;
        },
        [](const config::SpoutFlatProjection& p) {
            nlohmann::json proj = p;
            proj["type"] = "SpoutFlatProjection";
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

void from_json(const nlohmann::json& j, Window& w) {
    std::optional<int> id;
    parseValue(j, "id", id);
    w.id = id.value_or(InvalidWindowIndex);

    parseValue(j, "name", w.name);
    parseValue(j, "tags", w.tags);

    if (auto it = j.find("bufferbitdepth");  it != j.end()) {
        std::string bbd = it->get<std::string>();
        w.bufferBitDepth = parseBufferColorBitDepth(bbd);
    }

    parseValue(j, "fullscreen", w.isFullScreen);
    parseValue(j, "autoiconify", w.shouldAutoiconify);
    parseValue(j, "hidemousecursor", w.hideMouseCursor);
    parseValue(j, "floating", w.isFloating);
    parseValue(j, "alwaysrender", w.alwaysRender);
    parseValue(j, "hidden", w.isHidden);
    parseValue(j, "doublebuffered", w.doubleBuffered);

    parseValue(j, "msaa", w.msaa);
    parseValue(j, "fxaa", w.useFxaa);

    parseValue(j, "border", w.isDecorated);
    parseValue(j, "resizable", w.isResizable);
    parseValue(j, "mirror", w.isMirrored);
    parseValue(j, "draw2d", w.draw2D);
    parseValue(j, "draw3d", w.draw3D);
    parseValue(j, "blitwindowid", w.blitWindowId);
    parseValue(j, "monitor", w.monitor);

    if (auto it = j.find("stereo");  it != j.end()) {
        w.stereo = parseStereoType(it->get<std::string>());
    }

    parseValue(j, "pos", w.pos);
    parseValue(j, "size", w.size);
    parseValue(j, "res", w.resolution);

    parseValue(j, "viewports", w.viewports);
}

void to_json(nlohmann::json& j, const Window& w) {
    j["id"] = w.id;

    if (w.name.has_value()) {
        j["name"] = *w.name;
    }

    if (!w.tags.empty()) {
        j["tags"] = w.tags;
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

    if (w.hideMouseCursor.has_value()) {
        j["hidemousecursor"] = *w.hideMouseCursor;
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

    if (w.doubleBuffered.has_value()) {
        j["doublebuffered"] = *w.doubleBuffered;
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

    if (w.isMirrored.has_value()) {
        j["mirror"] = *w.isMirrored;
    }

    if (w.draw2D.has_value()) {
        j["draw2d"] = *w.draw2D;
    }

    if (w.draw3D.has_value()) {
        j["draw3d"] = *w.draw3D;
    }

    if (w.blitWindowId.has_value()) {
        j["blitwindowid"] = *w.blitWindowId;
    }

    if (w.monitor.has_value()) {
        j["monitor"] = *w.monitor;
    }

    if (w.stereo.has_value()) {
        j["stereo"] = toString(*w.stereo);
    }

    if (w.pos.has_value()) {
        j["pos"] = *w.pos;
    }

    j["size"] = w.size;

    if (w.resolution.has_value()) {
        j["res"] = *w.resolution;
    }

    if (!w.viewports.empty()) {
        j["viewports"] = w.viewports;
    }
}

void from_json(const nlohmann::json& j, Node& n) {
    if (auto it = j.find("address");  it != j.end()) {
        it->get_to(n.address);
    }
    else {
        throw Err(6040, "Missing field address in node");
    }

    if (auto it = j.find("port");  it != j.end()) {
        it->get_to(n.port);
    }
    else {
        throw Err(6041, "Missing field port in node");
    }

    parseValue(j, "datatransferport", n.dataTransferPort);
    parseValue(j, "swaplock", n.swapLock);

    parseValue(j, "windows", n.windows);
    for (size_t i = 0; i < n.windows.size(); i += 1) {
        if (n.windows[i].id == InvalidWindowIndex) {
            n.windows[i].id = static_cast<int>(i);
        }
    }
}

void to_json(nlohmann::json& j, const Node& n) {
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

void from_json(const nlohmann::json& j, Cluster& c) {
    if (auto it = j.find("masteraddress");  it != j.end()) {
        it->get_to(c.masterAddress);
    }
    else {
        throw Err(6084, "Cannot find master address");
    }

    parseValue(j, "threadaffinity", c.setThreadAffinity);
    parseValue(j, "debuglog", c.debugLog);
    parseValue(j, "firmsync", c.firmSync);

    parseValue(j, "scene", c.scene);
    parseValue(j, "users", c.users);
    parseValue(j, "settings", c.settings);
    parseValue(j, "capture", c.capture);

    parseValue(j, "trackers", c.trackers);
    parseValue(j, "nodes", c.nodes);
}

void to_json(nlohmann::json& j, const Cluster& c) {
    j["masteraddress"] = c.masterAddress;

    if (c.setThreadAffinity.has_value()) {
        j["threadaffinity"] = *c.setThreadAffinity;
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
}

void from_json(const nlohmann::json& j, GeneratorVersion& v) {
    if (auto it = j.find("generator");  it != j.end()) {
        parseValue(*it, "name", v.name);
        parseValue(*it, "major", v.major);
        parseValue(*it, "minor", v.minor);
    }
    else {
        throw Err(6089, "This configuration file was not generated from the window "
            "editor, and thus cannot be edited (missing field 'generator' in file)");
    }
}

void from_json(const nlohmann::json& j, Meta& m) {
    if (auto it = j.find("meta");  it != j.end()) {
        if (it->find("description") != it->end()) {
            parseValue(*it, "description", m.description);
        }
        if (it->find("name") != it->end()) {
            parseValue(*it, "name", m.name);
        }
        if (it->find("author") != it->end()) {
            parseValue(*it, "author", m.author);
        }
        if (it->find("license") != it->end()) {
            parseValue(*it, "license", m.license);
        }
        if (it->find("version") != it->end()) {
            parseValue(*it, "version", m.version);
        }
    }
}

void to_json(nlohmann::json& j, const GeneratorVersion& v) {
    j["name"] = v.name;
    j["major"] = v.major;
    j["minor"] = v.minor;
}

} // namespace sgct::config

namespace sgct {

config::Cluster readConfig(const std::string& filename,
                           const std::string additionalErrorDescription)
{
    Log::Debug(fmt::format("Parsing config '{}'", filename));
    if (filename.empty()) {
        throw Err(6080, "No configuration file provided");
    }

    std::string name = std::filesystem::absolute(filename).string();
    if (!std::filesystem::exists(name)) {
        throw Err(
            6081,
            fmt::format("Could not find configuration file: {}", name)
        );
    }

    // First save the old current working directory, set the new one
    std::filesystem::path oldPwd = std::filesystem::current_path();
    std::filesystem::path configFolder = std::filesystem::path(name).parent_path();
    if (!configFolder.empty()) {
        std::filesystem::current_path(configFolder);
    }

    // Then load the cluster
    config::Cluster cluster = [&additionalErrorDescription](std::filesystem::path path) {
        try {
            std::ifstream f(path);
            std::string contents = std::string(
                (std::istreambuf_iterator<char>(f)),
                std::istreambuf_iterator<char>()
            );
            return readJsonConfig(contents);
        }
        catch (const nlohmann::json::exception& e) {
            if (!additionalErrorDescription.empty()) {
                throw Err(
                    6082,
                    fmt::format(
                        "Importing of this configuration file failed with the "
                        "message:\n\n{}:\n\n{}",
                        additionalErrorDescription, e.what()
                    )
                );
            }
            else {
                throw Err(6082, e.what());
            }
        }
    }(name);

    // and reset the current working directory to the old value
    std::filesystem::current_path(oldPwd);

    Log::Debug(fmt::format("Config file '{}' read successfully", name));
    Log::Info(fmt::format("Number of nodes in cluster: {}", cluster.nodes.size()));

    for (size_t i = 0; i < cluster.nodes.size(); i++) {
        const config::Node& node = cluster.nodes[i];
        Log::Info(fmt::format(
            "\tNode ({}) address: {} [{}]", i, node.address, node.port
        ));
    }

    return cluster;
}

sgct::config::Cluster readJsonConfig(std::string_view configuration) {
    nlohmann::json j = nlohmann::json::parse(configuration);

    auto it = j.find("version");
    if (it == j.end()) {
        throw std::runtime_error("Missing 'version' information");
    }

    sgct::config::Cluster cluster;
    from_json(j, cluster);
    cluster.success = true;
    return cluster;
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

class custom_error_handler : public nlohmann::json_schema::basic_error_handler
{
public:
    void error(const nlohmann::json::json_pointer &ptr, const nlohmann::json &instance,
               const std::string &message) override;
    bool validationSucceeded();
    std::string& message();
private:
    std::string mErrMessage;
};

void custom_error_handler::error(const nlohmann::json::json_pointer &ptr,
                                 const nlohmann::json &instance,
                                 const std::string &message)
{
    nlohmann::json_schema::basic_error_handler::error(ptr, instance, message);
    mErrMessage = fmt::format(
        "Validation of config file failed '{}'\nat entry in JSON file: {}",
        message, instance.dump()
    );
}

bool custom_error_handler::validationSucceeded() {
    return mErrMessage.empty();
}

std::string& custom_error_handler::message() {
    return mErrMessage;
}

std::string stringifyJsonFile(const std::string& filename) {
    std::ifstream myfile;
    myfile.open(filename);
    if (myfile.fail()) {
        throw Err(6082, fmt::format("Failed to open '{}'", filename));
    }
    std::stringstream buffer;
    buffer << myfile.rdbuf();
    return buffer.str();
}

bool loadFileAndSchemaThenValidate(const std::string& config,
                                   const std::string& schema,
                                   const std::string& validationTypeExplanation)
{                                 
    Log::Debug(fmt::format("Validating config '{}' against schema '{}'", config, schema));
    if (config.empty()) {
        throw Err(6080, "No configuration file provided");
    }
    if (schema.empty()) {
        throw Err(6080, "No schema file provided");
    }
    std::string configName = std::filesystem::absolute(config).string();
    if (!std::filesystem::exists(configName)) {
        throw Err(
            6081,
            fmt::format("Could not find configuration file '{}'", configName)
        );
    }
    std::string schemaName = std::filesystem::absolute(schema).string();
    if (!std::filesystem::exists(schemaName)) {
        throw Err(
            6081,
            fmt::format("Could not find schema file '{}'", schemaName)
        );
    }
    std::filesystem::path schemaDir = std::filesystem::path(schema).parent_path();
    std::string cfgString = stringifyJsonFile(std::string(config));
    bool validationSuccessful = false;
    try {
        // The schema is defined based upon string from file
        std::string schemaString = stringifyJsonFile(schema);
        validationSuccessful = validateConfigAgainstSchema(
            cfgString,
            schemaString,
            schemaDir
        );
    }
    catch (const nlohmann::json::parse_error& e) {
        convertToSgctExceptionAndThrow(schema, validationTypeExplanation, e.what());
    }
    catch (const std::runtime_error& e) {
        convertToSgctExceptionAndThrow(schema, validationTypeExplanation, e.what());
    }
    catch (const std::exception &e) {
        convertToSgctExceptionAndThrow(schema, validationTypeExplanation, e.what());
    }
    return validationSuccessful;
}

bool validateConfigAgainstSchema(const std::string& stringifiedConfig,
                                 const std::string& stringifiedSchema,
                                 std::filesystem::path& schemaDir)
{
    nlohmann::json schemaInput = nlohmann::json::parse(stringifiedSchema);
    nlohmann::json_schema::json_validator validator(
        schemaInput,
        [&schemaDir] (const nlohmann::json_uri& id, nlohmann::json& value) {
            std::string loadPath = schemaDir.string() + std::string("/") +
                id.to_string();
            size_t lbIndex = loadPath.find("#");
            if (lbIndex != std::string::npos) {
                loadPath = loadPath.substr(0, lbIndex);
            }
            //Remove trailing spaces
            if(loadPath.length() > 0 ) {
                const size_t strEnd = loadPath.find_last_not_of(" #\t\r\n\0");
                loadPath = loadPath.substr(0, strEnd + 1);
            }
            if (std::filesystem::exists(loadPath)) {
                Log::Debug(fmt::format("Loading schema file '{}'", loadPath));
                std::string newSchema = stringifyJsonFile(loadPath);
                value = nlohmann::json::parse(newSchema);
            }
            else {
                throw Err(
                    6081,
                    fmt::format("Could not find schema file to load: {}", loadPath)
                );
            }
        }
    );
    nlohmann::json sgct_cfg = nlohmann::json::parse(stringifiedConfig);
    validator.validate(sgct_cfg);
    return true;
}

[[ noreturn ]] void convertToSgctExceptionAndThrow(const std::string& schema,
                                             const std::string& validationTypeExplanation,
                                                      const std::string& exceptionMessage)
{
    throw Err(
        6089,
        fmt::format("Checking this configuration file against schema '{}' failed.\n\n"
            "{}.\n\nSchema validator provided the following error message:\n\n{}",
            schema, validationTypeExplanation, exceptionMessage
        )
    );
}

sgct::config::GeneratorVersion readJsonGeneratorVersion(const std::string& configuration) {
    nlohmann::json j = nlohmann::json::parse(stringifyJsonFile(configuration));
    auto it = j.find("version");
    if (it == j.end()) {
        throw std::runtime_error("Missing 'version' information");
    }
    sgct::config::GeneratorVersion genVersion;
    from_json(j, genVersion);
    return genVersion;
}

sgct::config::GeneratorVersion readConfigGenerator(const std::string& filename) {
    std::string name = std::filesystem::absolute(filename).string();
    if (!std::filesystem::exists(name)) {
        throw Err(
            6081,
            fmt::format("Could not find configuration file '{}'", name)
        );
    }

    config::GeneratorVersion genVersion = [](std::filesystem::path path) {
        if (path.extension() == ".json") {
            try {
                std::ifstream f(path);
                return readJsonGeneratorVersion(path.string());
            }
            catch (const std::runtime_error& e) {
                throw Err(6082, e.what());
            }
            catch (const nlohmann::json::exception& e) {
                throw Err(6082, e.what());
            }
        }
        else {
            throw Err(
                6088,
                fmt::format("Unsupported file extension '{}'", path.extension())
            );
        }
    }(name);

    Log::Debug(fmt::format(
        "Config file '{}' read for generator version '{}' version {}.{}",
        name, genVersion.name, genVersion.major, genVersion.minor
    ));

    return genVersion;
}

sgct::config::Meta readMeta(const std::string& filename) {
    assert(std::filesystem::path(filename).extension() == ".json");

    std::filesystem::path name = std::filesystem::absolute(filename);
    if (!std::filesystem::exists(name)) {
        throw Err(
            6081,
            fmt::format("Could not find configuration file '{}'", name)
        );
    }

    try {
        std::ifstream f(name);
        nlohmann::json j = nlohmann::json::parse(stringifyJsonFile(name.string()));
        if (auto it = j.find("meta");  it != j.end()) {
            sgct::config::Meta meta;
            from_json(j, meta);
            return meta;
        }
        else {
            return sgct::config::Meta();
        }
    }
    catch (const std::runtime_error& e) {
        throw Err(6082, e.what());
    }
    catch (const nlohmann::json::exception& e) {
        throw Err(6082, e.what());
    }
}

} // namespace sgct
