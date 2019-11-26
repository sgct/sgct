/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/readconfig.h>

#include <sgct/error.h>
#include <sgct/messagehandler.h>
#include <sgct/helpers/stringfunctions.h>
#include <glm/gtc/type_ptr.hpp>
#include <tinyxml2.h>
#include <algorithm>
#include <unordered_map>

#define Error(code, msg) sgct::Error(sgct::Error::Component::ReadConfig, code, msg)

namespace {
    glm::quat parseOrientationNode(tinyxml2::XMLElement& element) {
        float x = 0.f;
        float y = 0.f;
        float z = 0.f;

        bool eulerMode = false;
        bool quatMode = false;

        glm::quat quat = glm::quat(1.f, 0.f, 0.f, 0.f);

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
        if (type == "active" || type == "quadbuffer") {
            return sgct::config::Window::StereoMode::Active;
        }
        if (type == "checkerboard") {
            return sgct::config::Window::StereoMode::Checkerboard;
        }
        if (type == "checkerboard_inverted") {
            return sgct::config::Window::StereoMode::CheckerboardInverted;
        }
        if (type == "anaglyph_red_cyan") {
            return sgct::config::Window::StereoMode::AnaglyphRedCyan;
        }
        if (type == "anaglyph_amber_blue") {
            return sgct::config::Window::StereoMode::AnaglyphAmberBlue;
        }
        if (type == "anaglyph_wimmer") {
            return sgct::config::Window::StereoMode::AnaglyphRedCyanWimmer;
        }
        if (type == "vertical_interlaced") {
            return sgct::config::Window::StereoMode::VerticalInterlaced;
        }
        if (type == "vertical_interlaced_inverted") {
            return sgct::config::Window::StereoMode::VerticalInterlacedInverted;
        }
        if (type == "test" || type == "dummy") {
            return sgct::config::Window::StereoMode::Dummy;
        }
        if (type == "side_by_side") {
            return sgct::config::Window::StereoMode::SideBySide;
        }
        if (type == "side_by_side_inverted") {
            return sgct::config::Window::StereoMode::SideBySideInverted;
        }
        if (type == "top_bottom") {
            return sgct::config::Window::StereoMode::TopBottom;
        }
        if (type == "top_bottom_inverted") {
            return sgct::config::Window::StereoMode::TopBottomInverted;
        }

        sgct::MessageHandler::printError("Unknown stereo mode %s", type.c_str());
        return sgct::config::Window::StereoMode::NoStereo;
    }

