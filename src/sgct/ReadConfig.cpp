/*************************************************************************
 Copyright (c) 2012-2015 Miroslav Andel
 All rights reserved.
 
 For conditions of distribution and use, see copyright notice in sgct.h
 *************************************************************************/

#include <sgct/ReadConfig.h>

#include <sgct/ClusterManager.h>
#include <sgct/MessageHandler.h>
#include <sgct/SGCTMpcdi.h>
#include <sgct/SGCTSettings.h>
#include <sgct/SGCTTracker.h>
#include <sgct/SGCTTrackingDevice.h>
#include <sgct/SGCTUser.h>
#include <sgct/Viewport.h>
#include <algorithm>
#include <optional>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>

#ifndef SGCT_DONT_USE_EXTERNAL
#include <external/tinyxml2.h>
#else
#include <tinyxml2.h>
#endif

namespace {
    constexpr const char* DefaultSingleConfiguration = R"(
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

    sgct::SGCTWindow::StereoMode getStereoType(std::string type) {
        std::transform(type.begin(), type.end(), type.begin(), ::tolower);

        if (type == "none" || type == "no_stereo") {
            return sgct::SGCTWindow::StereoMode::NoStereo;
        }
        else if (type == "active" || type == "quadbuffer") {
            return sgct::SGCTWindow::StereoMode::Active;
        }
        else if (type == "checkerboard") {
            return sgct::SGCTWindow::StereoMode::Checkerboard;
        }
        else if (type == "checkerboard_inverted") {
            return sgct::SGCTWindow::StereoMode::CheckerboardInverted;
        }
        else if (type == "anaglyph_red_cyan") {
            return sgct::SGCTWindow::StereoMode::AnaglyphRedCyan;
        }
        else if (type == "anaglyph_amber_blue") {
            return sgct::SGCTWindow::StereoMode::AnaglyphAmberBlue;
        }
        else if (type == "anaglyph_wimmer") {
            return sgct::SGCTWindow::StereoMode::AnaglyphRedCyanWimmer;
        }
        else if (type == "vertical_interlaced") {
            return sgct::SGCTWindow::StereoMode::VerticalInterlaced;
        }
        else if (type == "vertical_interlaced_inverted") {
            return sgct::SGCTWindow::StereoMode::VerticalInterlacedInverted;
        }
        else if (type == "test" || type == "dummy") {
            return sgct::SGCTWindow::StereoMode::Dummy;
        }
        else if (type == "side_by_side") {
            return sgct::SGCTWindow::StereoMode::SideBySide;
        }
        else if (type == "side_by_side_inverted") {
            return sgct::SGCTWindow::StereoMode::SideBySideInverted;
        }
        else if (type == "top_bottom") {
            return sgct::SGCTWindow::StereoMode::TopBottom;
        }
        else if (type == "top_bottom_inverted") {
            return sgct::SGCTWindow::StereoMode::TopBottomInverted;
        }

        return sgct::SGCTWindow::StereoMode::NoStereo;
    }

    sgct::SGCTWindow::ColorBitDepth getBufferColorBitDepth(std::string type) {
        std::transform(type.begin(), type.end(), type.begin(), ::tolower);

        if (type == "8") {
            return sgct::SGCTWindow::ColorBitDepth::Depth8;
        }
        else if (type == "16") {
            return sgct::SGCTWindow::ColorBitDepth::Depth16;
        }

        else if (type == "16f") {
            return sgct::SGCTWindow::ColorBitDepth::Depth16Float;
        }
        else if (type == "32f") {
            return sgct::SGCTWindow::ColorBitDepth::Depth32Float;
        }

        else if (type == "16i") {
            return sgct::SGCTWindow::ColorBitDepth::Depth16Int;
        }
        else if (type == "32i") {
            return sgct::SGCTWindow::ColorBitDepth::Depth32Int;
        }

        else if (type == "16ui") {
            return sgct::SGCTWindow::ColorBitDepth::Depth16UInt;
        }
        else if (type == "32ui") {
            return sgct::SGCTWindow::ColorBitDepth::Depth32UInt;
        }

        return sgct::SGCTWindow::ColorBitDepth::Depth8;
    }

