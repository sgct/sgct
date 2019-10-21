/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/readconfig.h>

#include <sgct/config.h>
#include <sgct/messagehandler.h>
#include <sgct/helpers/stringfunctions.h>
#include <glm/gtc/type_ptr.hpp>
#include <tinyxml2.h>
#include <algorithm>
#include <optional>
#include <sstream>
#include <unordered_map>

namespace {
    glm::quat parseOrientationNode(tinyxml2::XMLElement& element) {
        float x = 0.f;
        float y = 0.f;
        float z = 0.f;

        bool eulerMode = false;
        bool quatMode = false;

        glm::quat quat;

        float value;
        if (element.QueryFloatAttribute("w", &value) == tinyxml2::XML_NO_ERROR) {
            quat.w = value;
            quatMode = true;
        }

        if (element.QueryFloatAttribute("y", &value) == tinyxml2::XML_NO_ERROR) {
            y = value;
            eulerMode = true;
        }

        if (element.QueryFloatAttribute("yaw", &value) == tinyxml2::XML_NO_ERROR) {
            y = -value;
        }

        if (element.QueryFloatAttribute("heading", &value) == tinyxml2::XML_NO_ERROR) {
            y = -value;
        }

        if (element.QueryFloatAttribute("azimuth", &value) == tinyxml2::XML_NO_ERROR) {
            y = -value;
        }

        if (element.QueryFloatAttribute("x", &value) == tinyxml2::XML_NO_ERROR) {
            x = value;
            eulerMode = true;
        }

        if (element.QueryFloatAttribute("pitch", &value) == tinyxml2::XML_NO_ERROR) {
            x = value;
        }

        if (element.QueryFloatAttribute("elevation", &value) == tinyxml2::XML_NO_ERROR) {
            x = value;
        }

        if (element.QueryFloatAttribute("z", &value) == tinyxml2::XML_NO_ERROR) {
            z = value;
            eulerMode = true;
        }

        if (element.QueryFloatAttribute("roll", &value) == tinyxml2::XML_NO_ERROR) {
            z = -value;
        }

        if (element.QueryFloatAttribute("bank", &value) == tinyxml2::XML_NO_ERROR) {
            z = -value;
        }

        if (quatMode) {
            quat.x = x;
            quat.y = y;
            quat.z = z;
        }
        else {
            if (eulerMode) {
                quat = glm::rotate(quat, glm::radians(x), glm::vec3(1.f, 0.f, 0.f));
                quat = glm::rotate(quat, glm::radians(y), glm::vec3(0.f, 1.f, 0.f));
                quat = glm::rotate(quat, glm::radians(z), glm::vec3(0.f, 0.f, 1.f));
            }
            else {
                quat = glm::rotate(quat, glm::radians(y), glm::vec3(0.f, 1.f, 0.f));
                quat = glm::rotate(quat, glm::radians(x), glm::vec3(1.f, 0.f, 0.f));
                quat = glm::rotate(quat, glm::radians(z), glm::vec3(0.f, 0.f, 1.f));
            }
        }

        return quat;
    }

    sgct::config::Window::StereoMode getStereoType(const std::string& type) {
        if (type == "none" || type == "no_stereo") {
            return sgct::config::Window::StereoMode::NoStereo;
        }
        else if (type == "active" || type == "quadbuffer") {
            return sgct::config::Window::StereoMode::Active;
        }
        else if (type == "checkerboard") {
            return sgct::config::Window::StereoMode::Checkerboard;
        }
        else if (type == "checkerboard_inverted") {
            return sgct::config::Window::StereoMode::CheckerboardInverted;
        }
        else if (type == "anaglyph_red_cyan") {
            return sgct::config::Window::StereoMode::AnaglyphRedCyan;
        }
        else if (type == "anaglyph_amber_blue") {
            return sgct::config::Window::StereoMode::AnaglyphAmberBlue;
        }
        else if (type == "anaglyph_wimmer") {
            return sgct::config::Window::StereoMode::AnaglyphRedCyanWimmer;
        }
        else if (type == "vertical_interlaced") {
            return sgct::config::Window::StereoMode::VerticalInterlaced;
        }
        else if (type == "vertical_interlaced_inverted") {
            return sgct::config::Window::StereoMode::VerticalInterlacedInverted;
        }
        else if (type == "test" || type == "dummy") {
            return sgct::config::Window::StereoMode::Dummy;
        }
        else if (type == "side_by_side") {
            return sgct::config::Window::StereoMode::SideBySide;
        }
        else if (type == "side_by_side_inverted") {
            return sgct::config::Window::StereoMode::SideBySideInverted;
        }
        else if (type == "top_bottom") {
            return sgct::config::Window::StereoMode::TopBottom;
        }
        else if (type == "top_bottom_inverted") {
            return sgct::config::Window::StereoMode::TopBottomInverted;
        }

        sgct::MessageHandler::printError("Unknown stereo mode %s", type.c_str());
        return sgct::config::Window::StereoMode::NoStereo;
    }

