/*************************************************************************
 Copyright (c) 2012-2015 Miroslav Andel
 All rights reserved.
 
 For conditions of distribution and use, see copyright notice in sgct.h
 *************************************************************************/

#include <sgct/readconfig.h>

#include <sgct/clustermanager.h>
#include <sgct/messagehandler.h>
#include <sgct/mpcdi.h>
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
    constexpr const char* DefaultConfig = R"(
    <?xml version="1.0" ?>
      <Cluster masterAddress="localhost">
        <Node address="localhost" port="20401">
          <Window fullScreen="false">
            <Size x="1280" y="720" />
            <Viewport>
              <Pos x="0.0" y="0.0" />
              <Size x="1.0" y="1.0" />
              <Projectionplane>
                <Pos x="-1.778" y="-1.0" z="0.0" />
                <Pos x="-1.778" y=" 1.0" z="0.0" />
                <Pos x=" 1.778" y=" 1.0" z="0.0" />
              </Projectionplane>
            </Viewport>
          </Window>
        </Node>
        <User eyeSeparation="0.06">
        <Pos x="0.0" y="0.0" z="4.0" />
      </User>
    </Cluster>
    )";

    sgct::Window::StereoMode getStereoType(std::string type) {
        std::transform(
            type.begin(),
            type.end(),
            type.begin(),
            [](char c) { return static_cast<char>(::tolower(c)); }
        );

        if (type == "none" || type == "no_stereo") {
            return sgct::Window::StereoMode::NoStereo;
        }
        else if (type == "active" || type == "quadbuffer") {
            return sgct::Window::StereoMode::Active;
        }
        else if (type == "checkerboard") {
            return sgct::Window::StereoMode::Checkerboard;
        }
        else if (type == "checkerboard_inverted") {
            return sgct::Window::StereoMode::CheckerboardInverted;
        }
        else if (type == "anaglyph_red_cyan") {
            return sgct::Window::StereoMode::AnaglyphRedCyan;
        }
        else if (type == "anaglyph_amber_blue") {
            return sgct::Window::StereoMode::AnaglyphAmberBlue;
        }
        else if (type == "anaglyph_wimmer") {
            return sgct::Window::StereoMode::AnaglyphRedCyanWimmer;
        }
        else if (type == "vertical_interlaced") {
            return sgct::Window::StereoMode::VerticalInterlaced;
        }
        else if (type == "vertical_interlaced_inverted") {
            return sgct::Window::StereoMode::VerticalInterlacedInverted;
        }
        else if (type == "test" || type == "dummy") {
            return sgct::Window::StereoMode::Dummy;
        }
        else if (type == "side_by_side") {
            return sgct::Window::StereoMode::SideBySide;
        }
        else if (type == "side_by_side_inverted") {
            return sgct::Window::StereoMode::SideBySideInverted;
        }
        else if (type == "top_bottom") {
            return sgct::Window::StereoMode::TopBottom;
        }
        else if (type == "top_bottom_inverted") {
            return sgct::Window::StereoMode::TopBottomInverted;
        }

        return sgct::Window::StereoMode::NoStereo;
    }

    sgct::Window::ColorBitDepth getBufferColorBitDepth(std::string type) {
        std::transform(
            type.begin(),
            type.end(),
            type.begin(),
            [](char c) { return static_cast<char>(::tolower(c)); }
        );

        if (type == "8") {
            return sgct::Window::ColorBitDepth::Depth8;
        }
        else if (type == "16") {
            return sgct::Window::ColorBitDepth::Depth16;
        }

        else if (type == "16f") {
            return sgct::Window::ColorBitDepth::Depth16Float;
        }
        else if (type == "32f") {
            return sgct::Window::ColorBitDepth::Depth32Float;
        }

        else if (type == "16i") {
            return sgct::Window::ColorBitDepth::Depth16Int;
        }
        else if (type == "32i") {
            return sgct::Window::ColorBitDepth::Depth32Int;
        }

        else if (type == "16ui") {
            return sgct::Window::ColorBitDepth::Depth16UInt;
        }
        else if (type == "32ui") {
            return sgct::Window::ColorBitDepth::Depth32UInt;
        }

        return sgct::Window::ColorBitDepth::Depth8;
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


    void parseScene(tinyxml2::XMLElement* element) {
        using namespace sgct::core;
        using namespace tinyxml2;

        XMLElement* child = element->FirstChildElement();
        while (child) {
            std::string_view childVal = child->Value();

            if (childVal == "Offset") {
                std::optional<glm::vec3> offset = parseValueVec3(*child);
                if (offset) {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Debug,
                        "ReadConfig: Setting scene offset to (%f, %f, %f)\n",
                        offset->x, offset->y, offset->z
                    );

                    ClusterManager::instance()->setSceneOffset(*offset);
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse scene offset from XML\n"
                    );
                }
            }

            if (childVal == "Orientation") {
                sgct::core::ClusterManager::instance()->setSceneRotation(
                    glm::mat4_cast(sgct::core::readconfig::parseOrientationNode(child))
                );
            }

            if (childVal == "Scale") {
                std::optional<float> scale = parseValue<float>(*child, "value");
                if (scale) {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Debug,
                        "ReadConfig: Setting scene scale to %f\n", *scale
                    );

                    ClusterManager::instance()->setSceneScale(*scale);
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse scene orientation from XML\n"
                    );
                }
            }

            child = child->NextSiblingElement();
        }
    }

    void parseWindow(tinyxml2::XMLElement* element, std::string xmlFileName,
                     sgct::core::Node& node)
    {
        using namespace sgct::core;
        using namespace tinyxml2;

        sgct::Window win = sgct::Window(node.getNumberOfWindows());

        if (element->Attribute("name")) {
            win.setName(element->Attribute("name"));
        }

        if (element->Attribute("tags")) {
            std::string tags = element->Attribute("tags");
            std::vector<std::string> t = sgct::helpers::split(tags, ',');
            win.setTags(std::move(t));
        }

        if (element->Attribute("bufferBitDepth")) {
            win.setColorBitDepth(
                getBufferColorBitDepth(element->Attribute("bufferBitDepth"))
            );
        }

        if (element->Attribute("preferBGR")) {
            win.setPreferBGR(strcmp(element->Attribute("preferBGR"), "true") == 0);
        }

        //compability with older versions
        if (element->Attribute("fullScreen")) {
            const bool v = strcmp(element->Attribute("fullScreen"), "true") == 0;
            win.setWindowMode(v);
        }

        if (element->Attribute("fullscreen")) {
            const bool v = strcmp(element->Attribute("fullscreen"), "true") == 0;
            win.setWindowMode(v);
        }

        if (element->Attribute("floating")) {
            const bool v = strcmp(element->Attribute("floating"), "true") == 0;
            win.setFloating(v);
        }

        if (element->Attribute("alwaysRender")) {
            const bool v = strcmp(element->Attribute("alwaysRender"), "true") == 0;
            win.setRenderWhileHidden(v);
        }

        if (element->Attribute("hidden")) {
            const bool v = strcmp(element->Attribute("hidden"), "true") == 0;
            win.setVisibility(!v);
        }

        if (element->Attribute("dbuffered")) {
            const bool v = strcmp(element->Attribute("dbuffered"), "true") == 0;
            win.setDoubleBuffered(v);
        }

        std::optional<float> gamma = parseValue<float>(*element, "gamma");
        if (gamma && gamma > 0.1f) {
            win.setGamma(*gamma);
        }

        std::optional<float> contrast = parseValue<float>(*element, "contrast");
        if (contrast && contrast > 0.f) {
            win.setContrast(*contrast);
        }

        std::optional<float> brightness = parseValue<float>(*element, "brightness");
        if (brightness && brightness > 0.f) {
            win.setBrightness(*brightness);
        }

        std::optional<int> numberOfSamples = parseValue<int>(*element, "numberOfSamples");
        if (numberOfSamples && numberOfSamples <= 128) {
            win.setNumberOfAASamples(*numberOfSamples);
        }

        std::optional<int> msaa = parseValue<int>(*element, "msaa");
        if (msaa && msaa <= 128) {
            win.setNumberOfAASamples(*msaa);
        }

        std::optional<int> MSAA = parseValue<int>(*element, "MSAA");
        if (MSAA && MSAA <= 128) {
            win.setNumberOfAASamples(*MSAA);
        }

        if (element->Attribute("alpha")) {
            const bool v = strcmp(element->Attribute("alpha"), "true") == 0;
            win.setAlpha(v);
        }

        if (element->Attribute("fxaa")) {
            const bool v = strcmp(element->Attribute("fxaa"), "true") == 0;
            win.setUseFXAA(v);
        }

        if (element->Attribute("FXAA")) {
            const bool v = strcmp(element->Attribute("FXAA"), "true") == 0;
            win.setUseFXAA(v);
        }

        if (element->Attribute("decorated")) {
            const bool v = strcmp(element->Attribute("decorated"), "true") == 0;
            win.setWindowDecoration(v);
        }

        if (element->Attribute("border")) {
            const bool v = strcmp(element->Attribute("border"), "true") == 0;
            win.setWindowDecoration(v);
        }

        if (element->Attribute("draw2D")) {
            const bool v = strcmp(element->Attribute("draw2D"), "true") == 0;
            win.setCallDraw2DFunction(v);
        }

        if (element->Attribute("draw3D")) {
            const bool v = strcmp(element->Attribute("draw3D"), "true") == 0;
            win.setCallDraw3DFunction(v);
        }

        if (element->Attribute("copyPreviousWindowToCurrentWindow")) {
            const bool v = strcmp(
                element->Attribute("copyPreviousWindowToCurrentWindow"),
                "true"
            ) == 0;
            win.setCopyPreviousWindowToCurrentWindow(v);
        }

        std::optional<int> index = parseValue<int>(*element, "monitor");
        if (index) {
            win.setFullScreenMonitorIndex(*index);
        }

        if (element->Attribute("mpcdi")) {
            std::string path;
            size_t lastSlashPos = xmlFileName.find_last_of("/");
            if (lastSlashPos != std::string::npos) {
                path = xmlFileName.substr(0, lastSlashPos) + "/";
            }
            path += element->Attribute("mpcdi");
            std::replace(path.begin(), path.end(), '\\', '/');
            sgct::core::Mpcdi().parseConfiguration(path, node, win);
        }

        XMLElement* child = element->FirstChildElement();
        while (child) {
            std::string_view childVal = child->Value();

            if (childVal == "Stereo") {
                sgct::Window::StereoMode v = getStereoType(child->Attribute("type"));
                win.setStereoMode(v);
            }
            else if (childVal == "Pos") {
                std::optional<glm::ivec2> pos = parseValueIVec2(*child);
                if (pos) {
                    win.setWindowPosition(std::move(*pos));
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse window position from XML\n"
                    );
                }
            }
            else if (childVal == "Size") {
                std::optional<glm::ivec2> size = parseValueIVec2(*child);
                if (size) {
                    win.initWindowResolution(std::move(*size));
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse window resolution from XML\n"
                    );
                }
            }
            else if (childVal == "Res") {
                std::optional<glm::ivec2> res = parseValueIVec2(*child);
                if (res) {
                    win.setFramebufferResolution(std::move(*res));
                    win.setFixResolution(true);
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse frame buffer resolution from XML\n"
                    );
                }
            }
            else if (childVal == "Viewport") {
                std::unique_ptr<Viewport> vp = std::make_unique<Viewport>();
                vp->configure(child);
                win.addViewport(std::move(vp));
            }
            child = child->NextSiblingElement();
        }
        node.addWindow(std::move(win));
    }

    void parseNode(tinyxml2::XMLElement* element, std::string xmlFileName) {
        using namespace sgct::core;
        using namespace tinyxml2;

        std::unique_ptr<Node> node = std::make_unique<Node>();

        if (element->Attribute("address")) {
            node->setAddress(element->Attribute("address"));
        }
        if (element->Attribute("name")) {
            node->setName(element->Attribute("name"));
        }
        if (element->Attribute("ip")) {
            // backward compability with older versions of SGCT config files
            node->setAddress(element->Attribute("ip"));
        }
        if (element->Attribute("port")) {
            node->setSyncPort(element->Attribute("port"));
        }
        if (element->Attribute("syncPort")) {
            node->setSyncPort(element->Attribute("syncPort"));
        }
        if (element->Attribute("dataTransferPort")) {
            node->setDataTransferPort(element->Attribute("dataTransferPort"));
        }
        if (element->Attribute("swapLock")) {
            const bool useSwapLock = strcmp(element->Attribute("swapLock"), "true") == 0;
            node->setUseSwapGroups(useSwapLock);
        }

        XMLElement* child = element->FirstChildElement();
        while (child) {
            std::string_view childVal = child->Value();
            if (childVal == "Window") {
                parseWindow(child, xmlFileName, *node);
            }
            child = child->NextSiblingElement();
        }
        ClusterManager::instance()->addNode(std::move(node));
    }

    void parseUser(tinyxml2::XMLElement* element) {
        using namespace sgct::core;
        using namespace tinyxml2;

        User* usrPtr;
        if (element->Attribute("name")) {
            std::string name = element->Attribute("name");
            std::unique_ptr<User> usr = std::make_unique<User>(std::move(name));
            usrPtr = usr.get();
            ClusterManager::instance()->addUser(std::move(usr));
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Info,
                "ReadConfig: Adding user '%s'\n", name.c_str()
            );
        }
        else {
            usrPtr = &ClusterManager::instance()->getDefaultUser();
        }

        std::optional<float> eyeSep = parseValue<float>(*element, "eyeSeparation");
        if (eyeSep) {
            usrPtr->setEyeSeparation(*eyeSep);
        }

        XMLElement* child = element->FirstChildElement();
        while (child) {
            std::string_view childVal = child->Value();

            if (childVal == "Pos") {
                std::optional<glm::vec3> pos = parseValueVec3(*child);
                if (pos) {
                    usrPtr->setPos(std::move(*pos));
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse user position from XML\n"
                    );
                }
            }
            else if (childVal == "Orientation") {
                usrPtr->setOrientation(
                    sgct::core::readconfig::parseOrientationNode(child)
                );
            }
            else if (childVal == "Quaternion") {
                std::optional<glm::quat> quat = parseValueQuat(*child);
                if (quat) {
                    usrPtr->setOrientation(std::move(*quat));
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse device orientation in XML\n"
                    );
                }
            }
            else if (childVal == "Matrix") {
                bool transpose = true;
                if (child->Attribute("transpose")) {
                    transpose = strcmp(child->Attribute("transpose"), "true") == 0;
                }

                std::optional<glm::mat4> mat = parseValueMat4(*child);
                if (mat) {
                    if (transpose) {
                        usrPtr->setTransform(glm::transpose(*mat));
                    }
                    else {
                        usrPtr->setTransform(*mat);
                    }
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse user matrix in XML\n"
                    );
                }
            }
            else if (childVal == "Tracking") {
                const char* tracker = child->Attribute("tracker");
                const char* device = child->Attribute("device");

                if (tracker && device) {
                    usrPtr->setHeadTracker(tracker, device);
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse user tracking data from XML\n"
                    );
                }
            }
            child = child->NextSiblingElement();
        }
    }

    void parseCapture(tinyxml2::XMLElement* element) {
        if (element->Attribute("path")) {

            const char* p = element->Attribute("path");
            using sgct::Settings;
            Settings::instance()->setCapturePath(p, Settings::CapturePath::Mono);
            Settings::instance()->setCapturePath(p, Settings::CapturePath::LeftStereo);
            Settings::instance()->setCapturePath(p, Settings::CapturePath::RightStereo);
        }
        if (element->Attribute("monoPath")) {
            const char* p = element->Attribute("monoPath");
            using sgct::Settings;
            Settings::instance()->setCapturePath(p, Settings::CapturePath::Mono);
        }
        if (element->Attribute("leftPath")) {
            const char* p = element->Attribute("leftPath");
            using sgct::Settings;
            Settings::instance()->setCapturePath(p, Settings::CapturePath::LeftStereo);
        }
        if (element->Attribute("rightPath")) {
            const char* p = element->Attribute("rightPath");
            using sgct::Settings;
            Settings::instance()->setCapturePath(p, Settings::CapturePath::RightStereo);
        }

        if (element->Attribute("format")) {
            std::string_view format = element->Attribute("format");
            sgct::Settings::CaptureFormat f = [](std::string_view format) {
                if (format == "png" || format == "PNG") {
                    return sgct::Settings::CaptureFormat::PNG;
                }
                else if (format == "tga" || format == "TGA") {
                    return sgct::Settings::CaptureFormat::TGA;
                }
                else if (format == "jpg" || format == "JPG") {
                    return sgct::Settings::CaptureFormat::JPG;
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Warning,
                        "ReadConfig: Unknown capturing format. Using PNG\n"
                    );
                    return sgct::Settings::CaptureFormat::PNG;
                }
            } (format);
            sgct::Settings::instance()->setCaptureFormat(f);
        }
    }

    void parseDevice(tinyxml2::XMLElement* element) {
        using namespace sgct::core;
        using namespace tinyxml2;

        ClusterManager& cm = *ClusterManager::instance();
        cm.getTrackingManager().addDeviceToCurrentTracker(element->Attribute("name"));

        XMLElement* child = element->FirstChildElement();

        while (child) {
            std::string_view childVal = child->Value();

            if (childVal == "Sensor") {
                std::optional<int> id = parseValue<int>(*child, "id");
                if (child->Attribute("vrpnAddress") && id) {
                    cm.getTrackingManager().addSensorToCurrentDevice(
                        child->Attribute("vrpnAddress"),
                        *id
                    );
                }
            }
            else if (childVal == "Buttons") {
                std::optional<int> count = parseValue<int>(*child, "count");
                if (child->Attribute("vrpnAddress") && count) {
                    cm.getTrackingManager().addButtonsToCurrentDevice(
                        child->Attribute("vrpnAddress"),
                        *count
                    );
                }

            }
            else if (childVal == "Axes") {
                std::optional<unsigned int> count = parseValue<unsigned int>(
                    *child,
                    "count"
                );
                if (child->Attribute("vrpnAddress") && count) {
                    cm.getTrackingManager().addAnalogsToCurrentDevice(
                        child->Attribute("vrpnAddress"),
                        *count
                    );
                }
            }
            else if (childVal == "Offset") {
                std::optional<glm::vec3> offset = parseValueVec3(*child);
                if (offset) {
                    sgct::TrackingManager& m = cm.getTrackingManager();
                    sgct::Tracker& tr = *m.getLastTracker();
                    sgct::TrackingDevice& device = *tr.getLastDevice();
                    device.setOffset(std::move(*offset));
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse device offset in XML\n"
                    );
                }
            }
            else if (childVal == "Orientation") {
                sgct::TrackingManager& m = cm.getTrackingManager();
                sgct::Tracker& tr = *m.getLastTracker();
                sgct::TrackingDevice& device = *tr.getLastDevice();
                device.setOrientation(sgct::core::readconfig::parseOrientationNode(child));
            }
            else if (childVal == "Quaternion") {
                std::optional<glm::quat> quat = parseValueQuat(*child);

                if (quat) {
                    sgct::TrackingManager& m = cm.getTrackingManager();
                    sgct::Tracker& tr = *m.getLastTracker();
                    sgct::TrackingDevice& device = *tr.getLastDevice();
                    device.setOrientation(std::move(*quat));
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse device orientation in XML\n"
                    );
                }
            }
            else if (childVal == "Matrix") {
                bool transpose = true;
                if (child->Attribute("transpose")) {
                    transpose = strcmp(child->Attribute("transpose"), "true") == 0;
                }

                std::optional<glm::mat4> mat = parseValueMat4(*child);
                if (mat) {
                    sgct::TrackingManager& m = cm.getTrackingManager();
                    sgct::Tracker& tr = *m.getLastTracker();
                    sgct::TrackingDevice& device = *tr.getLastDevice();

                    if (transpose) {
                        device.setTransform(glm::transpose(*mat));
                    }
                    else {
                        device.setTransform(*mat);
                    }
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse device matrix in XML\n"
                    );
                }
            }
            child = child->NextSiblingElement();
        }
    }

    void parseTracker(tinyxml2::XMLElement* element) {
        using namespace sgct::core;
        using namespace tinyxml2;

        ClusterManager& cm = *ClusterManager::instance();
        cm.getTrackingManager().addTracker(std::string(element->Attribute("name")));

        XMLElement* child = element->FirstChildElement();
        while (child) {
            std::string_view childVal = child->Value();

            if (childVal == "Device" && child->Attribute("name")) {
                parseDevice(child);
            }
            else if (childVal == "Offset") {
                std::optional<glm::vec3> offset = parseValueVec3(*child);
                if (offset) {
                    sgct::TrackingManager& m = cm.getTrackingManager();
                    sgct::Tracker& tr = *m.getLastTracker();
                    sgct::TrackingDevice& device = *tr.getLastDevice();
                    device.setOffset(std::move(*offset));
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse tracker offset in XML\n"
                    );
                }
            }
            else if (childVal == "Orientation") {
                sgct::TrackingManager& m = cm.getTrackingManager();
                sgct::Tracker& tr = *m.getLastTracker();
                sgct::TrackingDevice& device = *tr.getLastDevice();
                device.setOrientation(sgct::core::readconfig::parseOrientationNode(child));
            }
            else if (childVal == "Quaternion") {
                std::optional<glm::quat> quat = parseValueQuat(*child);
                if (quat) {
                    sgct::TrackingManager& m = cm.getTrackingManager();
                    sgct::Tracker& tr = *m.getLastTracker();
                    sgct::TrackingDevice& device = *tr.getLastDevice();
                    device.setOrientation(std::move(*quat));
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse tracker orientation quaternion\n"
                    );
                }
            }
            else if (childVal == "Scale") {
                std::optional<double> value = parseValue<double>(*child, "value");
                if (value) {
                    sgct::TrackingManager& m = cm.getTrackingManager();
                    sgct::Tracker& tr = *m.getLastTracker();
                    tr.setScale(*value);
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse tracker scale in XML\n"
                    );
                }
            }
            else if (childVal == "Matrix") {
                bool transpose = true;
                if (child->Attribute("transpose")) {
                    transpose = strcmp(child->Attribute("transpose"), "true") == 0;
                }

                std::optional<glm::mat4> mat = parseValueMat4(*child);
                if (mat) {
                    sgct::TrackingManager& m = cm.getTrackingManager();
                    sgct::Tracker& tr = *m.getLastTracker();

                    if (transpose) {
                        tr.setTransform(glm::transpose(std::move(*mat)));
                    }
                    else {
                        tr.setTransform(std::move(*mat));
                    }
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse tracker matrix in XML\n"
                    );
                }
            }
            child = child->NextSiblingElement();
        }
    }

    void readAndParseXML(tinyxml2::XMLDocument& xmlDoc, const std::string& filename) {
        using namespace tinyxml2;

        XMLElement* XMLroot = xmlDoc.FirstChildElement("Cluster");
        if (XMLroot == nullptr) {
            throw std::runtime_error("Cannot find XML root");
        }

        const char* masterAddress = XMLroot->Attribute("masterAddress");
        if (!masterAddress) {
            throw std::runtime_error("Cannot find master address or DNS name in XML");
        }
        sgct::core::ClusterManager::instance()->setMasterAddress(masterAddress);

        const char* debugMode = XMLroot->Attribute("debug");
        if (debugMode) {
            using namespace sgct;
            const bool m = strcmp(debugMode, "true") == 0;
            MessageHandler::instance()->setNotifyLevel(
                m ? MessageHandler::Level::Debug : MessageHandler::Level::Warning
            );
        }

        const char* port = XMLroot->Attribute("externalControlPort");
        if (port) {
            sgct::core::ClusterManager::instance()->setExternalControlPort(port);
        }

        const char* firmSync = XMLroot->Attribute("firmSync");
        if (firmSync) {
            sgct::core::ClusterManager::instance()->setFirmFrameLockSyncStatus(
                strcmp(firmSync, "true") == 0
            );
        }

        XMLElement* element = XMLroot->FirstChildElement();
        while (element) {
            std::string_view val = element->Value();

            if (val == "Scene") {
                parseScene(element);
            }
            else if (val == "Node") {
                parseNode(element, filename);
            }
            else if (val == "User") {
                parseUser(element);
            }
            else if (val == "Settings") {
                sgct::Settings::instance()->configure(element);
            }
            else if (val == "Capture") {
                parseCapture(element);
            }
            else if (val == "Tracker" && element->Attribute("name")) {
                parseTracker(element);
            }
            element = element->NextSiblingElement();
        }
    }

    void readAndParseXMLFile(const std::string& filename) {
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
        readAndParseXML(xmlDoc, filename);
    }

    void readAndParseXMLString() {
        using namespace tinyxml2;
        tinyxml2::XMLDocument xmlDoc;
        XMLError err = xmlDoc.Parse(DefaultConfig, strlen(DefaultConfig));
        assert(err == tinyxml2::XML_NO_ERROR);
        readAndParseXML(xmlDoc, "");
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

void readConfig(const std::string& filename) {
    std::string f = filename;
    if (f.empty()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "ReadConfig: No file specified! Using default configuration...\n"
        );
        readAndParseXMLString();
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "ReadConfig: Parsing XML config '%s'...\n", f.c_str()
        );

        f = replaceEnvVars(f);
        if (f.empty()) {
            throw std::runtime_error("Could not resolve file path");
        }

        readAndParseXMLFile(f);

        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "ReadConfig: Config file '%s' read successfully\n", f.c_str()
        );
    }
    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "ReadConfig: Number of nodes in cluster: %d\n",
        ClusterManager::instance()->getNumberOfNodes()
    );

    for (unsigned int i = 0; i < ClusterManager::instance()->getNumberOfNodes(); i++) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "\tNode(%d) address: %s [%s]\n", i,
            ClusterManager::instance()->getNode(i)->getAddress().c_str(),
            ClusterManager::instance()->getNode(i)->getSyncPort().c_str()
        );
    }
}

} // namespace sgct_config::readconfig