    std::optional<glm::ivec2> parseValueIVec2(const tinyxml2::XMLElement& e) {
        using namespace tinyxml2;

        glm::ivec2 value;
        XMLError xe = e.QueryIntAttribute("x", &value[0]);
        XMLError ye = e.QueryIntAttribute("y", &value[1]);
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
        using namespace tinyxml2;

        float tmpf[16];
        XMLError err[16] = {
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
        if (err[0] == XML_NO_ERROR && err[1] == XML_NO_ERROR &&
            err[2] == XML_NO_ERROR && err[3] == XML_NO_ERROR &&
            err[4] == XML_NO_ERROR && err[5] == XML_NO_ERROR &&
            err[6] == XML_NO_ERROR && err[7] == XML_NO_ERROR &&
            err[8] == XML_NO_ERROR && err[9] == XML_NO_ERROR &&
            err[10] == XML_NO_ERROR && err[11] == XML_NO_ERROR &&
            err[12] == XML_NO_ERROR && err[13] == XML_NO_ERROR &&
            err[14] == XML_NO_ERROR && err[15] == XML_NO_ERROR)
        {
            // glm & opengl uses column major order (normally row major
            // order is used in linear algebra)
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
        else {
            static_assert(false);
        }

        if (err == tinyxml2::XML_NO_ERROR) {
            return value;
        }
        else {
            return std::nullopt;
        }
    }


    void parseScene(tinyxml2::XMLElement* element) {
        using namespace sgct_core;
        using namespace tinyxml2;

        XMLElement* child = element->FirstChildElement();
        while (child) {
            std::string childVal = child->Value();

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
                        "ReadConfig: Failed to parse scene offset from XML!\n"
                    );
                }
            }

            if (childVal == "Orientation") {
                sgct_core::ClusterManager::instance()->setSceneRotation(
                    glm::mat4_cast(sgct_core::parseOrientationNode(child))
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
                        "ReadConfig: Failed to parse scene orientation from XML!\n"
                    );
                }
            }

