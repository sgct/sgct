/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/readconfig.h>

#include <sgct/error.h>
#include <sgct/log.h>
#include <sgct/fmt.h>
#include <sgct/math.h>
#include <sgct/tinyxml.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <unordered_map>

#define Err(code, msg) sgct::Error(sgct::Error::Component::ReadConfig, code, msg)

namespace {
    template <typename From, typename To>
    To fromGLM(From v) {
        To r;
        std::memcpy(&r, glm::value_ptr(v), sizeof(To));
        return r;
    }

    std::vector<std::string> split(std::string str, char delimiter) {
        std::vector<std::string> res;
        std::stringstream ss(std::move(str));
        std::string part;
        while (std::getline(ss, part, delimiter)) {
            res.push_back(part);
        }
        return res;
    }

    sgct::quat parseOrientationNode(tinyxml2::XMLElement& element) {
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

        return fromGLM<glm::quat, sgct::quat>(quat);
    }

    sgct::config::Window::StereoMode getStereoType(std::string_view t) {
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

        throw Err(6085, fmt::format("Unkonwn stereo mode {}", t));
    }

    sgct::config::Window::ColorBitDepth getBufferColorBitDepth(std::string_view type) {
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

        throw Err(6087, fmt::format("Unknown resolution {} for cube map", quality));
    }

    std::optional<sgct::ivec2> parseValueIVec2(const tinyxml2::XMLElement& e) {
        sgct::ivec2 value;
        bool xe = e.QueryIntAttribute("x", &value.x) == tinyxml2::XML_NO_ERROR;
        bool ye = e.QueryIntAttribute("y", &value.y) == tinyxml2::XML_NO_ERROR;
        return (xe && ye) ? std::optional(value) : std::nullopt;
    }

    std::optional<sgct::vec2> parseValueVec2(const tinyxml2::XMLElement& e) {
        sgct::vec2 value;
        bool xe = e.QueryFloatAttribute("x", &value.x) == tinyxml2::XML_NO_ERROR;
        bool ye = e.QueryFloatAttribute("y", &value.y) == tinyxml2::XML_NO_ERROR;
        return (xe && ye) ? std::optional(value) : std::nullopt;
    }

    std::optional<sgct::vec3> parseValueVec3(const tinyxml2::XMLElement& e) {
        sgct::vec3 value;
        bool xe = e.QueryFloatAttribute("x", &value.x) == tinyxml2::XML_NO_ERROR;
        bool ye = e.QueryFloatAttribute("y", &value.y) == tinyxml2::XML_NO_ERROR;
        bool ze = e.QueryFloatAttribute("z", &value.z) == tinyxml2::XML_NO_ERROR;
        return (xe && ye && ze) ? std::optional(value) : std::nullopt;
    }

    std::optional<sgct::vec4> parseValueColor(const tinyxml2::XMLElement& e) {
        sgct::vec4 value;
        bool re = e.QueryFloatAttribute("r", &value.x) == tinyxml2::XML_NO_ERROR;
        bool ge = e.QueryFloatAttribute("g", &value.y) == tinyxml2::XML_NO_ERROR;
        bool be = e.QueryFloatAttribute("b", &value.z) == tinyxml2::XML_NO_ERROR;
        bool ae = e.QueryFloatAttribute("a", &value.w) == tinyxml2::XML_NO_ERROR;
        return (re && ge && be && ae) ? std::optional(value) : std::nullopt;
    }