    sgct::config::Window::ColorBitDepth getBufferColorBitDepth(const std::string& type) {
        if (type == "8") {
            return sgct::config::Window::ColorBitDepth::Depth8;
        }
        else if (type == "16") {
            return sgct::config::Window::ColorBitDepth::Depth16;
        }

        else if (type == "16f") {
            return sgct::config::Window::ColorBitDepth::Depth16Float;
        }
        else if (type == "32f") {
            return sgct::config::Window::ColorBitDepth::Depth32Float;
        }

        else if (type == "16i") {
            return sgct::config::Window::ColorBitDepth::Depth16Int;
        }
        else if (type == "32i") {
            return sgct::config::Window::ColorBitDepth::Depth32Int;
        }

        else if (type == "16ui") {
            return sgct::config::Window::ColorBitDepth::Depth16UInt;
        }
        else if (type == "32ui") {
            return sgct::config::Window::ColorBitDepth::Depth32UInt;
        }

        sgct::MessageHandler::printError("Unknown color bit depth %s", type.c_str());
        return sgct::config::Window::ColorBitDepth::Depth8;
    }

    int cubeMapResolutionForQuality(const std::string& quality) {
        static const std::unordered_map<std::string, int> Map = {
            { "low",     256 },
            { "256",     256 },
            { "medium",  512 },
            { "512",     512 },
            { "high",   1024 },
            { "1k",     1024 },
            { "1024",   1024 },
            { "1.5k",   1536 },
            { "1536",   1536 },
            { "2k",     2048 },
            { "2048",   2048 },
            { "4k",     4096 },
            { "4096",   4096 },
            { "8k",     8192 },
            { "8192",   8192 },
            { "16k",   16384 },
            { "16384", 16384 },
        };

        const auto it = Map.find(quality);
        return (it != Map.cend()) ? it->second : -1;
    }

    std::optional<glm::ivec2> parseValueIVec2(const tinyxml2::XMLElement& e) {
        glm::ivec2 value;
        tinyxml2::XMLError xe = e.QueryIntAttribute("x", &value[0]);
        tinyxml2::XMLError ye = e.QueryIntAttribute("y", &value[1]);
        if (xe == tinyxml2::XML_NO_ERROR && ye == tinyxml2::XML_NO_ERROR) {
            return value;
        }
        else {
            return std::nullopt;
        }
    }

    std::optional<glm::vec2> parseValueVec2(const tinyxml2::XMLElement& e) {
        glm::vec2 value;
        tinyxml2::XMLError xe = e.QueryFloatAttribute("x", &value[0]);
        tinyxml2::XMLError ye = e.QueryFloatAttribute("y", &value[1]);
        if (xe == tinyxml2::XML_NO_ERROR && ye == tinyxml2::XML_NO_ERROR) {
            return value;
        }
        else {
            return std::nullopt;
        }
    }

    std::optional<glm::vec3> parseValueVec3(const tinyxml2::XMLElement& e) {
        using namespace tinyxml2;

        glm::vec3 value;
        tinyxml2::XMLError xe = e.QueryFloatAttribute("x", &value[0]);
        tinyxml2::XMLError ye = e.QueryFloatAttribute("y", &value[1]);
        tinyxml2::XMLError ze = e.QueryFloatAttribute("z", &value[2]);
        if (xe == XML_NO_ERROR && ye == XML_NO_ERROR && ze == XML_NO_ERROR) {
            return value;
        }
        else {
            return std::nullopt;
        }
    }

    std::optional<glm::vec4> parseValueColor(const tinyxml2::XMLElement& e) {
        using namespace tinyxml2;

        glm::vec4 value;
        XMLError re = e.QueryFloatAttribute("r", &value[0]);
        XMLError ge = e.QueryFloatAttribute("g", &value[1]);
        XMLError be = e.QueryFloatAttribute("b", &value[2]);
        XMLError ae = e.QueryFloatAttribute("a", &value[3]);
        if (re == XML_NO_ERROR && ge == XML_NO_ERROR && be == XML_NO_ERROR &&
            ae == XML_NO_ERROR)
        {
            return value;
        }
        else {
            return std::nullopt;
        }
    }

    std::optional<glm::quat> parseValueQuat(const tinyxml2::XMLElement& e) {
        glm::quat q;
        // Yes, this order is correct;  the constructor takes w,x,y,z but the
        // layout in memory is x,y,z,w
        tinyxml2::XMLError we = e.QueryFloatAttribute("w", &q[3]);
        tinyxml2::XMLError xe = e.QueryFloatAttribute("x", &q[0]);
        tinyxml2::XMLError ye = e.QueryFloatAttribute("y", &q[1]);
        tinyxml2::XMLError ze = e.QueryFloatAttribute("z", &q[2]);
        if (we == tinyxml2::XML_NO_ERROR && xe == tinyxml2::XML_NO_ERROR &&
            ye == tinyxml2::XML_NO_ERROR && ze == tinyxml2::XML_NO_ERROR)
        {
            return q;
        }
        else {
            return std::nullopt;
        }
    }