            child = child->NextSiblingElement();
        }
    }

    void parseWindow(tinyxml2::XMLElement* element, std::string xmlFileName,
                     sgct_core::SGCTNode& node)
    {
        using namespace sgct_core;
        using namespace tinyxml2;

        sgct::SGCTWindow win(node.getNumberOfWindows());

        if (element->Attribute("name")) {
            win.setName(element->Attribute("name"));
        }

        if (element->Attribute("tags")) {
            std::string tags = element->Attribute("tags");

            std::vector<std::string> t;
            std::stringstream ss(std::move(tags));
            while (ss.good()) {
                std::string substr;
                getline(ss, substr, ',');
                t.push_back(substr);
            }

            win.setTags(std::move(t));
        }

        if (element->Attribute("bufferBitDepth")) {
            win.setColorBitDepth(
                getBufferColorBitDepth(element->Attribute("bufferBitDepth"))
            );
        }

        if (element->Attribute("preferBGR")) {
            win.setPreferBGR(
                strcmp(element->Attribute("preferBGR"), "true") == 0
            );
        }

        //compability with older versions
        if (element->Attribute("fullScreen")) {
            bool v = strcmp(element->Attribute("fullScreen"), "true") == 0;
            win.setWindowMode(v);
        }

        if (element->Attribute("fullscreen")) {
            bool v = strcmp(element->Attribute("fullscreen"), "true") == 0;
            win.setWindowMode(v);
        }

        if (element->Attribute("floating")) {
            bool v = strcmp(element->Attribute("floating"), "true") == 0;
            win.setFloating(v);
        }

        if (element->Attribute("alwaysRender")) {
            bool v = strcmp(element->Attribute("alwaysRender"), "true") == 0;
            win.setRenderWhileHidden(v);
        }

        if (element->Attribute("hidden")) {
            bool v = strcmp(element->Attribute("hidden"), "true") == 0;
            win.setVisibility(!v);
        }

        if (element->Attribute("dbuffered")) {
            bool v = strcmp(element->Attribute("dbuffered"), "true") == 0;
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

        int tmpSamples = 0;
        //compability with older versions

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
            bool v = strcmp(element->Attribute("alpha"), "true") == 0;
            win.setAlpha(v);
        }

        if (element->Attribute("fxaa")) {
            bool v = strcmp(element->Attribute("fxaa"), "true") == 0;
            win.setUseFXAA(v);
        }

        if (element->Attribute("FXAA")) {
            bool v = strcmp(element->Attribute("FXAA"), "true") == 0;
            win.setUseFXAA(v);
        }

        if (element->Attribute("decorated")) {
            bool v = strcmp(element->Attribute("decorated"), "true") == 0;
            win.setWindowDecoration(v);
        }

        if (element->Attribute("border")) {
            bool v = strcmp(element->Attribute("border"), "true") == 0;
            win.setWindowDecoration(v);
        }

        if (element->Attribute("draw2D")) {
            bool v = strcmp(element->Attribute("draw2D"), "true") == 0;
            win.setCallDraw2DFunction(v);
        }

        if (element->Attribute("draw3D")) {
            bool v = strcmp(element->Attribute("draw3D"), "true") == 0;
            win.setCallDraw3DFunction(v);
        }

        if (element->Attribute("copyPreviousWindowToCurrentWindow")) {
            bool v = strcmp(
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
            //replace all backslashes with slashes
            std::replace(path.begin(), path.end(), '\\', '/');
            bool parse = sgct_core::SGCTMpcdi().parseConfiguration(path, node, win);
        }

        XMLElement* child = element->FirstChildElement();
        while (child) {
            std::string childVal = child->Value();

            if (childVal == "Stereo") {
                sgct::SGCTWindow::StereoMode v = getStereoType(child->Attribute("type"));
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
                        "ReadConfig: Failed to parse window position from XML!\n"
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
                        "ReadConfig: Failed to parse window resolution from XML!\n"
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
                        "ReadConfig: Failed to parse frame buffer resolution from XML!\n"
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
        using namespace sgct_core;
        using namespace tinyxml2;

        SGCTNode node;

        if (element->Attribute("address")) {
            node.setAddress(element->Attribute("address"));
        }
        if (element->Attribute("name")) {
            node.setName(element->Attribute("name"));
        }
        if (element->Attribute("ip")) {
            //backward compability with older versions of SGCT config files
            node.setAddress(element->Attribute("ip"));
        }
        if (element->Attribute("port")) {
            node.setSyncPort(element->Attribute("port"));
        }
        if (element->Attribute("syncPort")) {
            node.setSyncPort(element->Attribute("syncPort"));
        }
        if (element->Attribute("dataTransferPort")) {
            node.setDataTransferPort(element->Attribute("dataTransferPort"));
        }
        if (element->Attribute("swapLock")) {
            bool useSwapLock = strcmp(element->Attribute("swapLock"), "true") == 0;
            node.setUseSwapGroups(useSwapLock);
        }

        XMLElement* child = element->FirstChildElement();
        while (child) {
            std::string childVal = child->Value();
            if (childVal == "Window") {
                parseWindow(child, xmlFileName, node);
            }
            child = child->NextSiblingElement();
        }
        ClusterManager::instance()->addNode(std::move(node));
    }

    void parseUser(tinyxml2::XMLElement* element) {
        using namespace sgct_core;
        using namespace tinyxml2;

        SGCTUser* usrPtr;
        if (element->Attribute("name")) {
            std::string name(element->Attribute("name"));
            std::unique_ptr<SGCTUser> usr = std::make_unique<SGCTUser>(name);
            usrPtr = usr.get();
            ClusterManager::instance()->addUser(std::move(usr));
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Info,
                "ReadConfig: Adding user '%s'!\n", name.c_str()
            );
        }
        else {
            usrPtr = ClusterManager::instance()->getDefaultUserPtr();
        }

        std::optional<float> eyeSep = parseValue<float>(*element, "eyeSeparation");
        if (eyeSep) {
            usrPtr->setEyeSeparation(*eyeSep);
        }

        XMLElement* child = element->FirstChildElement();
        while (child) {
            std::string childVal = child->Value();

            if (childVal == "Pos") {
                std::optional<glm::vec3> pos = parseValueVec3(*child);
                if (pos) {
                    usrPtr->setPos(std::move(*pos));
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse user position from XML!\n"
                    );
                }
            }
            else if (childVal == "Orientation") {
                usrPtr->setOrientation(parseOrientationNode(child));
            }
            else if (childVal == "Quaternion") {
                std::optional<glm::quat> quat = parseValueQuat(*child);
                if (quat) {
                    usrPtr->setOrientation(std::move(*quat));
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse device orientation in XML!\n"
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
                        "ReadConfig: Failed to parse user matrix in XML!\n"
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
                        "ReadConfig: Failed to parse user tracking data from XML!\n"
                    );
                }
            }
            child = child->NextSiblingElement();
        }
    }

    void parseCapture(tinyxml2::XMLElement* element) {
        if (element->Attribute("path")) {
            using CPI = sgct::SGCTSettings::CapturePath;

            const char* p = element->Attribute("path");
            sgct::SGCTSettings::instance()->setCapturePath(p, CPI::Mono);
            sgct::SGCTSettings::instance()->setCapturePath(p, CPI::LeftStereo);
            sgct::SGCTSettings::instance()->setCapturePath(p, CPI::RightStereo);
        }
        if (element->Attribute("monoPath") != nullptr) {
            using CPI = sgct::SGCTSettings::CapturePath;

            const char* p = element->Attribute("monoPath");
            sgct::SGCTSettings::instance()->setCapturePath(p, CPI::Mono);
        }
        if (element->Attribute("leftPath") != nullptr) {
            using CPI = sgct::SGCTSettings::CapturePath;

            const char* p = element->Attribute("leftPath");
            sgct::SGCTSettings::instance()->setCapturePath(p, CPI::LeftStereo);
        }
        if (element->Attribute("rightPath") != nullptr) {
            using CPI = sgct::SGCTSettings::CapturePath;

            const char* rPath = element->Attribute("rightPath");
            sgct::SGCTSettings::instance()->setCapturePath(rPath, CPI::RightStereo);
        }

        if (element->Attribute("format") != nullptr) {
            std::string format = element->Attribute("format");
            sgct::SGCTSettings::CaptureFormat f = [](const std::string& format) {
                if (format == "png" || format == "PNG") {
                    return sgct::SGCTSettings::CaptureFormat::PNG;
                }
                else if (format == "tga" || format == "TGA") {
                    return sgct::SGCTSettings::CaptureFormat::TGA;
                }
                else if (format == "jpg" || format == "JPG") {
                    return sgct::SGCTSettings::CaptureFormat::JPG;
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Warning,
                        "ReadConfig: Unkonwn capturing format. Using PNG\n"
                    );
                    return sgct::SGCTSettings::CaptureFormat::PNG;
                }
            } (format);
            sgct::SGCTSettings::instance()->setCaptureFormat(f);
        }
    }

    void parseDevice(tinyxml2::XMLElement* element) {
        using namespace sgct_core;
        using namespace tinyxml2;

        ClusterManager& cm = *ClusterManager::instance();
        cm.getTrackingManagerPtr().addDeviceToCurrentTracker(
            std::string(element->Attribute("name"))
        );

        XMLElement* child = element->FirstChildElement();

        while (child) {
            std::string childVal = child->Value();

            if (childVal == "Sensor") {
                std::optional<int> id = parseValue<int>(*child, "id");
                if (child->Attribute("vrpnAddress") && id) {
                    cm.getTrackingManagerPtr().addSensorToCurrentDevice(
                        child->Attribute("vrpnAddress"),
                        *id
                    );
                }
            }
            else if (childVal == "Buttons") {
                std::optional<int> count = parseValue<int>(*child, "count");
                if (child->Attribute("vrpnAddress") && count) {
                    cm.getTrackingManagerPtr().addButtonsToCurrentDevice(
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
                    cm.getTrackingManagerPtr().addAnalogsToCurrentDevice(
                        child->Attribute("vrpnAddress"),
                        *count
                    );
                }
            }
            else if (childVal == "Offset") {
                std::optional<glm::vec3> offset = parseValueVec3(*child);
                if (offset) {
                    sgct::SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                    sgct::SGCTTracker& tr = *m.getLastTrackerPtr();
                    sgct::SGCTTrackingDevice& device = *tr.getLastDevicePtr();
                    device.setOffset(std::move(*offset));
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse device offset in XML!\n"
                    );
                }
            }
            else if (childVal == "Orientation") {
                sgct::SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                sgct::SGCTTracker& tr = *m.getLastTrackerPtr();
                sgct::SGCTTrackingDevice& device = *tr.getLastDevicePtr();
                device.setOrientation(parseOrientationNode(child));
            }
            else if (childVal == "Quaternion") {
                std::optional<glm::quat> quat = parseValueQuat(*child);

                if (quat) {
                    sgct::SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                    sgct::SGCTTracker& tr = *m.getLastTrackerPtr();
                    sgct::SGCTTrackingDevice& device = *tr.getLastDevicePtr();
                    device.setOrientation(std::move(*quat));
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse device orientation in XML!\n"
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
                    sgct::SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                    sgct::SGCTTracker& tr = *m.getLastTrackerPtr();
                    sgct::SGCTTrackingDevice& device = *tr.getLastDevicePtr();

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
                        "ReadConfig: Failed to parse device matrix in XML!\n"
                    );
                }
            }
            child = child->NextSiblingElement();
        }
    }

    void parseTracker(tinyxml2::XMLElement* element) {
        using namespace sgct_core;
        using namespace tinyxml2;

        ClusterManager& cm = *ClusterManager::instance();
        cm.getTrackingManagerPtr().addTracker(std::string(element->Attribute("name")));

        XMLElement* child = element->FirstChildElement();
        while (child) {
            std::string childVal = child->Value();

            if (childVal == "Device" && child->Attribute("name")) {
                parseDevice(child);
            }
            else if (childVal == "Offset") {
                std::optional<glm::vec3> offset = parseValueVec3(*child);
                if (offset) {
                    sgct::SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                    sgct::SGCTTracker& tr = *m.getLastTrackerPtr();
                    sgct::SGCTTrackingDevice& device = *tr.getLastDevicePtr();
                    device.setOffset(std::move(*offset));
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse tracker offset in XML!\n"
                    );
                }
            }
            else if (childVal == "Orientation") {
                sgct::SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                sgct::SGCTTracker& tr = *m.getLastTrackerPtr();
                sgct::SGCTTrackingDevice& device = *tr.getLastDevicePtr();

                device.setOrientation(parseOrientationNode(child));
            }
            else if (childVal == "Quaternion") {
                std::optional<glm::quat> quat = parseValueQuat(*child);
                if (quat) {
                    sgct::SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                    sgct::SGCTTracker& tr = *m.getLastTrackerPtr();
                    sgct::SGCTTrackingDevice& device = *tr.getLastDevicePtr();
                    device.setOrientation(std::move(*quat));
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse tracker orientation quaternion!\n"
                    );
                }
            }
            else if (childVal == "Scale") {
                std::optional<double> value = parseValue<double>(*child, "value");
                if (value) {
                    sgct::SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                    sgct::SGCTTracker& tr = *m.getLastTrackerPtr();
                    tr.setScale(*value);
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse tracker scale in XML!\n"
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
                    sgct::SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                    sgct::SGCTTracker& tr = *m.getLastTrackerPtr();

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
                        "ReadConfig: Failed to parse tracker matrix in XML!\n"
                    );
                }
            }
            child = child->NextSiblingElement();
        }
    }

} // namespace

