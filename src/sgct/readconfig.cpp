/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/readconfig.h>

#include <sgct/clustermanager.h>
#include <sgct/config.h>
#include <sgct/messagehandler.h>
#include <sgct/node.h>
#include <sgct/settings.h>
#include <sgct/tracker.h>
#include <sgct/trackingDevice.h>
#include <sgct/user.h>
#include <sgct/Viewport.h>
#include <sgct/helpers/stringfunctions.h>
#include <algorithm>
#include <optional>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>
#include <tinyxml2.h>

namespace {
    glm::quat parseOrientationNode(tinyxml2::XMLElement* element) {
        float x = 0.f;
        float y = 0.f;
        float z = 0.f;

        bool eulerMode = false;
        bool quatMode = false;

        glm::quat quat;

        float value;
        if (element->QueryFloatAttribute("w", &value) == tinyxml2::XML_NO_ERROR) {
            quat.w = value;
            quatMode = true;
        }

        if (element->QueryFloatAttribute("y", &value) == tinyxml2::XML_NO_ERROR) {
            y = value;
            eulerMode = true;
        }

        if (element->QueryFloatAttribute("yaw", &value) == tinyxml2::XML_NO_ERROR) {
            y = -value;
        }

        if (element->QueryFloatAttribute("heading", &value) == tinyxml2::XML_NO_ERROR) {
            y = -value;
        }

        if (element->QueryFloatAttribute("azimuth", &value) == tinyxml2::XML_NO_ERROR) {
            y = -value;
        }

        if (element->QueryFloatAttribute("x", &value) == tinyxml2::XML_NO_ERROR) {
            x = value;
            eulerMode = true;
        }

        if (element->QueryFloatAttribute("pitch", &value) == tinyxml2::XML_NO_ERROR) {
            x = value;
        }

        if (element->QueryFloatAttribute("elevation", &value) == tinyxml2::XML_NO_ERROR) {
            x = value;
        }

        if (element->QueryFloatAttribute("z", &value) == tinyxml2::XML_NO_ERROR) {
            z = value;
            eulerMode = true;
        }

        if (element->QueryFloatAttribute("roll", &value) == tinyxml2::XML_NO_ERROR) {
            z = -value;
        }

        if (element->QueryFloatAttribute("bank", &value) == tinyxml2::XML_NO_ERROR) {
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

    sgct::config::Window::StereoMode getStereoType(std::string type) {
        std::transform(
            type.cbegin(),
            type.cend(),
            type.begin(),
            [](char c) { return static_cast<char>(::tolower(c)); }
        );

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

        sgct::MessageHandler::instance()->printError(
            "Unknown stereo mode %s", type.c_str()
        );
        return sgct::config::Window::StereoMode::NoStereo;
    }

    sgct::config::Window::ColorBitDepth getBufferColorBitDepth(std::string type) {
        std::transform(
            type.cbegin(),
            type.cend(),
            type.begin(),
            [](char c) { return static_cast<char>(::tolower(c)); }
        );

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

        sgct::MessageHandler::instance()->printError(
            "Unknown color bit depth %s", type.c_str()
        );
        return sgct::config::Window::ColorBitDepth::Depth8;
    }

    int cubeMapResolutionForQuality(std::string quality) {
        std::transform(
            quality.cbegin(),
            quality.cend(),
            quality.begin(),
            [](char c) { return static_cast<char>(::tolower(c)); }
        );

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

        auto it = Map.find(quality);
        return (it != Map.end()) ? it->second : -1;
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
        using namespace tinyxml2;

        glm::vec2 value;
        XMLError xe = e.QueryFloatAttribute("x", &value[0]);
        XMLError ye = e.QueryFloatAttribute("y", &value[1]);
        if (xe == XML_NO_ERROR && ye == XML_NO_ERROR) {
            return value;
        }
        else {
            return std::nullopt;
        }
    }

    std::optional<glm::vec3> parseValueVec3(const tinyxml2::XMLElement& e) {
        using namespace tinyxml2;

        glm::vec3 value;
        XMLError xe = e.QueryFloatAttribute("x", &value[0]);
        XMLError ye = e.QueryFloatAttribute("y", &value[1]);
        XMLError ze = e.QueryFloatAttribute("z", &value[2]);
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
        float tmpf[16];
        tinyxml2::XMLError err[16] = {
            e.QueryFloatAttribute("x0", &tmpf[0]),
            e.QueryFloatAttribute("y0", &tmpf[1]),
            e.QueryFloatAttribute("z0", &tmpf[2]),
            e.QueryFloatAttribute("w0", &tmpf[3]),
            e.QueryFloatAttribute("x1", &tmpf[4]),
            e.QueryFloatAttribute("y1", &tmpf[5]),
            e.QueryFloatAttribute("z1", &tmpf[6]),
            e.QueryFloatAttribute("w1", &tmpf[7]),
            e.QueryFloatAttribute("x2", &tmpf[8]),
            e.QueryFloatAttribute("y2", &tmpf[9]),
            e.QueryFloatAttribute("z2", &tmpf[10]),
            e.QueryFloatAttribute("w2", &tmpf[11]),
            e.QueryFloatAttribute("x3", &tmpf[12]),
            e.QueryFloatAttribute("y3", &tmpf[13]),
            e.QueryFloatAttribute("z3", &tmpf[14]),
            e.QueryFloatAttribute("w3", &tmpf[15])
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
            glm::mat4 mat = glm::make_mat4(tmpf);
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
            sgct::MessageHandler::instance()->printError(
                "Error extracting value '%s'", name
            );
            return std::nullopt;
        }
    }

    sgct::config::PlanarProjection parsePlanarProjection(tinyxml2::XMLElement* element) {
        using namespace tinyxml2;

        bool foundFov = false;

        sgct::config::PlanarProjection proj;
        XMLElement* subElement = element->FirstChildElement();
        while (subElement) {
            std::string_view val = subElement->Value();

            if (val == "FOV") {
                foundFov = true;
                std::optional<float> down = parseValue<float>(*subElement, "down");
                std::optional<float> left = parseValue<float>(*subElement, "left");
                std::optional<float> right = parseValue<float>(*subElement, "right");
                std::optional<float> up = parseValue<float>(*subElement, "up");

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
                    sgct::MessageHandler::instance()->printError(
                        "Viewport: Failed to parse planar projection FOV from XML"
                    );
                }
                proj.fov.distance = parseValue<float>(*subElement, "distance");
            }
            else if (val == "Orientation") {
                proj.orientation = parseOrientationNode(subElement);
            }
            else if (val == "Offset") {
                proj.offset = parseValueVec3(*subElement);
            }

            subElement = subElement->NextSiblingElement();
        }

        if (!foundFov) {
            throw std::runtime_error("Missing specification of field-of-view values");
        }

        return proj;
    }

    sgct::config::FisheyeProjection parseFisheyeProjection(tinyxml2::XMLElement* element)
    {
        sgct::config::FisheyeProjection proj;

        proj.fov = parseValue<float>(*element, "fov");
        if (element->Attribute("quality")) {
            const int res = cubeMapResolutionForQuality(element->Attribute("quality"));
            proj.quality = res;
        }
        if (element->Attribute("method")) {
            std::string_view method = element->Attribute("method");
            proj.method = method == "five_face_cube" ?
                sgct::config::FisheyeProjection::Method::FiveFace :
                sgct::config::FisheyeProjection::Method::FourFace;
        }
        if (element->Attribute("interpolation")) {
            std::string_view interpolation = element->Attribute("interpolation");
            proj.interpolation = interpolation == "cubic" ?
                sgct::config::FisheyeProjection::Interpolation::Cubic :
                sgct::config::FisheyeProjection::Interpolation::Linear;
        }
        proj.diameter = parseValue<float>(*element, "diameter");
        proj.tilt = parseValue<float>(*element, "tilt");

        tinyxml2::XMLElement* subElement = element->FirstChildElement();
        while (subElement) {
            std::string_view val = subElement->Value();

            if (val == "Crop") {
                std::optional<float> left = parseValue<float>(*subElement, "left");
                std::optional<float> right = parseValue<float>(*subElement, "right");
                std::optional<float> bottom = parseValue<float>(*subElement, "bottom");
                std::optional<float> top = parseValue<float>(*subElement, "top");
                sgct::config::FisheyeProjection::Crop crop;
                if (left) {
                    crop.left = *left;
                }
                if (right) {
                    crop.right = *right;
                }
                if (bottom) {
                    crop.bottom = *bottom;
                }
                if (top) {
                    crop.top = *top;
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
                                                            tinyxml2::XMLElement* element)
    {
        sgct::config::SphericalMirrorProjection proj;
        if (element->Attribute("quality")) {
            proj.quality = cubeMapResolutionForQuality(element->Attribute("quality"));
        }

        proj.tilt = parseValue<float>(*element, "tilt");

        tinyxml2::XMLElement* subElement = element->FirstChildElement();
        while (subElement) {
            std::string_view val = subElement->Value();

            if (val == "Background") {
                proj.background = parseValueColor(*subElement);
            }
            else if (val == "Geometry") {
                if (subElement->Attribute("bottom")) {
                    proj.mesh.bottom = subElement->Attribute("bottom");
                }

                if (subElement->Attribute("left")) {
                    proj.mesh.left = subElement->Attribute("left");
                }

                if (subElement->Attribute("right")) {
                    proj.mesh.right = subElement->Attribute("right");
                }

                if (subElement->Attribute("top")) {
                    proj.mesh.top = subElement->Attribute("top");
                }
            }

            subElement = subElement->NextSiblingElement();
        }
        return proj;
    }

    sgct::config::SpoutOutputProjection parseSpoutOutputProjection(
                                                            tinyxml2::XMLElement* element)
    {
        sgct::config::SpoutOutputProjection proj;

        if (element->Attribute("quality")) {
            proj.quality = cubeMapResolutionForQuality(element->Attribute("quality"));
        }
        if (element->Attribute("mapping")) {
            using namespace sgct::config;
            std::string_view val = element->Attribute("mapping");
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
        if (element->Attribute("mappingSpoutName")) {
            proj.mappingSpoutName = element->Attribute("mappingSpoutName");
        }

        tinyxml2::XMLElement* subElement = element->FirstChildElement();
        while (subElement) {
            std::string_view val = subElement->Value();

            if (val == "Background") {
                proj.background = parseValueColor(*subElement);
            }

            if (val == "Channels") {
                sgct::config::SpoutOutputProjection::Channels c;
                c.right = *parseValue<bool>(*subElement, "Right");
                c.zLeft = *parseValue<bool>(*subElement, "zLeft");
                c.bottom = *parseValue<bool>(*subElement, "Bottom");
                c.top = *parseValue<bool>(*subElement, "Top");
                c.left = *parseValue<bool>(*subElement, "Left");
                c.zRight = *parseValue<bool>(*subElement, "zRight");
                proj.channels = c;
            }

            if (val == "RigOrientation") {
                glm::vec3 orientation = glm::vec3(
                    *parseValue<float>(*subElement, "pitch"),
                    *parseValue<float>(*subElement, "yaw"),
                    *parseValue<float>(*subElement, "roll")
                );
                proj.orientation = orientation;
            }

            subElement = subElement->NextSiblingElement();
        }

        return proj;
    }

    sgct::config::ProjectionPlane parseProjectionPlane(tinyxml2::XMLElement* element) {
        using namespace tinyxml2;
        size_t i = 0;

        sgct::config::ProjectionPlane proj;

        tinyxml2::XMLElement* elem = element->FirstChildElement();
        while (elem) {
            std::string_view val = elem->Value();

            if (val == "Pos") {
                std::optional<glm::vec3> pos = parseValueVec3(*elem);
                if (pos) {
                    switch (i % 3) {
                        case 0:
                            proj.lowerLeft = pos;
                            break;
                        case 1:
                            proj.upperLeft = pos;
                            break;
                        case 2:
                            proj.upperRight = pos;
                            break;
                    }

                    i++;
                }
                else {
                    sgct::MessageHandler::instance()->printError(
                        "ProjectionPlane: Failed to parse coordinates from XML"
                    );
                }
            }

            elem = elem->NextSiblingElement();
        }

        return proj;
    }

    sgct::config::Viewport parseViewport(tinyxml2::XMLElement* element) {
        sgct::config::Viewport viewport;
        if (element->Attribute("user")) {
            viewport.user = element->Attribute("user");
        }

        if (element->Attribute("name")) {
            viewport.name = element->Attribute("name");
        }

        if (element->Attribute("overlay")) {
            viewport.overlayTexture = element->Attribute("overlay");
        }

        // for backward compability
        if (element->Attribute("mask")) {
            viewport.blendMaskTexture = element->Attribute("mask");
        }

        if (element->Attribute("BlendMask")) {
            viewport.blendMaskTexture = element->Attribute("BlendMask");
        }

        if (element->Attribute("BlackLevelMask")) {
            viewport.blendLevelMaskTexture = element->Attribute("BlackLevelMask");
        }

        if (element->Attribute("mesh")) {
            viewport.correctionMeshTexture = element->Attribute("mesh");
        }

        if (element->Attribute("hint")) {
            viewport.meshHint = element->Attribute("hint");
        }

        viewport.isTracked = parseValue<bool>(*element, "tracked");

        // get eye if set
        if (element->Attribute("eye")) {
            std::string_view eye = element->Attribute("eye");
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

        tinyxml2::XMLElement* subElement = element->FirstChildElement();
        while (subElement) {
            std::string_view val = subElement->Value();
            if (val == "Pos") {
                std::optional<glm::vec2> pos = parseValueVec2(*subElement);
                if (pos) {
                    viewport.position = *pos;
                }
                else {
                    sgct::MessageHandler::instance()->printError(
                        "Viewport: Failed to parse position from XML"
                    );
                }
            }
            else if (val == "Size") {
                std::optional<glm::vec2> size = parseValueVec2(*subElement);
                if (size) {
                    viewport.size = *size;
                }
                else {
                    sgct::MessageHandler::instance()->printError(
                        "Viewport: Failed to parse size from XML"
                    );
                }
            }
            else if (val == "PlanarProjection") {
                viewport.projection = parsePlanarProjection(subElement);
            }
            else if (val == "FisheyeProjection") {
                viewport.projection = parseFisheyeProjection(subElement);
            }
            else if (val == "SphericalMirrorProjection") {
                viewport.projection = parseSphericalMirrorProjection(subElement);
            }
            else if (val == "SpoutOutputProjection") {
                viewport.projection = parseSpoutOutputProjection(subElement);
            }
            else if (val == "Viewplane" || val == "Projectionplane") {
                viewport.projection = parseProjectionPlane(subElement);
            }

            subElement = subElement->NextSiblingElement();
        }

        return viewport;
    }


    sgct::config::Scene parseScene(tinyxml2::XMLElement* element) {
        using namespace sgct::core;

        sgct::config::Scene scene;

        tinyxml2::XMLElement* child = element->FirstChildElement();
        while (child) {
            std::string_view childVal = child->Value();

            if (childVal == "Offset") {
                scene.offset = parseValueVec3(*child);
            }
            else if (childVal == "Orientation") {
                scene.orientation = parseOrientationNode(child);
            }
            else if (childVal == "Scale") {
                scene.scale = parseValue<float>(*child, "value");
            }

            child = child->NextSiblingElement();
        }

        return scene;
    }

    sgct::config::Window parseWindow(tinyxml2::XMLElement* element, std::string filename)
    {
        using namespace tinyxml2;

        sgct::config::Window window;

        if (element->Attribute("name")) {
            window.name = element->Attribute("name");
        }

        if (element->Attribute("tags")) {
            std::string tags = element->Attribute("tags");
            std::vector<std::string> t = sgct::helpers::split(tags, ',');
            window.tags = std::move(t);
        }

        if (element->Attribute("bufferBitDepth")) {
            window.bufferBitDepth = getBufferColorBitDepth(
                element->Attribute("bufferBitDepth")
            );
        }

        // compatibility with older versions
        if (element->Attribute("fullScreen")) {
            window.isFullScreen = parseValue<bool>(*element, "fullScreen");
        }

        if (element->Attribute("fullscreen")) {
            window.isFullScreen = parseValue<bool>(*element, "fullscreen");
        }

        window.isFloating = parseValue<bool>(*element, "floating");
        window.alwaysRender = parseValue<bool>(*element, "alwaysRender");
        window.isHidden = parseValue<bool>(*element, "hidden");
        window.doubleBuffered = parseValue<bool>(*element, "dbuffered");
        window.gamma = parseValue<float>(*element, "gamma");
        window.contrast = parseValue<float>(*element, "contrast");
        window.brightness = parseValue<float>(*element, "brightness");
        window.msaa = parseValue<int>(*element, "numberOfSamples");

        if (element->Attribute("numberOfSamples")) {
            window.msaa = parseValue<int>(*element, "numberOfSamples");
        }
        if (element->Attribute("msaa")) {
            window.msaa = parseValue<int>(*element, "msaa");
        }
        if (element->Attribute("MSAA")) {
            window.msaa = parseValue<int>(*element, "MSAA");
        }
        window.hasAlpha = parseValue<bool>(*element, "alpha");

        if (element->Attribute("fxaa")) {
            window.useFxaa = parseValue<bool>(*element, "fxaa");
        }
        if (element->Attribute("FXAA")) {
            window.useFxaa = parseValue<bool>(*element, "FXAA");
        }

        window.isDecorated = parseValue<bool>(*element, "decorated");
        window.hasBorder = parseValue<bool>(*element, "border");
        window.draw2D = parseValue<bool>(*element, "draw2D");
        window.draw3D = parseValue<bool>(*element, "draw3D");
        window.copyPreviousWindowToCurrentWindow = parseValue<bool>(
            *element,
            "copyPreviousWindowToCurrentWindow"
        );
        window.monitor = parseValue<int>(*element, "monitor");

        if (element->Attribute("mpcdi")) {
            std::string path;
            size_t lastSlashPos = filename.find_last_of("/");
            if (lastSlashPos != std::string::npos) {
                path = filename.substr(0, lastSlashPos) + "/";
            }
            path += element->Attribute("mpcdi");
            std::replace(path.begin(), path.end(), '\\', '/');
            window.mpcdi = path;
        }

        XMLElement* child = element->FirstChildElement();
        while (child) {
            std::string_view childVal = child->Value();

            if (childVal == "Stereo") {
                window.stereo = getStereoType(child->Attribute("type"));
            }
            else if (childVal == "Pos") {
                window.pos = parseValueIVec2(*child);
            }
            else if (childVal == "Size") {
                std::optional<glm::ivec2> s = parseValueIVec2(*child);
                if (s) {
                    window.size = *s;
                }
                else {
                    throw std::runtime_error("Could not parse window size");
                }
            }
            else if (childVal == "Res") {
                window.resolution = parseValueIVec2(*child);
            }
            else if (childVal == "Viewport") {
                window.viewports.push_back(parseViewport(child));
            }
            child = child->NextSiblingElement();
        }

        return window;
    }

    sgct::config::Node parseNode(tinyxml2::XMLElement* element, std::string xmlFileName) {
        bool foundAddress = false;
        bool foundPort = false;
        sgct::config::Node node;
        if (element->Attribute("address")) {
            node.address = element->Attribute("address");
            foundAddress = true;
        }
        if (element->Attribute("ip")) {
            node.address = element->Attribute("ip");
            foundAddress = true;
        }
        if (element->Attribute("name")) {
            node.name = element->Attribute("name");
        }
        if (element->Attribute("port")) {
            node.port = *parseValue<int>(*element, "port");
            foundPort = true;
        }
        if (element->Attribute("syncPort")) {
            node.port = *parseValue<int>(*element, "syncPort");
            foundPort = true;
        }
        node.dataTransferPort = parseValue<int>(*element, "dataTransferPort");
        node.swapLock = parseValue<bool>(*element, "swapLock");

        tinyxml2::XMLElement* child = element->FirstChildElement();
        while (child) {
            std::string_view childVal = child->Value();
            if (childVal == "Window") {
                sgct::config::Window window = parseWindow(child, xmlFileName);
                node.windows.push_back(window);
            }
            child = child->NextSiblingElement();
        }

        if (!foundAddress) {
            throw std::runtime_error("Missing field address in node");
        }
        if (!foundPort) {
            throw std::runtime_error("Missing field port in node");
        }
        return node;
    }

    sgct::config::User parseUser(tinyxml2::XMLElement* element) {
        using namespace sgct::core;
        using namespace tinyxml2;

        sgct::config::User user;
        if (element->Attribute("name")) {
            user.name = element->Attribute("name");
        }
        user.eyeSeparation = parseValue<float>(*element, "eyeSeparation");

        XMLElement* child = element->FirstChildElement();
        while (child) {
            std::string_view childVal = child->Value();
            if (childVal == "Pos") {
                user.position = parseValueVec3(*child);
            }
            else if (childVal == "Orientation") {
                user.transformation = glm::mat4_cast(parseOrientationNode(child));
            }
            else if (childVal == "Matrix") {
                user.transformation = parseValueMat4(*child);
                if (user.transformation) {
                    std::optional<bool> transpose = parseValue<bool>(*child, "transpose");
                    if (transpose && *transpose) {
                        user.transformation = glm::transpose(*user.transformation);
                    }
                }
            }
            else if (childVal == "Tracking") {
                sgct::config::User::Tracking tracking;
                tracking.tracker = child->Attribute("tracker");
                tracking.device = child->Attribute("device");
            }
            child = child->NextSiblingElement();
        }

        return user;
    }


    sgct::config::Settings parseSettings(tinyxml2::XMLElement* element) {
        sgct::config::Settings settings;

        tinyxml2::XMLElement* elem = element->FirstChildElement();
        while (elem) {
            std::string_view val = elem->Value();

            if (val == "DepthBufferTexture") {
                settings.useDepthTexture = parseValue<bool>(*elem, "value");
            }
            else if (val == "NormalTexture") {
                settings.useNormalTexture = parseValue<bool>(*elem, "value");
            }
            else if (val == "PositionTexture") {
                settings.usePositionTexture = parseValue<bool>(*elem, "value");
            }
            else if (val == "PBO") {
                settings.usePBO = parseValue<bool>(*elem, "value");
            }
            else if (val == "Precision") {
                std::optional<float> f = parseValue<float>(*elem, "float");
                if (f) {
                    if (*f == 16) {
                        settings.bufferFloatPrecision =
                            sgct::config::Settings::BufferFloatPrecision::Float16Bit;
                    }
                    else if (*f == 32) {
                        settings.bufferFloatPrecision =
                            sgct::config::Settings::BufferFloatPrecision::Float32Bit;
                    }
                    else {
                        sgct::MessageHandler::instance()->printWarning(
                            "ReadConfig: Wrong precision value (%d). Must be 16 or 32", *f
                        );
                    }
                }
            }
            else if (val == "Display") {
                sgct::config::Settings::Display display;
                display.swapInterval = parseValue<int>(*elem, "swapInterval");
                display.refreshRate = parseValue<int>(*elem, "refreshRate");
                display.maintainAspectRatio = parseValue<bool>(
                    *elem,
                    "tryMaintainAspectRatio"
                );
                display.exportWarpingMeshes = parseValue<bool>(
                    *elem,
                    "exportWarpingMeshes"
                );
                settings.display = display;
            }
            else if (val == "OSDText") {
                sgct::config::Settings::OSDText osdText;
                if (elem->Attribute("name")) {
                    osdText.name = elem->Attribute("name");
                }
                if (elem->Attribute("path")) {
                    osdText.path = elem->Attribute("path");
                }
                osdText.size = parseValue<int>(*elem, "size");
                osdText.xOffset = parseValue<float>(*elem, "xOffset");
                osdText.yOffset = parseValue<float>(*elem, "yOffset");
                settings.osdText = osdText;
            }
            else if (val == "FXAA") {
                sgct::config::Settings::FXAA fxaa;
                fxaa.offset = parseValue<float>(*elem, "offset");
                fxaa.trim = parseValue<float>(*elem, "trim");
                settings.fxaa = fxaa;
            }

            elem = elem->NextSiblingElement();
        }

        return settings;
    }

    sgct::config::Capture parseCapture(tinyxml2::XMLElement* element) {
        sgct::config::Capture res;
        if (element->Attribute("path")) {
            res.monoPath = element->Attribute("path");
            res.leftPath = element->Attribute("path");
            res.rightPath = element->Attribute("path");
        }
        if (element->Attribute("monoPath")) {
            res.monoPath = element->Attribute("monoPath");
        }
        if (element->Attribute("leftPath")) {
            res.leftPath = element->Attribute("leftPath");
        }
        if (element->Attribute("rightPath")) {
            res.rightPath = element->Attribute("rightPath");
        }
        if (element->Attribute("format")) {
            std::string_view format = element->Attribute("format");
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
                    sgct::MessageHandler::instance()->printWarning(
                        "ReadConfig: Unknown capturing format. Using PNG"
                    );
                    return sgct::config::Capture::Format::PNG;
                }
            } (format);
        }
        return res;
    }

    sgct::config::Device parseDevice(tinyxml2::XMLElement* element) {
        using namespace sgct::core;
        using namespace tinyxml2;

        sgct::config::Device device;
        device.name = element->Attribute("name");

        XMLElement* child = element->FirstChildElement();
        while (child) {
            std::string_view childVal = child->Value();
            if (childVal == "Sensor") {
                sgct::config::Device::Sensors sensors;
                sensors.vrpnAddress = child->Attribute("vrpnAddress");
                sensors.identifier = *parseValue<int>(*child, "id");
                device.sensors.push_back(sensors);
            }
            else if (childVal == "Buttons") {
                sgct::config::Device::Buttons buttons;
                buttons.vrpnAddress = child->Attribute("vrpnAddress");
                buttons.count = *parseValue<int>(*child, "count");
                device.buttons.push_back(buttons);
            }
            else if (childVal == "Axes") {
                sgct::config::Device::Axes axes;
                axes.vrpnAddress = child->Attribute("vrpnAddress");
                axes.count = *parseValue<int>(*child, "count");
                device.axes.push_back(axes);
            }
            else if (childVal == "Offset") {
                device.offset = parseValueVec3(*child);
            }
            else if (childVal == "Orientation") {
                device.transformation = glm::mat4_cast(parseOrientationNode(child));
            }
            else if (childVal == "Matrix") {
                device.transformation = parseValueMat4(*child);
                if (device.transformation) {
                    std::optional<bool> transpose = parseValue<bool>(*child, "transpose");
                    if (transpose && *transpose) {
                        device.transformation = glm::transpose(*device.transformation);
                    }
                }
            }
            child = child->NextSiblingElement();
        }

        return device;
    }

    sgct::config::Tracker parseTracker(tinyxml2::XMLElement* element) {
        using namespace sgct::core;
        using namespace tinyxml2;

        sgct::config::Tracker tracker;
        tracker.name = element->Attribute("name");

        XMLElement* child = element->FirstChildElement();
        while (child) {
            std::string_view childVal = child->Value();
            if (childVal == "Device") {
                sgct::config::Device device = parseDevice(child);
                tracker.devices.push_back(device);
            }
            else if (childVal == "Offset") {
                tracker.offset = parseValueVec3(*child);
            }
            else if (childVal == "Orientation") {
                tracker.transformation = glm::mat4_cast(parseOrientationNode(child));
            }
            else if (childVal == "Scale") {
                tracker.scale = parseValue<double>(*child, "value");
            }
            else if (childVal == "Matrix") {
                tracker.transformation = parseValueMat4(*child);
                if (tracker.transformation) {
                    std::optional<bool> transpose = parseValue<bool>(*child, "transpose");
                    if (transpose && *transpose) {
                        tracker.transformation = glm::transpose(*tracker.transformation);
                    }
                }
            }
            child = child->NextSiblingElement();
        }

        return tracker;
    }

    sgct::config::Cluster readAndParseXMLFile(const std::string& filename) {
        if (filename.empty()) {
            throw std::runtime_error("No XML file set");
        }

        tinyxml2::XMLDocument xmlDoc;
        bool s = xmlDoc.LoadFile(filename.c_str()) == tinyxml2::XML_NO_ERROR;
        if (!s) {
            std::string s1 = xmlDoc.ErrorName() ? xmlDoc.ErrorName() : "";
            std::string s2 = xmlDoc.GetErrorStr1() ? xmlDoc.GetErrorStr1() : "";
            std::string s3 = xmlDoc.GetErrorStr2() ? xmlDoc.GetErrorStr2() : "";
            throw std::runtime_error(
                "Error loading XML file '" + filename + "'. " + s1 + ' ' + s2 + ' ' + s3
            );
        }

        sgct::config::Cluster cluster;

        tinyxml2::XMLElement* XMLroot = xmlDoc.FirstChildElement("Cluster");
        if (XMLroot == nullptr) {
            throw std::runtime_error("Cannot find XML root");
        }

        const char* masterAddress = XMLroot->Attribute("masterAddress");
        if (!masterAddress) {
            throw std::runtime_error("Cannot find master address or DNS name in XML");
        }
        cluster.masterAddress = masterAddress;

        cluster.debug = parseValue<bool>(*XMLroot, "debug");
        cluster.externalControlPort = parseValue<int>(*XMLroot, "externalControlPort");
        cluster.firmSync = parseValue<bool>(*XMLroot, "firmSync");

        tinyxml2::XMLElement* element = XMLroot->FirstChildElement();
        while (element) {
            std::string_view val = element->Value();

            if (val == "Scene") {
                cluster.scene = parseScene(element);
            }
            else if (val == "Node") {
                sgct::config::Node node = parseNode(element, filename);
                cluster.nodes.push_back(node);
            }
            else if (val == "User") {
                cluster.user = parseUser(element);
            }
            else if (val == "Settings") {
                cluster.settings = parseSettings(element);
            }
            else if (val == "Capture") {
                cluster.capture = parseCapture(element);
            }
            else if (val == "Tracker" && element->Attribute("name")) {
                cluster.tracker = parseTracker(element);
            }
            element = element->NextSiblingElement();
        }

        return cluster;
    }

    std::string replaceEnvVars(const std::string& filename) {
        size_t foundPercentage = filename.find('%');
        if (foundPercentage != std::string::npos) {
            sgct::MessageHandler::instance()->printError(
                "ReadConfig: SGCT doesn't support the usage of '%%' in the path"
            );
            return "";
        }

        std::vector<size_t> beginEnvVar;
        std::vector<size_t> endEnvVar;

        std::string res;

        size_t foundIndex = 0;
        while (foundIndex != std::string::npos) {
            foundIndex = filename.find("$(", foundIndex);
            if (foundIndex != std::string::npos) {
                beginEnvVar.push_back(foundIndex);
                foundIndex = filename.find(')', foundIndex);
                if (foundIndex != std::string::npos) {
                    endEnvVar.push_back(foundIndex);
                }
            }
        }

        if (beginEnvVar.size() != endEnvVar.size()) {
            sgct::MessageHandler::instance()->printError(
                "ReadConfig: Error: Bad configuration path string"
            );
            return std::string();
        }
        else {
            size_t appendPos = 0;
            for (unsigned int i = 0; i < beginEnvVar.size(); i++) {
                res.append(filename.substr(appendPos, beginEnvVar[i] - appendPos));
                std::string envVar = filename.substr(
                    beginEnvVar[i] + 2,
                    endEnvVar[i] - (beginEnvVar[i] + 2)
                );

#if (_MSC_VER >= 1400) // visual studio 2005 or later
                size_t len;
                char* fetchedEnvVar;
                errno_t err = _dupenv_s(&fetchedEnvVar, &len, envVar.c_str());
                if (err) {
                    sgct::MessageHandler::instance()->printError(
                        "ReadConfig: Error: Cannot fetch environment variable '%s'",
                        envVar.c_str()
                    );
                    return std::string();
                }
#else
                char* fetchedEnvVar = getenv(envVar.c_str());
                if (fetchedEnvVar == nullptr) {
                    sgct::MessageHandler::instance()->printError(
                        "ReadConfig: Error: Cannot fetch environment variable '%s'",
                        envVar.c_str()
                    );
                    return std::string();
                }
#endif

                res.append(fetchedEnvVar);
                appendPos = endEnvVar[i] + 1;
            }

            res.append(filename.substr(appendPos));
            std::replace(res.begin(), res.end(), char(92), '/');
        }

        return res;
    }
} // namespace

namespace sgct::core::readconfig {

glm::quat parseMpcdiOrientationNode(float yaw, float pitch, float roll) {
    const float x = pitch;
    const float y = -yaw;
    const float z = -roll;

    glm::quat quat;
    quat = glm::rotate(quat, glm::radians(y), glm::vec3(0.f, 1.f, 0.f));
    quat = glm::rotate(quat, glm::radians(x), glm::vec3(1.f, 0.f, 0.f));
    quat = glm::rotate(quat, glm::radians(z), glm::vec3(0.f, 0.f, 1.f));

    return quat;
}

sgct::config::Cluster readConfig(const std::string& filename) {
    std::string f = filename;
    MessageHandler::instance()->printDebug(
        "ReadConfig: Parsing XML config '%s'", f.c_str()
    );

    f = replaceEnvVars(f);
    if (f.empty()) {
        throw std::runtime_error("Could not resolve file path");
    }

    sgct::config::Cluster cluster = readAndParseXMLFile(f);

    MessageHandler::instance()->printDebug(
        "ReadConfig: Config file '%s' read successfully", f.c_str()
    );
    MessageHandler::instance()->printInfo(
        "ReadConfig: Number of nodes in cluster: %d", cluster.nodes.size()
    );

    for (size_t i = 0; i < cluster.nodes.size(); i++) {
        const sgct::config::Node& node = cluster.nodes[i];
        MessageHandler::instance()->printInfo(
            "\tNode(%d) address: %s [%d]", i, node.address.c_str(), node.port
        );
    }

    return cluster;
}

} // namespace sgct_config::readconfig