    sgct::config::Window::ColorBitDepth getBufferColorBitDepth(const std::string& type) {
        if (type == "8") {
            return sgct::config::Window::ColorBitDepth::Depth8;
        }
        if (type == "16") {
            return sgct::config::Window::ColorBitDepth::Depth16;
        }
        if (type == "16f") {
            return sgct::config::Window::ColorBitDepth::Depth16Float;
        }
        if (type == "32f") {
            return sgct::config::Window::ColorBitDepth::Depth32Float;
        }
        if (type == "16i") {
            return sgct::config::Window::ColorBitDepth::Depth16Int;
        }
        if (type == "32i") {
            return sgct::config::Window::ColorBitDepth::Depth32Int;
        }
        if (type == "16ui") {
            return sgct::config::Window::ColorBitDepth::Depth16UInt;
        }
        if (type == "32ui") {
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
        if (it != Map.cend()) {
            return it->second;
        }
        else {
            sgct::MessageHandler::printError("Unknown resolution %s", quality.c_str());
            return -1;
        }
    }

    std::optional<glm::ivec2> parseValueIVec2(const tinyxml2::XMLElement& e) {
        glm::ivec2 value;
        bool xe = e.QueryIntAttribute("x", &value[0]) == tinyxml2::XML_NO_ERROR;
        bool ye = e.QueryIntAttribute("y", &value[1]) == tinyxml2::XML_NO_ERROR;
        return (xe && ye) ? std::optional(value) : std::nullopt;
    }

    std::optional<glm::vec2> parseValueVec2(const tinyxml2::XMLElement& e) {
        glm::vec2 value;
        bool xe = e.QueryFloatAttribute("x", &value[0]) == tinyxml2::XML_NO_ERROR;
        bool ye = e.QueryFloatAttribute("y", &value[1]) == tinyxml2::XML_NO_ERROR;
        return (xe && ye) ? std::optional(value) : std::nullopt;
    }

    std::optional<glm::vec3> parseValueVec3(const tinyxml2::XMLElement& e) {
        glm::vec3 value;
        bool xe = e.QueryFloatAttribute("x", &value[0]) == tinyxml2::XML_NO_ERROR;
        bool ye = e.QueryFloatAttribute("y", &value[1]) == tinyxml2::XML_NO_ERROR;
        bool ze = e.QueryFloatAttribute("z", &value[2]) == tinyxml2::XML_NO_ERROR;
        return (xe && ye && ze) ? std::optional(value) : std::nullopt;
    }

    std::optional<glm::vec4> parseValueColor(const tinyxml2::XMLElement& e) {
        using namespace tinyxml2;

        glm::vec4 value;
        bool re = e.QueryFloatAttribute("r", &value[0]) == XML_NO_ERROR;
        bool ge = e.QueryFloatAttribute("g", &value[1]) == XML_NO_ERROR;
        bool be = e.QueryFloatAttribute("b", &value[2]) == XML_NO_ERROR;
        bool ae = e.QueryFloatAttribute("a", &value[3]) == XML_NO_ERROR;
        return (re && ge && be && ae) ? std::optional(value) : std::nullopt;
    }

    std::optional<glm::mat4> parseValueMat4(const tinyxml2::XMLElement& e) {
        float value[16];
        bool err[16] = {
            e.QueryFloatAttribute("x0", &value[0]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("y0", &value[1]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("z0", &value[2]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("w0", &value[3]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("x1", &value[4]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("y1", &value[5]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("z1", &value[6]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("w1", &value[7]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("x2", &value[8]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("y2", &value[9]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("z2", &value[10]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("w2", &value[11]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("x3", &value[12]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("y3", &value[13]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("z3", &value[14]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("w3", &value[15]) == tinyxml2::XML_NO_ERROR
        };
        
        bool suc = std::all_of(std::begin(err), std::end(err), [](bool v) { return v; });
        return suc ? std::optional(glm::make_mat4(value)) : std::nullopt;
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
        sgct::config::PlanarProjection proj;
        tinyxml2::XMLElement* fovElement = element.FirstChildElement("FOV");
        if (fovElement == nullptr) {
            throw Error(6000, "Missing specification of field-of-view values");
        }

        std::optional<float> down = parseValue<float>(*fovElement, "down");
        std::optional<float> left = parseValue<float>(*fovElement, "left");
        std::optional<float> right = parseValue<float>(*fovElement, "right");
        std::optional<float> up = parseValue<float>(*fovElement, "up");

        if (down && left && right && up) {
            // The negative signs here were lifted up from the viewport class. I think it
            // is nicer to store them in negative values and consider the fact that the
            // down and left fovs are inverted to be a detail of the XML specification
            proj.fov.down = -*down;
            proj.fov.left = -*left;
            proj.fov.right = *right;
            proj.fov.up = *up;
        }
        else {
            throw Error(6001, "Failed to parse planar projection FOV");
        }
        proj.fov.distance = parseValue<float>(*fovElement, "distance");

        if (tinyxml2::XMLElement* e = element.FirstChildElement("Orientation"); e) {
            proj.orientation = parseOrientationNode(*e);
        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("Offset"); e) {
            proj.offset = parseValueVec3(*e);
        }

        return proj;
    }

    sgct::config::FisheyeProjection parseFisheyeProjection(tinyxml2::XMLElement& elem) {
        sgct::config::FisheyeProjection proj;

        proj.fov = parseValue<float>(elem, "fov");
        if (const char* a = elem.Attribute("quality"); a) {
            proj.quality = cubeMapResolutionForQuality(a);
        }
        if (const char* a = elem.Attribute("method"); a) {
            proj.method = std::string_view(a) == "five_face_cube" ?
                sgct::config::FisheyeProjection::Method::FiveFace :
                sgct::config::FisheyeProjection::Method::FourFace;
        }
        if (const char* a = elem.Attribute("interpolation"); a) {
            proj.interpolation = std::string_view(a) == "cubic" ?
                sgct::config::FisheyeProjection::Interpolation::Cubic :
                sgct::config::FisheyeProjection::Interpolation::Linear;
        }
        proj.diameter = parseValue<float>(elem, "diameter");
        proj.tilt = parseValue<float>(elem, "tilt");

        if (tinyxml2::XMLElement* e = elem.FirstChildElement("Crop"); e) {
            sgct::config::FisheyeProjection::Crop crop;
            if (std::optional<float> v = parseValue<float>(*e, "left"); v) {
                crop.left = *v;
            }
            if (std::optional<float> v = parseValue<float>(*e, "right"); v) {
                crop.right = *v;
            }
            if (std::optional<float> v = parseValue<float>(*e, "bottom"); v) {
                crop.bottom = *v;
            }
            if (std::optional<float> v = parseValue<float>(*e, "top"); v) {
                crop.top = *v;
            }
            proj.crop = crop;
        }

        if (tinyxml2::XMLElement* e = elem.FirstChildElement("Offset"); e) {
            proj.offset = parseValueVec3(*e);
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("Background"); e) {
            proj.background = parseValueColor(*e);
        }

        return proj;
    }

    sgct::config::SphericalMirrorProjection parseSphericalMirrorProjection(
                                                            tinyxml2::XMLElement& element)
    {
        sgct::config::SphericalMirrorProjection proj;
        if (const char* v = element.Attribute("quality"); v) {
            proj.quality = cubeMapResolutionForQuality(v);
        }

        proj.tilt = parseValue<float>(element, "tilt");

        if (tinyxml2::XMLElement* e = element.FirstChildElement("Background"); e) {
            proj.background = parseValueColor(*e);
        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("Geometry"); e) {
            if (const char* a = e->Attribute("bottom"); a) {
                proj.mesh.bottom = a;
            }
            if (const char* a = e->Attribute("left"); a) {
                proj.mesh.left = a;
            }
            if (const char* a = e->Attribute("right"); a) {
                proj.mesh.right = a;
            }
            if (const char* a = e->Attribute("top"); a) {
                proj.mesh.top = a;
            }
        }

        return proj;
    }

    sgct::config::SpoutOutputProjection parseSpoutOutputProjection(
                                                            tinyxml2::XMLElement& element)
    {
        sgct::config::SpoutOutputProjection proj;

        if (const char* a = element.Attribute("quality"); a) {
            proj.quality = cubeMapResolutionForQuality(a);
        }
        if (const char* a = element.Attribute("mapping"); a) {
            using namespace sgct::config;
            std::string_view val = a;
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
        if (const char* a = element.Attribute("mappingSpoutName"); a) {
            proj.mappingSpoutName = a;
        }

        if (tinyxml2::XMLElement* e = element.FirstChildElement("Background"); e) {
            proj.background = parseValueColor(*e);
        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("Channels"); e) {
            sgct::config::SpoutOutputProjection::Channels c;
            c.right = *parseValue<bool>(*e, "Right");
            c.zLeft = *parseValue<bool>(*e, "zLeft");
            c.bottom = *parseValue<bool>(*e, "Bottom");
            c.top = *parseValue<bool>(*e, "Top");
            c.left = *parseValue<bool>(*e, "Left");
            c.zRight = *parseValue<bool>(*e, "zRight");
            proj.channels = c;
        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("RigOrientation"); e) {
            glm::vec3 orientation = glm::vec3(
                *parseValue<float>(*e, "pitch"),
                *parseValue<float>(*e, "yaw"),
                *parseValue<float>(*e, "roll")
            );
            proj.orientation = orientation;
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
            throw Error(6010, "Failed parsing coordinates. Missing XML children");
        }
        std::optional<glm::vec3> p1 = parseValueVec3(*c1);
        std::optional<glm::vec3> p2 = parseValueVec3(*c2);
        std::optional<glm::vec3> p3 = parseValueVec3(*c3);
        if (!(p1 && p2 && p3)) {
            throw Error(6011, "Failed parsing ProjectionPlane coordinates. Type error");
        }

        sgct::config::ProjectionPlane proj;
        proj.lowerLeft = *p1;
        proj.upperLeft = *p2;
        proj.upperRight = *p3;
        return proj;
    }

    sgct::config::Viewport parseViewport(tinyxml2::XMLElement& elem) {
        sgct::config::Viewport viewport;
        if (const char* a = elem.Attribute("user"); a) {
            viewport.user = a;
        }
        if (const char* a = elem.Attribute("name"); a) {
            viewport.name = a;
        }
        if (const char* a = elem.Attribute("overlay"); a) {
            viewport.overlayTexture = a;
        }
        if (const char* a = elem.Attribute("mask"); a) {
            viewport.blendMaskTexture = a;
        }
        if (const char* a = elem.Attribute("BlendMask"); a) {
            viewport.blendMaskTexture = a;
        }
        if (const char* a = elem.Attribute("BlackLevelMask"); a) {
            viewport.blendLevelMaskTexture = a;
        }
        if (const char* a = elem.Attribute("mesh"); a) {
            viewport.correctionMeshTexture = a;
        }
        if (const char* a = elem.Attribute("hint"); a) {
            viewport.meshHint = a;
        }

        viewport.isTracked = parseValue<bool>(elem, "tracked");

        // get eye if set
        if (const char* a = elem.Attribute("eye"); a) {
            std::string_view eye = a;
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

        if (tinyxml2::XMLElement* e = elem.FirstChildElement("Pos"); e) {
            if (std::optional<glm::vec2> pos = parseValueVec2(*e); pos) {
                viewport.position = *pos;
            }
            else {
                throw Error(6020, "Failed to parse position. Type error");
            }
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("Size"); e) {
            if (std::optional<glm::vec2> size = parseValueVec2(*e); size) {
                viewport.size = *size;
            }
            else {
                throw Error(6021, "Failed to parse size. Type error");
            }
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("PlanarProjection"); e) {
            viewport.projection = parsePlanarProjection(*e);
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("FisheyeProjection"); e) {
            viewport.projection = parseFisheyeProjection(*e);
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("SphericalMirrorProjection");
            e)
        {
            viewport.projection = parseSphericalMirrorProjection(*e);
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("SpoutOutputProjection"); e)
        {
            viewport.projection = parseSpoutOutputProjection(*e);
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("Viewplane"); e) {
            viewport.projection = parseProjectionPlane(*e);
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("Projectionplane"); e) {
            viewport.projection = parseProjectionPlane(*e);
        }

        return viewport;
    }

    sgct::config::Scene parseScene(tinyxml2::XMLElement& element) {
        sgct::config::Scene scene;

        if (tinyxml2::XMLElement* e = element.FirstChildElement("Offset"); e) {
            scene.offset = parseValueVec3(*e);
        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("Orientation"); e) {
            scene.orientation = parseOrientationNode(*e);
        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("Scale"); e) {
            scene.scale = parseValue<float>(*e, "value");
        }

        return scene;
    }

    sgct::config::Window parseWindow(tinyxml2::XMLElement& elem, const std::string& f) {
        sgct::config::Window window;

        if (const char* a = elem.Attribute("name"); a) {
            window.name = a;
        }
        if (const char* a = elem.Attribute("tags"); a) {
            window.tags = sgct::helpers::split(a, ',');
        }
        if (const char* a = elem.Attribute("bufferBitDepth"); a) {
            window.bufferBitDepth = getBufferColorBitDepth(a);
        }
        // compatibility with older versions
        if (elem.Attribute("fullScreen")) {
            window.isFullScreen = parseValue<bool>(elem, "fullScreen");
        }
        if (elem.Attribute("fullscreen")) {
            window.isFullScreen = parseValue<bool>(elem, "fullscreen");
        }

        window.isFloating = parseValue<bool>(elem, "floating");
        window.alwaysRender = parseValue<bool>(elem, "alwaysRender");
        window.isHidden = parseValue<bool>(elem, "hidden");
        window.doubleBuffered = parseValue<bool>(elem, "dbuffered");
        window.gamma = parseValue<float>(elem, "gamma");
        window.contrast = parseValue<float>(elem, "contrast");
        window.brightness = parseValue<float>(elem, "brightness");

        window.msaa = parseValue<int>(elem, "numberOfSamples");
        if (elem.Attribute("msaa")) {
            window.msaa = parseValue<int>(elem, "msaa");
        }
        if (elem.Attribute("MSAA")) {
            window.msaa = parseValue<int>(elem, "MSAA");
        }
        window.hasAlpha = parseValue<bool>(elem, "alpha");

        if (elem.Attribute("fxaa")) {
            window.useFxaa = parseValue<bool>(elem, "fxaa");
        }
        if (elem.Attribute("FXAA")) {
            window.useFxaa = parseValue<bool>(elem, "FXAA");
        }

        window.isDecorated = parseValue<bool>(elem, "decorated");
        window.hasBorder = parseValue<bool>(elem, "border");
        window.draw2D = parseValue<bool>(elem, "draw2D");
        window.draw3D = parseValue<bool>(elem, "draw3D");
        window.blitPreviousWindow = parseValue<bool>(elem, "blitPreviousWindow");
        window.monitor = parseValue<int>(elem, "monitor");

        if (const char* a = elem.Attribute("mpcdi"); a) {
            std::string path;
            const size_t lastSlashPos = f.find_last_of("/");
            if (lastSlashPos != std::string::npos) {
                path = f.substr(0, lastSlashPos) + "/";
            }
            path += a;
            std::replace(path.begin(), path.end(), '\\', '/');
            window.mpcdi = path;
        }

        if (tinyxml2::XMLElement* e = elem.FirstChildElement("Stereo"); e) {
            window.stereo = getStereoType(e->Attribute("type"));
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("Pos"); e) {
            window.pos = parseValueIVec2(*e);
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("Size"); e) {
            if (std::optional<glm::ivec2> s = parseValueIVec2(*e); s) {
                window.size = *s;
            }
            else {
                throw Error(6030, "Could not parse window size. Type error");
            }
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("Res"); e) {
            window.resolution = parseValueIVec2(*e);
        }

        tinyxml2::XMLElement* vp = elem.FirstChildElement("Viewport");
        while (vp) {
            window.viewports.push_back(parseViewport(*vp));
            vp = vp->NextSiblingElement("Viewport");
        }

        return window;
    }

    sgct::config::Node parseNode(tinyxml2::XMLElement& elem, const std::string& file) {
        sgct::config::Node node;
        if (const char* a = elem.Attribute("address"); a) {
            node.address = a;
        }
        else {
            throw Error(6040, "Missing field address in node");
        }
        if (elem.Attribute("port")) {
            node.port = *parseValue<int>(elem, "port");
        }
        else {
            throw Error(6041, "Missing field port in node");
        }
        node.dataTransferPort = parseValue<int>(elem, "dataTransferPort");
        node.swapLock = parseValue<bool>(elem, "swapLock");

        tinyxml2::XMLElement* wnd = elem.FirstChildElement("Window");
        while (wnd) {
            sgct::config::Window window = parseWindow(*wnd, file);
            node.windows.push_back(window);
            wnd = wnd->NextSiblingElement("Window");
        }

        return node;
    }

    sgct::config::User parseUser(tinyxml2::XMLElement& element) {
        sgct::config::User user;
        if (const char* a = element.Attribute("name"); a) {
            user.name = a;
        }
        user.eyeSeparation = parseValue<float>(element, "eyeSeparation");

        if (tinyxml2::XMLElement* e = element.FirstChildElement("Pos"); e) {
            user.position = parseValueVec3(*e);
        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("Orientation"); e) {
            user.transformation = glm::mat4_cast(parseOrientationNode(*e));
        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("Matrix"); e) {
            user.transformation = parseValueMat4(*e);
            if (user.transformation) {
                if (std::optional<bool> t = parseValue<bool>(*e, "transpose"); t && *t) {
                    user.transformation = glm::transpose(*user.transformation);
                }
            }
        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("Tracking"); e) {
            sgct::config::User::Tracking tracking;
            tracking.tracker = e->Attribute("tracker");
            tracking.device = e->Attribute("device");
            user.tracking = tracking;
        }

        return user;
    }

    sgct::config::Settings parseSettings(tinyxml2::XMLElement& elem) {
        sgct::config::Settings settings;

        if (tinyxml2::XMLElement* e = elem.FirstChildElement("DepthBufferTexture"); e) {
            settings.useDepthTexture = parseValue<bool>(*e, "value");
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("NormalTexture"); e) {
            settings.useNormalTexture = parseValue<bool>(*e, "value");
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("PositionTexture"); e) {
            settings.usePositionTexture = parseValue<bool>(*e, "value");
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("Precision"); e) {
            std::optional<float> f = parseValue<float>(*e, "float");
            if (f && *f == 16.f) {
                settings.bufferFloatPrecision =
                    sgct::config::Settings::BufferFloatPrecision::Float16Bit;
            }
            else if (f && *f == 32) {
                settings.bufferFloatPrecision =
                    sgct::config::Settings::BufferFloatPrecision::Float32Bit;
            }
            else if (f) {
                throw Error(6050, "Wrong buffer precision value " + std::to_string(*f));
            }
            else {
                throw Error(6051, "Wrong buffer precision value type");
            }
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("Display"); e) {
            sgct::config::Settings::Display display;
            display.swapInterval = parseValue<int>(*e, "swapInterval");
            display.refreshRate = parseValue<int>(*e, "refreshRate");
            display.keepAspectRatio = parseValue<bool>(*e, "tryMaintainAspectRatio");
            display.exportWarpingMeshes = parseValue<bool>(*e, "exportWarpingMeshes");
            settings.display = display;
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("OSDText"); e) {
            sgct::config::Settings::OSDText osdText;
            if (const char* a = e->Attribute("name"); a) {
                osdText.name = a;
            }
            if (const char* a = e->Attribute("path"); a) {
                osdText.path = a;
            }
            osdText.size = parseValue<int>(*e, "size");
            osdText.xOffset = parseValue<float>(*e, "xOffset");
            osdText.yOffset = parseValue<float>(*e, "yOffset");
            settings.osdText = osdText;
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("FXAA"); e) {
            sgct::config::Settings::FXAA fxaa;
            fxaa.offset = parseValue<float>(*e, "offset");
            fxaa.trim = parseValue<float>(*e, "trim");
            settings.fxaa = fxaa;
        }

        return settings;
    }

    sgct::config::Capture parseCapture(tinyxml2::XMLElement& element) {
        sgct::config::Capture res;
        if (const char* a = element.Attribute("path"); a) {
            res.monoPath = a;
            res.leftPath = a;
            res.rightPath = a;
        }
        if (const char* a = element.Attribute("monoPath"); a) {
            res.monoPath = a;
        }
        if (const char* a = element.Attribute("leftPath"); a) {
            res.leftPath = a;
        }
        if (const char* a = element.Attribute("rightPath"); a) {
            res.rightPath = a;
        }
        if (const char* a = element.Attribute("format"); a) {
            res.format = [](std::string_view format) {
                if (format == "png" || format == "PNG") {
                    return sgct::config::Capture::Format::PNG;
                }
                if (format == "tga" || format == "TGA") {
                    return sgct::config::Capture::Format::TGA;
                }
                if (format == "jpg" || format == "JPG") {
                    return sgct::config::Capture::Format::JPG;
                }
                throw Error(6060, "Unknown capturing format");
            }(a);
        }
        return res;
    }

    sgct::config::Device parseDevice(tinyxml2::XMLElement& element) {
        sgct::config::Device device;
        device.name = element.Attribute("name");

        tinyxml2::XMLElement* sensorElem = element.FirstChildElement("Sensor");
        while (sensorElem) {
            sgct::config::Device::Sensors sensors;
            sensors.vrpnAddress = sensorElem->Attribute("vrpnAddress");
            sensors.identifier = *parseValue<int>(*sensorElem, "id");
            device.sensors.push_back(sensors);
            sensorElem = sensorElem->NextSiblingElement("Sensor");
        }
        tinyxml2::XMLElement* buttonElem = element.FirstChildElement("Buttons");
        while (buttonElem) {
            sgct::config::Device::Buttons buttons;
            buttons.vrpnAddress = buttonElem->Attribute("vrpnAddress");
            buttons.count = *parseValue<int>(*buttonElem, "count");
            device.buttons.push_back(buttons);
            buttonElem = buttonElem->NextSiblingElement("Buttons");
        }
        tinyxml2::XMLElement* axesElem = element.FirstChildElement("Axes");
        while (axesElem) {
            sgct::config::Device::Axes axes;
            axes.vrpnAddress = axesElem->Attribute("vrpnAddress");
            axes.count = *parseValue<int>(*axesElem, "count");
            device.axes.push_back(axes);
            axesElem = axesElem->NextSiblingElement("Axes");

        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("Offset"); e) {
            device.offset = parseValueVec3(*e);
        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("Orientation"); e) {
            device.transformation = glm::mat4_cast(parseOrientationNode(*e));
        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("Matrix"); e) {
            device.transformation = parseValueMat4(*e);
            if (device.transformation) {
                if (std::optional<bool> t = parseValue<bool>(*e, "transpose"); t && *t) {
                    device.transformation = glm::transpose(*device.transformation);
                }
            }
        }

        return device;
    }

    sgct::config::Tracker parseTracker(tinyxml2::XMLElement& element) {
        sgct::config::Tracker tracker;
        if (const char* a = element.Attribute("name"); a) {
            tracker.name = a;
        }
        else {
            throw Error(6070, "Tracker is missing 'name'");
        }

        if (tinyxml2::XMLElement* e = element.FirstChildElement("Device"); e) {
            sgct::config::Device device = parseDevice(*e);
            tracker.devices.push_back(device);
        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("Offset"); e) {
            tracker.offset = parseValueVec3(*e);
        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("Orientation"); e) {
            tracker.transformation = glm::mat4_cast(parseOrientationNode(*e));
        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("Scale"); e) {
            tracker.scale = parseValue<double>(*e, "value");
        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("Matrix"); e) {
            tracker.transformation = parseValueMat4(*e);
            if (tracker.transformation) {
                if (std::optional<bool> t = parseValue<bool>(*e, "transpose"); t && *t) {
                    tracker.transformation = glm::transpose(*tracker.transformation);
                }
            }
        }

        return tracker;
    }

    sgct::config::Cluster readAndParseXMLFile(const std::string& filename) {
        if (filename.empty()) {
            throw Error(6080, "No XML file provided");
        }

        tinyxml2::XMLDocument xmlDoc;
        const bool s = xmlDoc.LoadFile(filename.c_str()) == tinyxml2::XML_NO_ERROR;
        if (!s) {
            std::string s1 = xmlDoc.ErrorName() ? xmlDoc.ErrorName() : "";
            std::string s2 = xmlDoc.GetErrorStr1() ? xmlDoc.GetErrorStr1() : "";
            std::string s3 = xmlDoc.GetErrorStr2() ? xmlDoc.GetErrorStr2() : "";
            std::string s4 = s1 + ' ' + s2 + ' ' + s3;
            throw Error(6081, "Error loading XML file '" + filename + "'. " + s4);
        }

        sgct::config::Cluster cluster;
        tinyxml2::XMLElement* xmlRoot = xmlDoc.FirstChildElement("Cluster");
        if (xmlRoot == nullptr) {
            throw Error(6082, "Cannot find 'Cluster' node");
        }
        tinyxml2::XMLElement& root = *xmlRoot;

        if (const char* a = root.Attribute("masterAddress"); a) {
            cluster.masterAddress = a;
        }
        else {
            throw Error(6083, "Cannot find master address or DNS name in XML");
        }

        cluster.debug = parseValue<bool>(root, "debug");
        cluster.checkOpenGL = parseValue<bool>(root, "checkOpenGL");
        cluster.checkFBOs = parseValue<bool>(root, "checkFBO");
        cluster.externalControlPort = parseValue<int>(root, "externalControlPort");
        cluster.firmSync = parseValue<bool>(root, "firmSync");

        if (tinyxml2::XMLElement* e = root.FirstChildElement("Scene"); e) {
            cluster.scene = parseScene(*e);
        }
        tinyxml2::XMLElement* u = root.FirstChildElement("User");
        while (u) {
            sgct::config::User user = parseUser(*u);
            cluster.users.push_back(user);
            u = u->NextSiblingElement("User");
        }
        if (tinyxml2::XMLElement* e = root.FirstChildElement("Settings"); e) {
            cluster.settings = parseSettings(*e);
        }
        if (tinyxml2::XMLElement* e = root.FirstChildElement("Capture"); e) {
            cluster.capture = parseCapture(*e);
        }

        tinyxml2::XMLElement* trackerElem = root.FirstChildElement("Tracker");
        while (trackerElem) {
            sgct::config::Tracker tracker = parseTracker(*trackerElem);
            cluster.trackers.push_back(tracker);
            trackerElem = trackerElem->NextSiblingElement("Tracker");
        }

        tinyxml2::XMLElement* nodeElem = root.FirstChildElement("Node");
        while (nodeElem) {
            sgct::config::Node node = parseNode(*nodeElem, filename);
            cluster.nodes.push_back(node);
            nodeElem = nodeElem->NextSiblingElement("Node");
        }

        return cluster;
    }

    std::string replaceEnvVars(const std::string& filename) {
        size_t foundPercentage = filename.find('%');
        if (foundPercentage != std::string::npos) {
            throw Error(6084, "SGCT doesn't support usage of '%%' in the path");
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
                    envVarLocs.emplace_back(beginLocation, foundIndex);
                }
                else {
                    throw Error(6085, "Bad configuration path string");
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
            if (!environmentSuccess) {
                throw Error(6086, "Cannot fetch environment variable " + envVar);
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

config::Cluster readConfig(const std::string& filename) {
    MessageHandler::printDebug("Parsing XML config '%s'", filename.c_str());

    std::string f = replaceEnvVars(filename);
    config::Cluster cluster = readAndParseXMLFile(f);

    MessageHandler::printDebug("Config file '%s' read successfully", f.c_str());
    MessageHandler::printInfo("Number of nodes in cluster: %d", cluster.nodes.size());

    for (size_t i = 0; i < cluster.nodes.size(); i++) {
        const config::Node& node = cluster.nodes[i];
        MessageHandler::printInfo(
            "\tNode(%d) address: %s [%d]", i, node.address.c_str(), node.port
        );
    }

    return cluster;
}

} // namespace sgct::core