namespace sgct_core {

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

ReadConfig::ReadConfig(std::string filename) {
    mIsValid = false;
    
    if (filename.empty()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Warning,
            "ReadConfig: No file specified! Using default configuration...\n"
        );
        readAndParseXMLString();
        mIsValid = true;
    }
    else {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "ReadConfig: Parsing XML config '%s'...\n", filename.c_str()
        );
    
        if (!replaceEnvVars(filename)) {
            return;
        }
    
        if (!readAndParseXMLFile()) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "ReadConfig: Error occured while reading config file '%s'\nError: %s\n",
                xmlFileName.c_str(), mErrorMsg.c_str()
            );
            return;
        }
        mIsValid = true;
    
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "ReadConfig: Config file '%s' read successfully!\n", xmlFileName.c_str()
        );
    }
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Info,
        "ReadConfig: Number of nodes in cluster: %d\n",
        ClusterManager::instance()->getNumberOfNodes()
    );
    
    for (unsigned int i = 0; i < ClusterManager::instance()->getNumberOfNodes(); i++) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Info,
            "\tNode(%d) address: %s [%s]\n", i,
            ClusterManager::instance()->getNodePtr(i)->getAddress().c_str(),
            ClusterManager::instance()->getNodePtr(i)->getSyncPort().c_str()
        );
    }
}

