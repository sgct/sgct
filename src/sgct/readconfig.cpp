/*************************************************************************
 Copyright (c) 2012-2015 Miroslav Andel
 All rights reserved.
 
 For conditions of distribution and use, see copyright notice in sgct.h
 *************************************************************************/

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
            type.begin(),
            type.end(),
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

        return sgct::config::Window::StereoMode::NoStereo;
    }

    sgct::config::Window::ColorBitDepth getBufferColorBitDepth(std::string type) {
        std::transform(
            type.begin(),
            type.end(),
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

        return sgct::config::Window::ColorBitDepth::Depth8;
    }

    int cubeMapResolutionForQuality(const std::string& quality) {
        std::string q = quality;
        q.resize(quality.size());
        std::transform(
            quality.begin(),
            quality.end(),
            q.begin(),
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
        if (it != Map.end()) {
            return it->second;
        }
        else {
            return -1;
        }
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

    std::optional<glm::quat> parseValueQuat(const tinyxml2::XMLElement& e) {
        glm::quat q;
        // Yes, this order is correct;  the constructor takes w,x,y,z but the
        // layout in memory is x, y, z, w
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
        T value;
        tinyxml2::XMLError err;
        if constexpr (std::is_same_v<T, float>) {
            err = e.QueryFloatAttribute(name, &value);
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
            return std::nullopt;
        }
    }

    std::optional<float> parseFrustumElement(const tinyxml2::XMLElement& elem,
        std::string_view frustumTag)
    {
        if (frustumTag == elem.Value()) {
            try {
                return std::stof(elem.GetText());
            }
            catch (const std::invalid_argument&) {
                std::string msg = "Viewport: Failed to parse frustum element "
                    + std::string(frustumTag) + " from MPCDI XML\n";
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
                    msg.c_str()
                );
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] sgct::config::PlanarProjection parsePlanarProjection(tinyxml2::XMLElement* element) {
        using namespace tinyxml2;

        sgct::config::PlanarProjection proj;
        XMLElement* subElement = element->FirstChildElement();
        while (subElement) {
            std::string_view val = subElement->Value();

            if (val == "FOV") {
                sgct::config::PlanarProjection::FOV fov;
                XMLError errDown = subElement->QueryFloatAttribute("down", &fov.down);
                XMLError errLeft = subElement->QueryFloatAttribute("left", &fov.left);
                XMLError errRight = subElement->QueryFloatAttribute("right", &fov.right);
                XMLError errUp = subElement->QueryFloatAttribute("up", &fov.up);

                if (errDown == XML_NO_ERROR && errLeft == XML_NO_ERROR &&
                    errRight == XML_NO_ERROR && errUp == XML_NO_ERROR)
                {
                    proj.fov = fov;
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "Viewport: Failed to parse planar projection FOV from XML\n"
                    );
                }
            }
            else if (val == "Orientation") {
                proj.orientation = parseOrientationNode(subElement);
            }
            else if (val == "Offset") {
                glm::vec3 offset;
                subElement->QueryFloatAttribute("x", &offset[0]);
                subElement->QueryFloatAttribute("y", &offset[1]);
                subElement->QueryFloatAttribute("z", &offset[2]);
                proj.offset = offset;
            }

            subElement = subElement->NextSiblingElement();
        }

        return proj;
    }

    [[nodiscard]] sgct::config::FisheyeProjection parseFisheyeProjection(tinyxml2::XMLElement* element) {
        sgct::config::FisheyeProjection proj;

        float fov;
        if (element->QueryFloatAttribute("fov", &fov) == tinyxml2::XML_NO_ERROR) {
            proj.fov = fov;
        }
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
        float diameter;
        if (element->QueryFloatAttribute("diameter", &diameter) == tinyxml2::XML_NO_ERROR) {
            proj.diameter = diameter;
        }

        float tilt;
        if (element->QueryFloatAttribute("tilt", &tilt) == tinyxml2::XML_NO_ERROR) {
            proj.tilt = tilt;
        }

        tinyxml2::XMLElement* subElement = element->FirstChildElement();
        while (subElement) {
            std::string_view val = subElement->Value();

            if (val == "Crop") {
                sgct::config::FisheyeProjection::Crop crop;
                subElement->QueryFloatAttribute("left", &crop.left);
                subElement->QueryFloatAttribute("right", &crop.right);
                subElement->QueryFloatAttribute("bottom", &crop.bottom);
                subElement->QueryFloatAttribute("top", &crop.top);
                proj.crop = crop;
            }
            else if (val == "Offset") {
                glm::vec3 offset = glm::vec3(0.f);
                subElement->QueryFloatAttribute("x", &offset[0]);
                subElement->QueryFloatAttribute("y", &offset[1]);
                subElement->QueryFloatAttribute("z", &offset[2]);
                proj.offset = offset;
            }
            if (val == "Background") {
                glm::vec4 color;
                subElement->QueryFloatAttribute("r", &color[0]);
                subElement->QueryFloatAttribute("g", &color[1]);
                subElement->QueryFloatAttribute("b", &color[2]);
                subElement->QueryFloatAttribute("a", &color[3]);
                proj.background = color;
            }

            subElement = subElement->NextSiblingElement();
        }

        return proj;
    }

    [[nodiscard]] sgct::config::SphericalMirrorProjection parseSphericalMirrorProjection(tinyxml2::XMLElement* element) {
        sgct::config::SphericalMirrorProjection proj;
        if (element->Attribute("quality")) {
            proj.quality = cubeMapResolutionForQuality(element->Attribute("quality"));
        }

        float tilt;
        if (element->QueryFloatAttribute("tilt", &tilt) == tinyxml2::XML_NO_ERROR) {
            proj.tilt = tilt;
        }

        tinyxml2::XMLElement* subElement = element->FirstChildElement();
        while (subElement) {
            std::string_view val = subElement->Value();

            if (val == "Background") {
                glm::vec4 color;
                subElement->QueryFloatAttribute("r", &color[0]);
                subElement->QueryFloatAttribute("g", &color[1]);
                subElement->QueryFloatAttribute("b", &color[2]);
                subElement->QueryFloatAttribute("a", &color[3]);
                proj.background = color;
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

    [[nodiscard]] sgct::config::SpoutOutputProjection parseSpoutOutputProjection(tinyxml2::XMLElement* element) {
        sgct::config::SpoutOutputProjection proj;

        if (element->Attribute("quality")) {
            proj.quality = cubeMapResolutionForQuality(element->Attribute("quality"));
        }
        if (element->Attribute("mapping")) {
            std::string_view val = element->Attribute("mapping");
            if (val == "fisheye") {
                proj.mapping = sgct::config::SpoutOutputProjection::Mapping::Fisheye;
            }
            else if (val == "equirectangular") {
                proj.mapping = sgct::config::SpoutOutputProjection::Mapping::Equirectangular;
            }
            else if (val == "cubemap") {
                proj.mapping = sgct::config::SpoutOutputProjection::Mapping::Cubemap;
            }
            else {
                proj.mapping = sgct::config::SpoutOutputProjection::Mapping::Cubemap;
            }
        }
        if (element->Attribute("mappingSpoutName")) {
            proj.mappingSpoutName = element->Attribute("mappingSpoutName");
        }

        tinyxml2::XMLElement* subElement = element->FirstChildElement();
        while (subElement) {
            std::string_view val = subElement->Value();

            if (val == "Background") {
                glm::vec4 color;
                subElement->QueryFloatAttribute("r", &color[0]);
                subElement->QueryFloatAttribute("g", &color[1]);
                subElement->QueryFloatAttribute("b", &color[2]);
                subElement->QueryFloatAttribute("a", &color[3]);
                proj.background = color;
            }

            if (val == "Channels") {
                // @TODO(abock)  In the previous version it was ambiguous whether it should be
                //               initialized to false or true;  it did use 'true' in the end
                //               but I don't think that is the correct way though

                sgct::config::SpoutOutputProjection::Channels c;
                subElement->QueryBoolAttribute("Right", &c.right);
                subElement->QueryBoolAttribute("zLeft", &c.zLeft);
                subElement->QueryBoolAttribute("Bottom", &c.bottom);
                subElement->QueryBoolAttribute("Top", &c.top);
                subElement->QueryBoolAttribute("Left", &c.left);
                subElement->QueryBoolAttribute("zRight", &c.zRight);
                proj.channels = c;
            }

            if (val == "RigOrientation") {
                glm::vec3 orientation;
                subElement->QueryFloatAttribute("pitch", &orientation[0]);
                subElement->QueryFloatAttribute("yaw", &orientation[1]);
                subElement->QueryFloatAttribute("roll", &orientation[2]);
                proj.orientation = orientation;
            }

            subElement = subElement->NextSiblingElement();
        }

        return proj;
    }

    [[nodiscard]] sgct::config::ProjectionPlane parseProjectionPlane(tinyxml2::XMLElement* element) {
        using namespace tinyxml2;
        size_t i = 0;

        sgct::config::ProjectionPlane proj;

        tinyxml2::XMLElement* elem = element->FirstChildElement();
        while (elem) {
            std::string_view val = elem->Value();

            if (val == "Pos") {
                glm::vec3 pos;
                if (elem->QueryFloatAttribute("x", &pos[0]) == XML_NO_ERROR &&
                    elem->QueryFloatAttribute("y", &pos[1]) == XML_NO_ERROR &&
                    elem->QueryFloatAttribute("z", &pos[2]) == XML_NO_ERROR)
                {
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
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ProjectionPlane: Failed to parse coordinates from XML\n"
                    );
                }
            }

            elem = elem->NextSiblingElement();
        }

        return proj;
    }

    [[nodiscard]] sgct::config::Viewport parseViewport(tinyxml2::XMLElement* element) {
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

        if (element->Attribute("tracked")) {
            std::string_view tracked = element->Attribute("tracked");
            viewport.isTracked = (tracked == "true");
        }

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
            using namespace tinyxml2;

            std::string_view val = subElement->Value();
            if (val == "Pos") {
                glm::vec2 position;
                if (subElement->QueryFloatAttribute("x", &position[0]) == XML_NO_ERROR &&
                    subElement->QueryFloatAttribute("y", &position[1]) == XML_NO_ERROR)
                {
                    viewport.position = position;
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "Viewport: Failed to parse position from XML\n"
                    );
                }
            }
            else if (val == "Size") {
                glm::vec2 size;
                if (subElement->QueryFloatAttribute("x", &size[0]) == XML_NO_ERROR &&
                    subElement->QueryFloatAttribute("y", &size[1]) == XML_NO_ERROR)
                {
                    viewport.size = size;
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "Viewport: Failed to parse size from XML!\n"
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

            // iterate
            subElement = subElement->NextSiblingElement();
        }

        return viewport;
    }


    sgct::config::Scene parseScene(tinyxml2::XMLElement* element) {
        using namespace sgct::core;
        using namespace tinyxml2;

        sgct::config::Scene scene;

        XMLElement* child = element->FirstChildElement();
        while (child) {
            std::string_view childVal = child->Value();

            if (childVal == "Offset") {
                scene.offset = *parseValueVec3(*child);
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

    [[nodiscard]] sgct::config::Window parseWindow(tinyxml2::XMLElement* element, std::string xmlFileName)
    {
        using namespace tinyxml2;

        sgct::config::Window window;

        if (element->Attribute("name")) {
            window.name = element->Attribute("name");
        }

        if (element->Attribute("tags")) {
            std::string tags = element->Attribute("tags");
            std::vector<std::string> t = sgct::helpers::split(tags, ',');
            window.tags = t;
        }

        if (element->Attribute("bufferBitDepth")) {
            window.bufferBitDepth =
                getBufferColorBitDepth(element->Attribute("bufferBitDepth"));
        }

        if (element->Attribute("preferBGR")) {
            std::string_view v = element->Attribute("preferBGR");
            window.preferBGR = (v == "true");
        }
        // @TODO (abock, 2019-09-22) replace these construct with calls to
        // parseValue<bool> function

        // compatibility with older versions
        if (element->Attribute("fullScreen")) {
            std::string_view v = element->Attribute("fullScreen");
            window.isFullScreen = (v == "true");
        }

        if (element->Attribute("fullscreen")) {
            std::string_view v = element->Attribute("fullscreen");
            window.isFullScreen = (v == "true");
        }

        if (element->Attribute("floating")) {
            std::string_view v = element->Attribute("floating");
            window.isFloating = (v == "true");
        }

        if (element->Attribute("alwaysRender")) {
            std::string_view v = element->Attribute("alwaysRender");
            window.alwaysRender = (v == "true");
        }

        if (element->Attribute("hidden")) {
            std::string_view v = element->Attribute("hidden");
            window.isHidden = (v == "true");
        }

        if (element->Attribute("dbuffered")) {
            std::string_view v = element->Attribute("dbuffered");
            window.doubleBuffered = (v == "true");
        }

        window.gamma = parseValue<float>(*element, "gamma");
        window.contrast = parseValue<float>(*element, "contrast");
        window.brightness = parseValue<float>(*element, "brightness");
        window.msaa = parseValue<int>(*element, "numberOfSamples");

        std::optional<int> numberOfSamples = parseValue<int>(*element, "numberOfSamples");
        if (numberOfSamples) {
            window.msaa = numberOfSamples;
        }
        std::optional<int> msaa = parseValue<int>(*element, "msaa");
        if (msaa) {
            window.msaa = msaa;
        }
        std::optional<int> MSAA = parseValue<int>(*element, "MSAA");
        if (MSAA) {
            window.msaa = MSAA;
        }

        if (element->Attribute("alpha")) {
            std::string_view v = element->Attribute("alpha");
            window.hasAlpha = (v == "true");
        }

        if (element->Attribute("fxaa")) {
            std::string_view v = element->Attribute("fxaa");
            window.useFxaa = (v == "true");
        }

        if (element->Attribute("FXAA")) {
            std::string_view v = element->Attribute("FXAA");
            window.useFxaa = (v == "true");
        }

        if (element->Attribute("decorated")) {
            std::string_view v = element->Attribute("decorated");
            window.isDecorated = (v == "true");
        }

        if (element->Attribute("border")) {
            std::string_view v = element->Attribute("border");
            window.hasBorder = (v == "true");
        }

        if (element->Attribute("draw2D")) {
            std::string_view v = element->Attribute("draw2D");
            window.draw2D = (v == "true");
        }

        if (element->Attribute("draw3D")) {
            std::string_view v = element->Attribute("draw3D");
            window.draw3D = (v == "true");
        }

        if (element->Attribute("copyPreviousWindowToCurrentWindow")) {
            std::string_view v = element->Attribute("copyPreviousWindowToCurrentWindow");
            window.copyPreviousWindowToCurrentWindow = (v == "true");
        }

        window.monitor = parseValue<int>(*element, "monitor");

        if (element->Attribute("mpcdi")) {
            std::string path;
            size_t lastSlashPos = xmlFileName.find_last_of("/");
            if (lastSlashPos != std::string::npos) {
                path = xmlFileName.substr(0, lastSlashPos) + "/";
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
                window.size = parseValueIVec2(*child);
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

    [[nodiscard]] sgct::config::Node parseNode(tinyxml2::XMLElement* element, std::string xmlFileName) {
        sgct::config::Node node;
        if (element->Attribute("address")) {
            node.address = element->Attribute("address");
        }
        if (element->Attribute("ip")) {
            node.address = element->Attribute("ip");
        }
        if (element->Attribute("name")) {
            node.name = element->Attribute("name");
        }
        if (element->Attribute("port")) {
            node.port = parseValue<int>(*element, "port");
        }
        if (element->Attribute("syncPort")) {
            node.port = parseValue<int>(*element, "syncPort");
        }
        if (element->Attribute("dataTransferPort")) {
            node.dataTransferPort = parseValue<int>(*element, "dataTransferPort");
        }
        if (element->Attribute("swapLock")) {
            std::string_view v = element->Attribute("swapLock");
            node.swapLock = (v == "true");
        }

        tinyxml2::XMLElement* child = element->FirstChildElement();
        while (child) {
            std::string_view childVal = child->Value();
            if (childVal == "Window") {
                sgct::config::Window window = parseWindow(child, xmlFileName);
                node.windows.push_back(window);
            }
            child = child->NextSiblingElement();
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
                user.orientation = parseOrientationNode(child);
            }
            else if (childVal == "Matrix") {
                sgct::config::User::Transform trans;
                if (child->Attribute("transpose")) {
                    trans.transpose = strcmp(child->Attribute("transpose"), "true") == 0;
                }
                trans.transformation = *parseValueMat4(*child);
                user.transformation = trans;
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


    [[nodiscard]] sgct::config::Settings parseSettings(tinyxml2::XMLElement* element) {
        sgct::config::Settings settings;

        tinyxml2::XMLElement* elem = element->FirstChildElement();
        while (elem) {
            std::string_view val = elem->Value();

            if (val == "DepthBufferTexture") {
                if (elem->Attribute("value")) {
                    std::string_view v = elem->Attribute("value");
                    settings.useDepthTexture = (v == "true");
                }
            }
            else if (val == "NormalTexture") {
                if (elem->Attribute("value")) {
                    std::string_view v = elem->Attribute("value");
                    settings.useNormalTexture = (v == "true");
                }
            }
            else if (val == "PositionTexture") {
                if (elem->Attribute("value")) {
                    std::string_view v = elem->Attribute("value");
                    settings.usePositionTexture = (v == "true");
                }
            }
            else if (val == "PBO") {
                if (elem->Attribute("value")) {
                    std::string_view v = elem->Attribute("value");
                    settings.usePBO = (v == "true");
                }
            }
            else if (val == "Precision") {
                std::optional<float> f = parseValue<float>(*elem, "float");
                if (f) {
                    if (*f == 16) {
                        settings.bufferFloatPrecision = sgct::config::Settings::BufferFloatPrecision::Float16Bit;
                    }
                    else if (*f == 32) {
                        settings.bufferFloatPrecision = sgct::config::Settings::BufferFloatPrecision::Float32Bit;
                    }
                    else {
                        sgct::MessageHandler::instance()->print(
                            sgct::MessageHandler::Level::Warning,
                            "ReadConfig: Invalid precision value (%d)! Must be 16 or 32\n",
                            *f
                        );
                    }
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Warning,
                        "ReadConfig: Invalid precision value! Must be 16 or 32\n"
                    );
                }
            }
            else if (val == "Display") {
                sgct::config::Settings::Display display;
                display.swapInterval = parseValue<int>(*elem, "swapInterval");
                display.refreshRate = parseValue<int>(*elem, "refreshRate");
                if (elem->Attribute("tryMaintainAspectRatio")) {
                    std::string_view v = elem->Attribute("tryMaintainAspectRatio");
                    display.maintainAspectRatio = (v == "true");
                }
                if (elem->Attribute("exportWarpingMeshes")) {
                    std::string_view v = elem->Attribute("exportWarpingMeshes");
                    display.exportWarpingMeshes = (v == "true");
                }
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
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Warning,
                        "ReadConfig: Unknown capturing format. Using PNG\n"
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
                device.orientation = parseOrientationNode(child);
            }
            else if (childVal == "Matrix") {
                sgct::config::Device::Transform trans;
                if (child->Attribute("transpose")) {
                    trans.transpose = strcmp(child->Attribute("transpose"), "true") == 0;
                }
                trans.transformation = *parseValueMat4(*child);
                device.transformation = trans;
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
                tracker.orientation = parseOrientationNode(child);
            }
            else if (childVal == "Scale") {
                tracker.scale = parseValue<double>(*child, "value");
            }
            else if (childVal == "Matrix") {
                sgct::config::Tracker::Transform trans;
                if (child->Attribute("transpose")) {
                    trans.transpose = strcmp(child->Attribute("transpose"), "true") == 0;
                }
                trans.transformation = *parseValueMat4(*child);
                tracker.transformation = trans;
            }
            child = child->NextSiblingElement();
        }

        return tracker;
    }

    [[nodiscard]] sgct::config::Cluster readAndParseXML(tinyxml2::XMLDocument& xmlDoc, const std::string& filename) {
        using namespace tinyxml2;

        sgct::config::Cluster cluster;

        XMLElement* XMLroot = xmlDoc.FirstChildElement("Cluster");
        if (XMLroot == nullptr) {
            throw std::runtime_error("Cannot find XML root");
        }

        const char* masterAddress = XMLroot->Attribute("masterAddress");
        if (!masterAddress) {
            throw std::runtime_error("Cannot find master address or DNS name in XML");
        }
        cluster.masterAddress = masterAddress;

        if (XMLroot->Attribute("debug")) {
            std::string_view v = XMLroot->Attribute("debug");
            cluster.debug = (v == "true");
        }

        cluster.externalControlport = parseValue<int>(*XMLroot, "externalControlPort");

        if (XMLroot->Attribute("firmSync")) {
            std::string_view v = XMLroot->Attribute("firmSync");
            cluster.firmSync = (v == "true");
        }

        XMLElement* element = XMLroot->FirstChildElement();
        while (element) {
            std::string_view val = element->Value();

            if (val == "Scene") {
                sgct::config::Scene scene = parseScene(element);
                cluster.scene = scene;
            }
            else if (val == "Node") {
                sgct::config::Node node = parseNode(element, filename);
                cluster.nodes.push_back(node);
            }
            else if (val == "User") {
                sgct::config::User user = parseUser(element);
                cluster.user = user;
            }
            else if (val == "Settings") {
                sgct::config::Settings settings = parseSettings(element);
                cluster.settings = settings;
            }
            else if (val == "Capture") {
                sgct::config::Capture capture = parseCapture(element);
                cluster.capture = capture;
            }
            else if (val == "Tracker" && element->Attribute("name")) {
                sgct::config::Tracker tracker = parseTracker(element);
                cluster.tracker = tracker;
            }
            element = element->NextSiblingElement();
        }

        return cluster;
    }

    [[nodiscard]] sgct::config::Cluster readAndParseXMLFile(const std::string& filename) {
        if (filename.empty()) {
            throw std::runtime_error("No XML file set");
        }

        tinyxml2::XMLDocument xmlDoc;
        bool s = xmlDoc.LoadFile(filename.c_str()) == tinyxml2::XML_NO_ERROR;
        if (!s) {
            throw std::runtime_error(
                "Error loading XML file '" + filename + "'. " +
                xmlDoc.GetErrorStr1() + ' ' + xmlDoc.GetErrorStr2()
            );
        }
        return readAndParseXML(xmlDoc, filename);
    }

    std::string replaceEnvVars(const std::string& filename) {
        size_t foundPercentage = filename.find('%');
        if (foundPercentage != std::string::npos) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "ReadConfig: SGCT doesn't support the usage of '%%' in the path\n"
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
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "ReadConfig: Error: Bad configuration path string\n"
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
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Error: Cannot fetch environment variable '%s'\n",
                        envVar.c_str()
                    );
                    return std::string();
                }
#else
                char* fetchedEnvVar = getenv(envVar.c_str());
                if (fetchedEnvVar == nullptr) {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Error: Cannot fetch environment variable '%s'\n",
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
    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "ReadConfig: Parsing XML config '%s'...\n", f.c_str()
    );

    f = replaceEnvVars(f);
    if (f.empty()) {
        throw std::runtime_error("Could not resolve file path");
    }

    sgct::config::Cluster cluster = readAndParseXMLFile(f);

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "ReadConfig: Config file '%s' read successfully\n", f.c_str()
    );
    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "ReadConfig: Number of nodes in cluster: %d\n", cluster.nodes.size()
    );

    for (size_t i = 0; i < cluster.nodes.size(); i++) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "\tNode(%d) address: %s [%d]\n", i,
            cluster.nodes[i].address->c_str(), *cluster.nodes[i].port
        );
    }

    return cluster;
}

} // namespace sgct_config::readconfig