    std::optional<glm::mat4> parseValueMat4(const tinyxml2::XMLElement& e) {
        float value[16];
        tinyxml2::XMLError err[16] = {
            e.QueryFloatAttribute("x0", &value[0]),
            e.QueryFloatAttribute("y0", &value[1]),
            e.QueryFloatAttribute("z0", &value[2]),
            e.QueryFloatAttribute("w0", &value[3]),
            e.QueryFloatAttribute("x1", &value[4]),
            e.QueryFloatAttribute("y1", &value[5]),
            e.QueryFloatAttribute("z1", &value[6]),
            e.QueryFloatAttribute("w1", &value[7]),
            e.QueryFloatAttribute("x2", &value[8]),
            e.QueryFloatAttribute("y2", &value[9]),
            e.QueryFloatAttribute("z2", &value[10]),
            e.QueryFloatAttribute("w2", &value[11]),
            e.QueryFloatAttribute("x3", &value[12]),
            e.QueryFloatAttribute("y3", &value[13]),
            e.QueryFloatAttribute("z3", &value[14]),
            e.QueryFloatAttribute("w3", &value[15])
        };
        if (err[0] == tinyxml2::XML_NO_ERROR && err[1] == tinyxml2::XML_NO_ERROR &&
            err[2] == tinyxml2::XML_NO_ERROR && err[3] == tinyxml2::XML_NO_ERROR &&
            err[4] == tinyxml2::XML_NO_ERROR && err[5] == tinyxml2::XML_NO_ERROR &&
            err[6] == tinyxml2::XML_NO_ERROR && err[7] == tinyxml2::XML_NO_ERROR &&
            err[8] == tinyxml2::XML_NO_ERROR && err[9] == tinyxml2::XML_NO_ERROR &&
            err[10] == tinyxml2::XML_NO_ERROR && err[11] == tinyxml2::XML_NO_ERROR &&
            err[12] == tinyxml2::XML_NO_ERROR && err[13] == tinyxml2::XML_NO_ERROR &&
            err[14] == tinyxml2::XML_NO_ERROR && err[15] == tinyxml2::XML_NO_ERROR)
        {
            const glm::mat4 mat = glm::make_mat4(value);
            return mat;
        }
        else {
            return std::nullopt;
        }
    }

    template <typename T>
    std::optional<T> parseValue(const tinyxml2::XMLElement& e, const char* name) {
        if (e.Attribute(name) == nullptr) {
            // The attribute does not exist, in which case we want to silently return
            return std::nullopt;
        }

        T value;
        tinyxml2::XMLError err;
        if constexpr (std::is_same_v<T, float>) {
            err = e.QueryFloatAttribute(name, &value);
        }
        else if constexpr (std::is_same_v<T, bool>) {
            err = e.QueryBoolAttribute(name, &value);
        }
        else if constexpr (std::is_same_v<T, int>) {
            err = e.QueryIntAttribute(name, &value);
        }
        else if constexpr (std::is_same_v<T, unsigned int>) {
            err = e.QueryUnsignedAttribute(name, &value);
        }
        else if constexpr (std::is_same_v<T, double>) {
            err = e.QueryDoubleAttribute(name, &value);
        }

        if (err == tinyxml2::XML_NO_ERROR) {
            return value;
        }
        else {
            sgct::MessageHandler::printError("Error extracting value '%s'", name);
            return std::nullopt;
        }
    }

    sgct::config::PlanarProjection parsePlanarProjection(tinyxml2::XMLElement& element) {
        using namespace tinyxml2;

        bool foundFov = false;

        sgct::config::PlanarProjection proj;
        XMLElement* subElement = element.FirstChildElement();
        while (subElement) {
            tinyxml2::XMLElement& e = *subElement;
            std::string_view val = e.Value();

            if (val == "FOV") {
                foundFov = true;
                std::optional<float> down = parseValue<float>(e, "down");
                std::optional<float> left = parseValue<float>(e, "left");
                std::optional<float> right = parseValue<float>(e, "right");
                std::optional<float> up = parseValue<float>(e, "up");

                if (down && left && right && up) {
                    // The negative signs here were lifted up from the viewport class. I
                    // think it is nicer to store them in negative values and consider the
                    // fact that the down and left fovs are inverted to be a detail of the
                    // XML specification rather than SGCT
                    proj.fov.down = -*down;
                    proj.fov.left = -*left;
                    proj.fov.right = *right;
                    proj.fov.up = *up;
                }
                else {
                    throw std::runtime_error("Failed to parse planar projection FOV");
                }
                proj.fov.distance = parseValue<float>(e, "distance");
            }
            else if (val == "Orientation") {
                proj.orientation = parseOrientationNode(e);
            }
            else if (val == "Offset") {
                proj.offset = parseValueVec3(e);
            }

            subElement = e.NextSiblingElement();
        }

        if (!foundFov) {
            throw std::runtime_error("Missing specification of field-of-view values");
        }

        return proj;
    }

    sgct::config::FisheyeProjection parseFisheyeProjection(tinyxml2::XMLElement& element)
    {
        sgct::config::FisheyeProjection proj;

        proj.fov = parseValue<float>(element, "fov");
        if (element.Attribute("quality")) {
            const int res = cubeMapResolutionForQuality(element.Attribute("quality"));
            proj.quality = res;
        }
        if (element.Attribute("method")) {
            std::string_view method = element.Attribute("method");
            proj.method = method == "five_face_cube" ?
                sgct::config::FisheyeProjection::Method::FiveFace :
                sgct::config::FisheyeProjection::Method::FourFace;
        }
        if (element.Attribute("interpolation")) {
            std::string_view interpolation = element.Attribute("interpolation");
            proj.interpolation = interpolation == "cubic" ?
                sgct::config::FisheyeProjection::Interpolation::Cubic :
                sgct::config::FisheyeProjection::Interpolation::Linear;
        }
        proj.diameter = parseValue<float>(element, "diameter");
        proj.tilt = parseValue<float>(element, "tilt");

        tinyxml2::XMLElement* subElement = element.FirstChildElement();
        while (subElement) {
            tinyxml2::XMLElement& e = *subElement;
            std::string_view val = e.Value();

            if (val == "Crop") {
                sgct::config::FisheyeProjection::Crop crop;
                if (std::optional<float> v = parseValue<float>(e, "left"); v) {
                    crop.left = *v;
                }
                if (std::optional<float> v = parseValue<float>(e, "right"); v) {
                    crop.right = *v;
                }
                if (std::optional<float> v = parseValue<float>(e, "bottom"); v) {
                    crop.bottom = *v;
                }
                if (std::optional<float> v = parseValue<float>(e, "top"); v) {
                    crop.top = *v;
                }
                proj.crop = crop;
            }
            else if (val == "Offset") {
                proj.offset = parseValueVec3(*subElement);
            }
            if (val == "Background") {
                proj.background = parseValueColor(*subElement);
            }

            subElement = subElement->NextSiblingElement();
        }

        return proj;
    }