bool ReadConfig::replaceEnvVars(const std::string& filename) {
    size_t foundPercentage = filename.find('%');
    if (foundPercentage != std::string::npos) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "ReadConfig: SGCT doesn't support the usage of '%%' characters in the path.\n"
        );
        return false;
    }
    
    std::vector<size_t> beginEnvVar;
    std::vector<size_t> endEnvVar;
    
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
            "ReadConfig: Error: Bad configuration path string!\n"
        );
        return false;
    }
    else {
        size_t appendPos = 0;
        for (unsigned int i = 0; i < beginEnvVar.size(); i++) {
            xmlFileName.append(filename.substr(appendPos, beginEnvVar[i] - appendPos));
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
                    "ReadConfig: Error: Cannot fetch environment variable '%s'.\n",
                    envVar.c_str()
                );
                return false;
            }
#else
            char* fetchedEnvVar = getenv(envVar.c_str());
            if (fetchedEnvVar == nullptr) {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
                    "ReadConfig: Error: Cannot fetch environment variable '%s'.\n",
                    envVar.c_str()
                );
                return false;
            }
#endif
            
            xmlFileName.append(fetchedEnvVar);
            appendPos = endEnvVar[i] + 1;
        }
        
        xmlFileName.append(filename.substr(appendPos));
        std::replace(xmlFileName.begin(), xmlFileName.end(), char(92), '/');
    }
    
    return true;
}