    std::optional<sgct::mat4> parseValueMat4(const tinyxml2::XMLElement& e) {
        sgct::mat4 r;
        bool err[16] = {
            e.QueryFloatAttribute("x0", &r.values[0]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("y0", &r.values[1]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("z0", &r.values[2]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("w0", &r.values[3]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("x1", &r.values[4]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("y1", &r.values[5]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("z1", &r.values[6]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("w1", &r.values[7]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("x2", &r.values[8]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("y2", &r.values[9]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("z2", &r.values[10]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("w2", &r.values[11]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("x3", &r.values[12]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("y3", &r.values[13]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("z3", &r.values[14]) == tinyxml2::XML_NO_ERROR,
            e.QueryFloatAttribute("w3", &r.values[15]) == tinyxml2::XML_NO_ERROR
        };

        bool suc = std::all_of(std::begin(err), std::end(err), [](bool v) { return v; });
        return suc ? std::optional(r) : std::nullopt;
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
            sgct::Log::Error(fmt::format("Error extracting value '{}'", name));
            return std::nullopt;
        }
    }

    sgct::config::PlanarProjection parsePlanarProjection(tinyxml2::XMLElement& element) {
        sgct::config::PlanarProjection proj;
        tinyxml2::XMLElement* fovElement = element.FirstChildElement("FOV");
        if (fovElement == nullptr) {
            throw Err(6000, "Missing specification of field-of-view values");
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
            throw Err(6001, "Failed to parse planar projection FOV");
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

        proj.keepAspectRatio = parseValue<bool>(elem, "keepAspectRatio");

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
        else {
            throw Err(6100, "Missing geometry paths");
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
                throw Err(6086, fmt::format("Unknown spout output mapping: {}", val));
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
            proj.orientation = sgct::vec3{
                *parseValue<float>(*e, "pitch"),
                *parseValue<float>(*e, "yaw"),
                *parseValue<float>(*e, "roll")
            };
        }

        return proj;
    }

    sgct::config::CylindricalProjection parseCylindricalProjection(
                                                            tinyxml2::XMLElement& element)
    {
        sgct::config::CylindricalProjection proj;

        if (const char* a = element.Attribute("quality"); a) {
            proj.quality = cubeMapResolutionForQuality(a);
        }
        proj.rotation = parseValue<float>(element, "rotation");
        proj.heightOffset = parseValue<float>(element, "heightOffset");
        proj.radius = parseValue<float>(element, "radius");

        return proj;
    }

    sgct::config::EquirectangularProjection parseEquirectangularProjection(
                                                            tinyxml2::XMLElement& element)
    {
        sgct::config::EquirectangularProjection proj;
        if (const char* a = element.Attribute("quality"); a) {
            proj.quality = cubeMapResolutionForQuality(a);
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
            throw Err(6010, "Failed parsing coordinates. Missing XML children");
        }
        std::optional<sgct::vec3> p1 = parseValueVec3(*c1);
        std::optional<sgct::vec3> p2 = parseValueVec3(*c2);
        std::optional<sgct::vec3> p3 = parseValueVec3(*c3);
        if (!(p1 && p2 && p3)) {
            throw Err(6011, "Failed parsing ProjectionPlane coordinates. Type error");
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
        if (const char* a = elem.Attribute("overlay"); a) {
            viewport.overlayTexture = std::filesystem::absolute(a).string();
        }
        if (const char* a = elem.Attribute("mask"); a) {
            viewport.blendMaskTexture = std::filesystem::absolute(a).string();
        }
        if (const char* a = elem.Attribute("BlendMask"); a) {
            viewport.blendMaskTexture = std::filesystem::absolute(a).string();
        }
        if (const char* a = elem.Attribute("BlackLevelMask"); a) {
            viewport.blendLevelMaskTexture = std::filesystem::absolute(a).string();
        }
        if (const char* a = elem.Attribute("mesh"); a) {
            viewport.correctionMeshTexture = std::filesystem::absolute(a).string();
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
            else {
                throw Err(6020, "Unrecognized eye position");
            }
        }

        if (tinyxml2::XMLElement* e = elem.FirstChildElement("Pos"); e) {
            if (std::optional<sgct::vec2> pos = parseValueVec2(*e); pos) {
                viewport.position = *pos;
            }
            else {
                throw Err(6021, "Failed to parse position. Type error");
            }
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("Size"); e) {
            if (std::optional<sgct::vec2> size = parseValueVec2(*e); size) {
                viewport.size = *size;
            }
            else {
                throw Err(6022, "Failed to parse size. Type error");
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
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("CylindricalProjection"); e)
        {
            viewport.projection = parseCylindricalProjection(*e);
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("EquirectangularProjection");
            e
        )
        {
            viewport.projection = parseEquirectangularProjection(*e);
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

    sgct::config::Window parseWindow(tinyxml2::XMLElement& elem) {
        sgct::config::Window window;

        if (const char* a = elem.Attribute("name"); a) {
            window.name = a;
        }
        if (const char* a = elem.Attribute("tags"); a) {
            window.tags = split(a, ',');
        }
        if (const char* a = elem.Attribute("bufferBitDepth"); a) {
            window.bufferBitDepth = getBufferColorBitDepth(a);
        }

        window.isFullScreen = parseValue<bool>(elem, "fullscreen");
        window.shouldAutoiconify = parseValue<bool>(elem, "autoiconify");
        window.isFloating = parseValue<bool>(elem, "floating");
        window.alwaysRender = parseValue<bool>(elem, "alwaysRender");
        window.isHidden = parseValue<bool>(elem, "hidden");
        window.doubleBuffered = parseValue<bool>(elem, "dbuffered");

        window.msaa = parseValue<int>(elem, "msaa");
        window.hasAlpha = parseValue<bool>(elem, "alpha");
        window.useFxaa = parseValue<bool>(elem, "fxaa");

        window.isDecorated = parseValue<bool>(elem, "decorated");
        window.isDecorated = parseValue<bool>(elem, "border");
        window.isMirrored = parseValue<bool>(elem, "mirror");
        window.draw2D = parseValue<bool>(elem, "draw2D");
        window.draw3D = parseValue<bool>(elem, "draw3D");
        window.blitPreviousWindow = parseValue<bool>(elem, "blitPreviousWindow");
        window.monitor = parseValue<int>(elem, "monitor");

        if (const char* a = elem.Attribute("mpcdi"); a) {
            window.mpcdi = std::filesystem::absolute(a).string();
        }

        if (tinyxml2::XMLElement* e = elem.FirstChildElement("Stereo"); e) {
            window.stereo = getStereoType(e->Attribute("type"));
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("Pos"); e) {
            if (std::optional<sgct::ivec2> s = parseValueIVec2(*e); s) {
                window.pos = *s;
            }
            else {
                throw Err(6030, "Could not parse window position. Type error");
            }
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("Size"); e) {
            if (std::optional<sgct::ivec2> s = parseValueIVec2(*e); s) {
                window.size = *s;
            }
            else {
                throw Err(6031, "Could not parse window size. Type error");
            }
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("Res"); e) {
            if (std::optional<sgct::ivec2> s = parseValueIVec2(*e); s) {
                window.resolution = *s;
            }
            else {
                throw Err(6032, "Could not parse window resolution. Type error");
            }
        }

        tinyxml2::XMLElement* vp = elem.FirstChildElement("Viewport");
        while (vp) {
            window.viewports.push_back(parseViewport(*vp));
            vp = vp->NextSiblingElement("Viewport");
        }

        return window;
    }

    sgct::config::Node parseNode(tinyxml2::XMLElement& elem) {
        sgct::config::Node node;
        if (const char* a = elem.Attribute("address"); a) {
            node.address = a;
        }
        else {
            throw Err(6040, "Missing field address in node");
        }
        if (elem.Attribute("port")) {
            node.port = *parseValue<int>(elem, "port");
        }
        else {
            throw Err(6041, "Missing field port in node");
        }
        node.dataTransferPort = parseValue<int>(elem, "dataTransferPort");
        node.swapLock = parseValue<bool>(elem, "swapLock");

        tinyxml2::XMLElement* wnd = elem.FirstChildElement("Window");
        while (wnd) {
            sgct::config::Window window = parseWindow(*wnd);
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
            sgct::quat orientation = parseOrientationNode(*e);
            user.transformation = fromGLM<glm::mat4, sgct::mat4>(
                glm::mat4_cast(glm::make_quat(&orientation.x))
            );
        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("Matrix"); e) {
            user.transformation = parseValueMat4(*e);
            if (user.transformation) {
                if (std::optional<bool> t = parseValue<bool>(*e, "transpose"); t && *t) {
                    user.transformation = fromGLM<glm::mat4, sgct::mat4>(
                        glm::transpose(glm::make_mat4(user.transformation->values))
                    );
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

        settings.useDepthTexture = parseValue<bool>(elem, "DepthBufferTexture");
        settings.useNormalTexture = parseValue<bool>(elem, "NormalTexture");
        settings.usePositionTexture = parseValue<bool>(elem, "PositionTexture");

        std::optional<float> f = parseValue<float>(elem, "Precision");
        if (f && *f == 16.f) {
            settings.bufferFloatPrecision =
                sgct::config::Settings::BufferFloatPrecision::Float16Bit;
        }
        else if (f && *f == 32) {
            settings.bufferFloatPrecision =
                sgct::config::Settings::BufferFloatPrecision::Float32Bit;
        }
        else if (f) {
            throw Err(6050, fmt::format("Wrong buffer precision value {}", *f));
        }
        if (tinyxml2::XMLElement* e = elem.FirstChildElement("Display"); e) {
            sgct::config::Settings::Display display;
            display.swapInterval = parseValue<int>(*e, "swapInterval");
            display.refreshRate = parseValue<int>(*e, "refreshRate");
            settings.display = display;
        }

        return settings;
    }

    sgct::config::Capture parseCapture(tinyxml2::XMLElement& element) {
        sgct::config::Capture res;
        if (const char* a = element.Attribute("path"); a) {
            res.path = a;
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
                throw Err(6060, "Unknown capturing format");
            }(a);
        }
        std::optional<int> rangeBeg = parseValue<int>(element, "range-begin");
        std::optional<int> rangeEnd = parseValue<int>(element, "range-end");

        if (rangeBeg || rangeEnd) {
            res.range = sgct::config::Capture::ScreenShotRange();
        }

        if (rangeBeg) {
            res.range->first = *rangeBeg;
        }
        if (rangeEnd) {
            res.range->last = *rangeEnd;
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
            sgct::quat orientation = parseOrientationNode(*e);
            device.transformation = fromGLM<glm::mat4, sgct::mat4>(
                glm::mat4_cast(glm::make_quat(&orientation.x))
            );
        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("Matrix"); e) {
            device.transformation = parseValueMat4(*e);
            if (device.transformation) {
                if (std::optional<bool> t = parseValue<bool>(*e, "transpose"); t && *t) {
                    device.transformation = fromGLM<glm::mat4, sgct::mat4>(
                        glm::transpose(glm::make_mat4(device.transformation->values))
                    );
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
            throw Err(6070, "Tracker is missing 'name'");
        }

        if (tinyxml2::XMLElement* e = element.FirstChildElement("Device"); e) {
            sgct::config::Device device = parseDevice(*e);
            tracker.devices.push_back(device);
        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("Offset"); e) {
            tracker.offset = parseValueVec3(*e);
        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("Orientation"); e) {
            sgct::quat orientation = parseOrientationNode(*e);
            tracker.transformation = fromGLM<glm::mat4, sgct::mat4>(
                glm::mat4_cast(glm::make_quat(&orientation.x))
            );
        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("Scale"); e) {
            tracker.scale = parseValue<double>(*e, "value");
        }
        if (tinyxml2::XMLElement* e = element.FirstChildElement("Matrix"); e) {
            tracker.transformation = parseValueMat4(*e);
            if (tracker.transformation) {
                if (std::optional<bool> t = parseValue<bool>(*e, "transpose"); t && *t) {
                    tracker.transformation = fromGLM<glm::mat4, sgct::mat4>(
                        glm::transpose(glm::make_mat4(tracker.transformation->values))
                    );
                }
            }
        }

        return tracker;
    }

    sgct::config::Cluster readXMLFile(const std::string& filename) {
        if (filename.empty()) {
            throw Err(6080, "No XML file provided");
        }

        tinyxml2::XMLDocument xmlDoc;
        tinyxml2::XMLError err = xmlDoc.LoadFile(filename.c_str());
        const bool s = err == tinyxml2::XML_NO_ERROR;
        if (!s) {
            if (err == tinyxml2::XML_ERROR_FILE_NOT_FOUND) {
                throw Err(
                    6081,
                    fmt::format("Could not find configuration file: {}", filename)
                );
            }
            else {
                std::string s1 = xmlDoc.ErrorName() ? xmlDoc.ErrorName() : "";
                std::string s2 = xmlDoc.GetErrorStr1() ? xmlDoc.GetErrorStr1() : "";
                std::string s3 = xmlDoc.GetErrorStr2() ? xmlDoc.GetErrorStr2() : "";
                std::string s4 = s1 + ' ' + s2 + ' ' + s3;
                throw Err(
                    6082,
                    fmt::format(
                        "Error loading XML file '{}'. {} {} {}", filename, s1, s2, s3
                    )
                );
            }
        }

        sgct::config::Cluster cluster;
        tinyxml2::XMLElement* xmlRoot = xmlDoc.FirstChildElement("Cluster");
        if (xmlRoot == nullptr) {
            throw Err(6083, "Cannot find 'Cluster' node");
        }
        tinyxml2::XMLElement& root = *xmlRoot;

        if (const char* a = root.Attribute("masterAddress"); a) {
            cluster.masterAddress = a;
        }
        else {
            throw Err(6084, "Cannot find master address");
        }

        cluster.setThreadAffinity = parseValue<int>(root, "setThreadAffinity");
        cluster.debugLog = parseValue<bool>(root, "debugLog");
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
            sgct::config::Node node = parseNode(*nodeElem);
            cluster.nodes.push_back(node);
            nodeElem = nodeElem->NextSiblingElement("Node");
        }

        cluster.success = true;
        return cluster;
    }
} // namespace

namespace sgct {

config::Cluster readConfig(const std::string& filename) {
    Log::Debug(fmt::format("Parsing XML config '{}'", filename));

    // First save the old current working directory, set the new one
    std::filesystem::path oldPwd = std::filesystem::current_path();
    std::filesystem::path configFolder = std::filesystem::path(filename).parent_path();
    if (!configFolder.empty()) {
        std::filesystem::current_path(configFolder);
    }

    // Then load the cluster
    config::Cluster cluster = readXMLFile(filename);

    // and reset the current working directory to the old value
    std::filesystem::current_path(oldPwd);

    Log::Debug(fmt::format("Config file '{}' read successfully", filename));
    Log::Info(fmt::format("Number of nodes in cluster: {}", cluster.nodes.size()));

    for (size_t i = 0; i < cluster.nodes.size(); i++) {
        const config::Node& node = cluster.nodes[i];
        Log::Info(fmt::format(
            "\tNode ({}) address: {} [{}]", i, node.address, node.port
        ));
    }

    return cluster;
}

} // namespace sgct