    sgct::config::SphericalMirrorProjection parseSphericalMirrorProjection(
                                                            tinyxml2::XMLElement& element)
    {
        sgct::config::SphericalMirrorProjection proj;
        if (element.Attribute("quality")) {
            proj.quality = cubeMapResolutionForQuality(element.Attribute("quality"));
        }

        proj.tilt = parseValue<float>(element, "tilt");

        tinyxml2::XMLElement* subElement = element.FirstChildElement();
        while (subElement) {
            tinyxml2::XMLElement& e = *subElement;
            std::string_view val = e.Value();

            if (val == "Background") {
                proj.background = parseValueColor(e);
            }
            else if (val == "Geometry") {
                if (e.Attribute("bottom")) {
                    proj.mesh.bottom = e.Attribute("bottom");
                }

                if (e.Attribute("left")) {
                    proj.mesh.left = e.Attribute("left");
                }

                if (e.Attribute("right")) {
                    proj.mesh.right = e.Attribute("right");
                }

                if (e.Attribute("top")) {
                    proj.mesh.top = e.Attribute("top");
                }
            }

            subElement = e.NextSiblingElement();
        }
        return proj;
    }

    sgct::config::SpoutOutputProjection parseSpoutOutputProjection(
                                                            tinyxml2::XMLElement& element)
    {
        sgct::config::SpoutOutputProjection proj;

        if (element.Attribute("quality")) {
            proj.quality = cubeMapResolutionForQuality(element.Attribute("quality"));
        }
        if (element.Attribute("mapping")) {
            using namespace sgct::config;
            std::string_view val = element.Attribute("mapping");
            if (val == "fisheye") {
                proj.mapping = SpoutOutputProjection::Mapping::Fisheye;
            }
            else if (val == "equirectangular") {
                proj.mapping = SpoutOutputProjection::Mapping::Equirectangular;
            }
            else if (val == "cubemap") {
                proj.mapping = SpoutOutputProjection::Mapping::Cubemap;
            }
            else {
                proj.mapping = SpoutOutputProjection::Mapping::Cubemap;
            }
        }
        if (element.Attribute("mappingSpoutName")) {
            proj.mappingSpoutName = element.Attribute("mappingSpoutName");
        }

        tinyxml2::XMLElement* subElement = element.FirstChildElement();
        while (subElement) {
            tinyxml2::XMLElement& e = *subElement;
            std::string_view val = e.Value();

            if (val == "Background") {
                proj.background = parseValueColor(e);
            }

            if (val == "Channels") {
                sgct::config::SpoutOutputProjection::Channels c;
                c.right = *parseValue<bool>(e, "Right");
                c.zLeft = *parseValue<bool>(e, "zLeft");
                c.bottom = *parseValue<bool>(e, "Bottom");
                c.top = *parseValue<bool>(e, "Top");
                c.left = *parseValue<bool>(e, "Left");
                c.zRight = *parseValue<bool>(e, "zRight");
                proj.channels = c;
            }

            if (val == "RigOrientation") {
                glm::vec3 orientation = glm::vec3(
                    *parseValue<float>(e, "pitch"),
                    *parseValue<float>(e, "yaw"),
                    *parseValue<float>(e, "roll")
                );
                proj.orientation = orientation;
            }

            subElement = e.NextSiblingElement();
        }

        return proj;
    }

    sgct::config::ProjectionPlane parseProjectionPlane(tinyxml2::XMLElement& element) {
        tinyxml2::XMLElement* elem = element.FirstChildElement();
        // There should be exactly three positions in this child
        tinyxml2::XMLElement* c1 = elem;
        tinyxml2::XMLElement* c2 = elem->NextSiblingElement();
        tinyxml2::XMLElement* c3 = c2->NextSiblingElement();
        if (!(c1 && c2 && c3)) {
            throw std::runtime_error(
                "Failed to parse ProjectionPlane coordinates. Missing XML children"
            );
        }
        std::optional<glm::vec3> p1 = parseValueVec3(*c1);
        std::optional<glm::vec3> p2 = parseValueVec3(*c2);
        std::optional<glm::vec3> p3 = parseValueVec3(*c3);
        if (!(p1 && p2 && p3)) {
            throw std::runtime_error(
                "Failed to parse ProjectionPlane coordinates. Type error"
            );
        }

        sgct::config::ProjectionPlane proj;
        proj.lowerLeft = *p1;
        proj.upperLeft = *p2;
        proj.upperRight = *p3;
        return proj;
    }