bool ReadConfig::readAndParseXMLFile() {
    if (xmlFileName.empty()) {
        mErrorMsg = "No XML file set!";
        return false;
    }
    
    tinyxml2::XMLDocument xmlDoc;
    bool loadSuccess = xmlDoc.LoadFile(xmlFileName.c_str()) == tinyxml2::XML_NO_ERROR;

    return readAndParseXML(xmlDoc, loadSuccess);
}

bool ReadConfig::readAndParseXMLString() {
    tinyxml2::XMLDocument xmlDoc;
    bool loadSuccess = xmlDoc.Parse(
        DefaultSingleConfiguration,
        strlen(DefaultSingleConfiguration)
    ) == tinyxml2::XML_NO_ERROR;
    
    return readAndParseXML(xmlDoc, loadSuccess);
}

bool ReadConfig::readAndParseXML(tinyxml2::XMLDocument& xmlDoc, bool loadSuccess) {
    using namespace tinyxml2;

    if (!loadSuccess) {
        mErrorMsg.clear();
        if (xmlDoc.GetErrorStr1() && xmlDoc.GetErrorStr2()) {
            mErrorMsg += "Parsing failed after: ";
            mErrorMsg += xmlDoc.GetErrorStr1() + ' ';
            mErrorMsg += xmlDoc.GetErrorStr2();
        }
        else if (xmlDoc.GetErrorStr1()) {
            mErrorMsg += "Parsing failed after: ";
            mErrorMsg += xmlDoc.GetErrorStr1();
        }
        else if (xmlDoc.GetErrorStr2()) {
            mErrorMsg += "Parsing failed after: ";
            mErrorMsg += xmlDoc.GetErrorStr2();
        }
        else {
            mErrorMsg = "File not found";
        }
        return false;
    }

    XMLElement* XMLroot = xmlDoc.FirstChildElement("Cluster");
    if (XMLroot == nullptr) {
        mErrorMsg = "Cannot find XML root!";
        return false;
    }
    
    const char* masterAddress = XMLroot->Attribute("masterAddress");
    if (!masterAddress) {
        mErrorMsg = "Cannot find master address or DNS name in XML!";
        return false;
    }
    ClusterManager::instance()->setMasterAddress(masterAddress);
    
    const char* debugMode = XMLroot->Attribute("debug");
    if (debugMode) {
        const bool m = strcmp(debugMode, "true") == 0;
        sgct::MessageHandler::instance()->setNotifyLevel(
             m ? sgct::MessageHandler::Level::Debug : sgct::MessageHandler::Level::Warning
        );
    }
    
    const char* externalControlPort = XMLroot->Attribute("externalControlPort");
    if (externalControlPort) {
        ClusterManager::instance()->setExternalControlPort(externalControlPort);
    }
    
    const char* firmSync = XMLroot->Attribute("firmSync");
    if (firmSync) {
        ClusterManager::instance()->setFirmFrameLockSyncStatus(
            strcmp(firmSync, "true") == 0
        );
    }
    
    XMLElement* element = XMLroot->FirstChildElement();
    while (element) {
        std::string val = element->Value();
        
        if (val == "Scene") {
            parseScene(element);
        }
        else if (val == "Node") {
            parseNode(element, xmlFileName);
        }
        else if (val == "User") {
            parseUser(element);
        }
        else if (val == "Settings") {
            sgct::SGCTSettings::instance()->configure(element);
        }
        else if (val == "Capture") {
            parseCapture(element);
        }
        else if (val == "Tracker" && element->Attribute("name")) {
            parseTracker(element);
        }
        element = element->NextSiblingElement();
    }

    return true;
}

bool ReadConfig::isValid() const {
    return mIsValid;
}

} // namespace sgct_config