    sgct::config::Viewport parseViewport(tinyxml2::XMLElement& element) {
        sgct::config::Viewport viewport;
        if (element.Attribute("user")) {
            viewport.user = element.Attribute("user");
        }

        if (element.Attribute("name")) {
            viewport.name = element.Attribute("name");
        }

        if (element.Attribute("overlay")) {
            viewport.overlayTexture = element.Attribute("overlay");
        }

        if (element.Attribute("mask")) {
            viewport.blendMaskTexture = element.Attribute("mask");
        }

        if (element.Attribute("BlendMask")) {
            viewport.blendMaskTexture = element.Attribute("BlendMask");
        }

        if (element.Attribute("BlackLevelMask")) {
            viewport.blendLevelMaskTexture = element.Attribute("BlackLevelMask");
        }

        if (element.Attribute("mesh")) {
            viewport.correctionMeshTexture = element.Attribute("mesh");
        }

        if (element.Attribute("hint")) {
            viewport.meshHint = element.Attribute("hint");
        }

        viewport.isTracked = parseValue<bool>(element, "tracked");

        // get eye if set
        if (element.Attribute("eye")) {
            std::string_view eye = element.Attribute("eye");
            if (eye == "center") {
                viewport.eye = sgct::config::Viewport::Eye::Mono;
            }
            else if (eye == "left") {
                viewport.eye = sgct::config::Viewport::Eye::StereoLeft;
            }
            else if (eye == "right") {
                viewport.eye = sgct::config::Viewport::Eye::StereoRight;
            }
        }

        tinyxml2::XMLElement* subElement = element.FirstChildElement();
        while (subElement) {
            tinyxml2::XMLElement& e = *subElement;
            std::string_view val = e.Value();
            if (val == "Pos") {
                std::optional<glm::vec2> pos = parseValueVec2(e);
                if (pos) {
                    viewport.position = *pos;
                }
                else {
                    throw std::runtime_error("Viewport: Failed to parse position");
                }
            }
            else if (val == "Size") {
                std::optional<glm::vec2> size = parseValueVec2(e);
                if (size) {
                    viewport.size = *size;
                }
                else {
                    throw std::runtime_error("Viewport: Failed to parse size");
                }
            }
            else if (val == "PlanarProjection") {
                viewport.projection = parsePlanarProjection(e);
            }
            else if (val == "FisheyeProjection") {
                viewport.projection = parseFisheyeProjection(e);
            }
            else if (val == "SphericalMirrorProjection") {
                viewport.projection = parseSphericalMirrorProjection(e);
            }
            else if (val == "SpoutOutputProjection") {
                viewport.projection = parseSpoutOutputProjection(e);
            }
            else if (val == "Viewplane" || val == "Projectionplane") {
                viewport.projection = parseProjectionPlane(e);
            }

            subElement = e.NextSiblingElement();
        }

        return viewport;
    }


    sgct::config::Scene parseScene(tinyxml2::XMLElement& element) {
        using namespace sgct::core;

        sgct::config::Scene scene;

        tinyxml2::XMLElement* child = element.FirstChildElement();
        while (child) {
            tinyxml2::XMLElement& e = *child;
            std::string_view childVal = e.Value();

            if (childVal == "Offset") {
                scene.offset = parseValueVec3(e);
            }
            else if (childVal == "Orientation") {
                scene.orientation = parseOrientationNode(e);
            }
            else if (childVal == "Scale") {
                scene.scale = parseValue<float>(e, "value");
            }

            child = e.NextSiblingElement();
        }

        return scene;
    }

    sgct::config::Window parseWindow(tinyxml2::XMLElement& element,
                                     const std::string& filename)
    {
        sgct::config::Window window;

        if (element.Attribute("name")) {
            window.name = element.Attribute("name");
        }

        if (element.Attribute("tags")) {
            std::string tags = element.Attribute("tags");
            std::vector<std::string> t = sgct::helpers::split(tags, ',');
            window.tags = std::move(t);
        }

        if (element.Attribute("bufferBitDepth")) {
            window.bufferBitDepth = getBufferColorBitDepth(
                element.Attribute("bufferBitDepth")
            );
        }

        // compatibility with older versions
        if (element.Attribute("fullScreen")) {
            window.isFullScreen = parseValue<bool>(element, "fullScreen");
        }

        if (element.Attribute("fullscreen")) {
            window.isFullScreen = parseValue<bool>(element, "fullscreen");
        }

        window.isFloating = parseValue<bool>(element, "floating");
        window.alwaysRender = parseValue<bool>(element, "alwaysRender");
        window.isHidden = parseValue<bool>(element, "hidden");
        window.doubleBuffered = parseValue<bool>(element, "dbuffered");
        window.gamma = parseValue<float>(element, "gamma");
        window.contrast = parseValue<float>(element, "contrast");
        window.brightness = parseValue<float>(element, "brightness");
        window.msaa = parseValue<int>(element, "numberOfSamples");

        if (element.Attribute("numberOfSamples")) {
            window.msaa = parseValue<int>(element, "numberOfSamples");
        }
        if (element.Attribute("msaa")) {
            window.msaa = parseValue<int>(element, "msaa");
        }
        if (element.Attribute("MSAA")) {
            window.msaa = parseValue<int>(element, "MSAA");
        }
        window.hasAlpha = parseValue<bool>(element, "alpha");

        if (element.Attribute("fxaa")) {
            window.useFxaa = parseValue<bool>(element, "fxaa");
        }
        if (element.Attribute("FXAA")) {
            window.useFxaa = parseValue<bool>(element, "FXAA");
        }

        window.isDecorated = parseValue<bool>(element, "decorated");
        window.hasBorder = parseValue<bool>(element, "border");
        window.draw2D = parseValue<bool>(element, "draw2D");
        window.draw3D = parseValue<bool>(element, "draw3D");
        window.blitPreviousWindow = parseValue<bool>(
            element,
            "copyPreviousWindowToCurrentWindow"
        );
        window.monitor = parseValue<int>(element, "monitor");

        if (element.Attribute("mpcdi")) {
            std::string path;
            const size_t lastSlashPos = filename.find_last_of("/");
            if (lastSlashPos != std::string::npos) {
                path = filename.substr(0, lastSlashPos) + "/";
            }
            path += element.Attribute("mpcdi");
            std::replace(path.begin(), path.end(), '\\', '/');
            window.mpcdi = path;
        }

        tinyxml2::XMLElement* child = element.FirstChildElement();
        while (child) {
            tinyxml2::XMLElement& e = *child;
            std::string_view childVal = e.Value();

            if (childVal == "Stereo") {
                window.stereo = getStereoType(e.Attribute("type"));
            }
            else if (childVal == "Pos") {
                window.pos = parseValueIVec2(e);
            }
            else if (childVal == "Size") {
                std::optional<glm::ivec2> s = parseValueIVec2(e);
                if (s) {
                    window.size = *s;
                }
                else {
                    throw std::runtime_error("Could not parse window size");
                }
            }
            else if (childVal == "Res") {
                window.resolution = parseValueIVec2(e);
            }
            else if (childVal == "Viewport") {
                window.viewports.push_back(parseViewport(e));
            }
            child = e.NextSiblingElement();
        }

        return window;
    }

    sgct::config::Node parseNode(tinyxml2::XMLElement& element,
                                 const std::string& xmlFileName)
    {
        sgct::config::Node node;
        if (element.Attribute("address")) {
            node.address = element.Attribute("address");
        }
        else {
            throw std::runtime_error("Missing field address in node");
        }
        if (element.Attribute("port")) {
            node.port = *parseValue<int>(element, "port");
        }
        else {
            throw std::runtime_error("Missing field port in node");
        }
        if (element.Attribute("name")) {
            node.name = element.Attribute("name");
        }
        node.dataTransferPort = parseValue<int>(element, "dataTransferPort");
        node.swapLock = parseValue<bool>(element, "swapLock");

        tinyxml2::XMLElement* child = element.FirstChildElement();
        while (child) {
            std::string_view childVal = child->Value();
            if (childVal == "Window") {
                sgct::config::Window window = parseWindow(*child, xmlFileName);
                node.windows.push_back(window);
            }
            child = child->NextSiblingElement();
        }

        return node;
    }

    sgct::config::User parseUser(tinyxml2::XMLElement& element) {
        sgct::config::User user;
        if (element.Attribute("name")) {
            user.name = element.Attribute("name");
        }
        user.eyeSeparation = parseValue<float>(element, "eyeSeparation");

        tinyxml2::XMLElement* child = element.FirstChildElement();
        while (child) {
            tinyxml2::XMLElement& e = *child;
            std::string_view childVal = e.Value();
            if (childVal == "Pos") {
                user.position = parseValueVec3(e);
            }
            else if (childVal == "Orientation") {
                user.transformation = glm::mat4_cast(parseOrientationNode(e));
            }
            else if (childVal == "Matrix") {
                user.transformation = parseValueMat4(e);
                if (user.transformation) {
                    std::optional<bool> transpose = parseValue<bool>(e, "transpose");
                    if (transpose && *transpose) {
                        user.transformation = glm::transpose(*user.transformation);
                    }
                }
            }
            else if (childVal == "Tracking") {
                sgct::config::User::Tracking tracking;
                tracking.tracker = e.Attribute("tracker");
                tracking.device = e.Attribute("device");
                user.tracking = tracking;
            }
            child = e.NextSiblingElement();
        }

        return user;
    }


    sgct::config::Settings parseSettings(tinyxml2::XMLElement& element) {
        sgct::config::Settings settings;

        tinyxml2::XMLElement* elem = element.FirstChildElement();
        while (elem) {
            tinyxml2::XMLElement& e = *elem;
            std::string_view val = e.Value();

            if (val == "DepthBufferTexture") {
                settings.useDepthTexture = parseValue<bool>(e, "value");
            }
            else if (val == "NormalTexture") {
                settings.useNormalTexture = parseValue<bool>(e, "value");
            }
            else if (val == "PositionTexture") {
                settings.usePositionTexture = parseValue<bool>(e, "value");
            }
            else if (val == "Precision") {
                std::optional<float> f = parseValue<float>(e, "float");
                if (f && *f == 16.f) {
                    settings.bufferFloatPrecision =
                        sgct::config::Settings::BufferFloatPrecision::Float16Bit;
                }
                else if (f && *f == 32) {
                    settings.bufferFloatPrecision =
                        sgct::config::Settings::BufferFloatPrecision::Float32Bit;
                }
                else if (f) {
                    throw std::runtime_error(
                        "Wrong buffer precision value " + std::to_string(*f) +
                        ". Must be 16 or 32"
                    );
                }
                else {
                    throw std::runtime_error("Wrong buffer precision value type");
                }
            }
            else if (val == "Display") {
                sgct::config::Settings::Display display;
                display.swapInterval = parseValue<int>(e, "swapInterval");
                display.refreshRate = parseValue<int>(e, "refreshRate");
                display.maintainAspectRatio = parseValue<bool>(
                    e,
                    "tryMaintainAspectRatio"
                );
                display.exportWarpingMeshes = parseValue<bool>(e, "exportWarpingMeshes");
                settings.display = display;
            }
            else if (val == "OSDText") {
                sgct::config::Settings::OSDText osdText;
                if (elem->Attribute("name")) {
                    osdText.name = e.Attribute("name");
                }
                if (elem->Attribute("path")) {
                    osdText.path = e.Attribute("path");
                }
                osdText.size = parseValue<int>(e, "size");
                osdText.xOffset = parseValue<float>(e, "xOffset");
                osdText.yOffset = parseValue<float>(e, "yOffset");
                settings.osdText = osdText;
            }
            else if (val == "FXAA") {
                sgct::config::Settings::FXAA fxaa;
                fxaa.offset = parseValue<float>(e, "offset");
                fxaa.trim = parseValue<float>(e, "trim");
                settings.fxaa = fxaa;
            }

            elem = e.NextSiblingElement();
        }

        return settings;
    }

    sgct::config::Capture parseCapture(tinyxml2::XMLElement& element) {
        sgct::config::Capture res;
        if (element.Attribute("path")) {
            res.monoPath = element.Attribute("path");
            res.leftPath = element.Attribute("path");
            res.rightPath = element.Attribute("path");
        }
        if (element.Attribute("monoPath")) {
            res.monoPath = element.Attribute("monoPath");
        }
        if (element.Attribute("leftPath")) {
            res.leftPath = element.Attribute("leftPath");
        }
        if (element.Attribute("rightPath")) {
            res.rightPath = element.Attribute("rightPath");
        }
        if (element.Attribute("format")) {
            std::string_view format = element.Attribute("format");
            res.format = [](std::string_view format) {
                if (format == "png" || format == "PNG") {
                    return sgct::config::Capture::Format::PNG;
                }
                else if (format == "tga" || format == "TGA") {
                    return sgct::config::Capture::Format::TGA;
                }
                else if (format == "jpg" || format == "JPG") {
                    return sgct::config::Capture::Format::JPG;
                }
                else {
                    throw std::runtime_error("Unknown capturing format");
                }
            }(format);
        }
        return res;
    }

    sgct::config::Device parseDevice(tinyxml2::XMLElement& element) {
        using namespace sgct::core;

        sgct::config::Device device;
        device.name = element.Attribute("name");

        tinyxml2::XMLElement* child = element.FirstChildElement();
        while (child) {
            tinyxml2::XMLElement& e = *child;
            std::string_view childVal = e.Value();
            if (childVal == "Sensor") {
                sgct::config::Device::Sensors sensors;
                sensors.vrpnAddress = e.Attribute("vrpnAddress");
                sensors.identifier = *parseValue<int>(e, "id");
                device.sensors.push_back(sensors);
            }
            else if (childVal == "Buttons") {
                sgct::config::Device::Buttons buttons;
                buttons.vrpnAddress = e.Attribute("vrpnAddress");
                buttons.count = *parseValue<int>(e, "count");
                device.buttons.push_back(buttons);
            }
            else if (childVal == "Axes") {
                sgct::config::Device::Axes axes;
                axes.vrpnAddress = e.Attribute("vrpnAddress");
                axes.count = *parseValue<int>(e, "count");
                device.axes.push_back(axes);
            }
            else if (childVal == "Offset") {
                device.offset = parseValueVec3(e);
            }
            else if (childVal == "Orientation") {
                device.transformation = glm::mat4_cast(parseOrientationNode(e));
            }
            else if (childVal == "Matrix") {
                device.transformation = parseValueMat4(e);
                if (device.transformation) {
                    std::optional<bool> transpose = parseValue<bool>(e, "transpose");
                    if (transpose && *transpose) {
                        device.transformation = glm::transpose(*device.transformation);
                    }
                }
            }
            child = e.NextSiblingElement();
        }

        return device;
    }

    sgct::config::Tracker parseTracker(tinyxml2::XMLElement& element) {
        using namespace sgct::core;

        sgct::config::Tracker tracker;
        tracker.name = element.Attribute("name");

        tinyxml2::XMLElement* child = element.FirstChildElement();
        while (child) {
            tinyxml2::XMLElement& e = *child;
            std::string_view childVal = e.Value();
            if (childVal == "Device") {
                sgct::config::Device device = parseDevice(e);
                tracker.devices.push_back(device);
            }
            else if (childVal == "Offset") {
                tracker.offset = parseValueVec3(e);
            }
            else if (childVal == "Orientation") {
                tracker.transformation = glm::mat4_cast(parseOrientationNode(e));
            }
            else if (childVal == "Scale") {
                tracker.scale = parseValue<double>(e, "value");
            }
            else if (childVal == "Matrix") {
                tracker.transformation = parseValueMat4(e);
                if (tracker.transformation) {
                    std::optional<bool> transpose = parseValue<bool>(e, "transpose");
                    if (transpose && *transpose) {
                        tracker.transformation = glm::transpose(*tracker.transformation);
                    }
                }
            }
            child = e.NextSiblingElement();
        }

        return tracker;
    }

    sgct::config::Cluster readAndParseXMLFile(const std::string& filename) {
        if (filename.empty()) {
            throw std::runtime_error("No XML file set");
        }

        tinyxml2::XMLDocument xmlDoc;
        const bool s = xmlDoc.LoadFile(filename.c_str()) == tinyxml2::XML_NO_ERROR;
        if (!s) {
            std::string s1 = xmlDoc.ErrorName() ? xmlDoc.ErrorName() : "";
            std::string s2 = xmlDoc.GetErrorStr1() ? xmlDoc.GetErrorStr1() : "";
            std::string s3 = xmlDoc.GetErrorStr2() ? xmlDoc.GetErrorStr2() : "";
            std::string s4 = s1 + ' ' + s2 + ' ' + s3;
            throw std::runtime_error("Error loading XML file '" + filename + "'. " + s4);
        }

        sgct::config::Cluster cluster;
        tinyxml2::XMLElement* xmlRoot = xmlDoc.FirstChildElement("Cluster");
        if (xmlRoot == nullptr) {
            throw std::runtime_error("Cannot find XML root");
        }
        tinyxml2::XMLElement& root = *xmlRoot;

        const char* masterAddress = root.Attribute("masterAddress");
        if (!masterAddress) {
            throw std::runtime_error("Cannot find master address or DNS name in XML");
        }
        cluster.masterAddress = masterAddress;

        cluster.debug = parseValue<bool>(root, "debug");
        cluster.externalControlPort = parseValue<int>(root, "externalControlPort");
        cluster.firmSync = parseValue<bool>(root, "firmSync");

        tinyxml2::XMLElement* element = root.FirstChildElement();
        while (element) {
            tinyxml2::XMLElement& e = *element;
            std::string_view val = e.Value();

            if (val == "Scene") {
                cluster.scene = parseScene(e);
            }
            else if (val == "Node") {
                sgct::config::Node node = parseNode(e, filename);
                cluster.nodes.push_back(node);
            }
            else if (val == "User") {
                cluster.user = parseUser(e);
            }
            else if (val == "Settings") {
                cluster.settings = parseSettings(e);
            }
            else if (val == "Capture") {
                cluster.capture = parseCapture(e);
            }
            else if (val == "Tracker" && e.Attribute("name")) {
                cluster.tracker = parseTracker(e);
            }
            element = e.NextSiblingElement();
        }

        return cluster;
    }

    std::string replaceEnvVars(const std::string& filename) {
        size_t foundPercentage = filename.find('%');
        if (foundPercentage != std::string::npos) {
            throw std::runtime_error("SGCT doesn't support usage of '%%' in the path");
        }

        // First get all of the locations so that substitutions don't screw up future ones
        size_t foundIndex = 0;
        std::vector<std::pair<size_t, size_t>> envVarLocs;
        while (foundIndex != std::string::npos) {
            foundIndex = filename.find("$(", foundIndex);
            if (foundIndex != std::string::npos) {
                const size_t beginLocation = foundIndex;
                foundIndex = filename.find(')', foundIndex);
                if (foundIndex != std::string::npos) {
                    const size_t endLocation = foundIndex;
                    envVarLocs.emplace_back(beginLocation, endLocation);
                }
                else {
                    throw std::runtime_error("Bad configuration path string");
                }
            }
        }

        size_t appendPos = 0;
        std::string res;
        for (const std::pair<size_t, size_t>& p : envVarLocs) {
            const size_t beginLoc = p.first;
            const size_t endLoc = p.second;
            res.append(filename.substr(appendPos, beginLoc - appendPos));
            std::string envVar = filename.substr(beginLoc + 2, endLoc - (beginLoc + 2));

            bool environmentSuccess = true;
#ifdef WIN32
            size_t len;
            char* fetchedEnvVar;
            errno_t err = _dupenv_s(&fetchedEnvVar, &len, envVar.c_str());
            environmentSuccess = !err;
#else // not windows
            char* fetchedEnvVar = getenv(envVar.c_str());
            environmentSuccess = (fetchedEnvVar == nullptr);
#endif
            if (environmentSuccess) {
                char ErrorBuffer[1024];
                sprintf(
                    ErrorBuffer,
                    "Cannot fetch environment variable '%s'",
                    envVar.c_str()
                );
                throw std::runtime_error(ErrorBuffer);
            }

            res.append(fetchedEnvVar);
            appendPos = endLoc + 1;
        }

        res.append(filename.substr(appendPos));
        std::replace(res.begin(), res.end(), '\\', '/');

        return res;
    }
} // namespace

namespace sgct::core {

sgct::config::Cluster readConfig(const std::string& filename) {
    MessageHandler::printDebug("Parsing XML config '%s'", filename.c_str());

    std::string f = replaceEnvVars(filename);
    sgct::config::Cluster cluster = readAndParseXMLFile(f);

    MessageHandler::printDebug("Config file '%s' read successfully", f.c_str());
    MessageHandler::printInfo("Number of nodes in cluster: %d", cluster.nodes.size());

    for (size_t i = 0; i < cluster.nodes.size(); i++) {
        const sgct::config::Node& node = cluster.nodes[i];
        MessageHandler::printInfo(
            "\tNode(%d) address: %s [%d]", i, node.address.c_str(), node.port
        );
    }

    return cluster;
}

} // namespace sgct::core
