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
            <Size x="640" y="480" />
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
        <User eyeSeparation="0.06\>
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


    void parseScene(tinyxml2::XMLElement* element) {
        using namespace sgct_core;
        using namespace tinyxml2;

        XMLElement* child = element->FirstChildElement();
        while (child) {
            const char* childVal = child->Value();

            if (strcmp("Offset", childVal) == 0) {
                glm::vec3 sceneOffset = glm::vec3(0.f);
                XMLError xValue = child->QueryFloatAttribute("x", &sceneOffset[0]);
                XMLError yValue = child->QueryFloatAttribute("y", &sceneOffset[1]);
                XMLError zValue = child->QueryFloatAttribute("z", &sceneOffset[2]);
                if (xValue == XML_NO_ERROR &&
                    yValue == XML_NO_ERROR &&
                    zValue == XML_NO_ERROR)
                {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Debug,
                        "ReadConfig: Setting scene offset to (%f, %f, %f)\n",
                        sceneOffset.x, sceneOffset.y, sceneOffset.z
                    );

                    sgct_core::ClusterManager::instance()->setSceneOffset(sceneOffset);
                }
                else {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "ReadConfig: Failed to parse scene offset from XML!\n"
                    );
                }
            }
            else if (strcmp("Orientation", childVal) == 0) {
                sgct_core::ClusterManager::instance()->setSceneRotation(
                    glm::mat4_cast(sgct_core::ReadConfig::parseOrientationNode(child))
                );
            }
            else if (strcmp("Scale", childVal) == 0) {
                float scale = 1.f;
                XMLError value = child->QueryFloatAttribute("value", &scale);
                if (value == tinyxml2::XML_NO_ERROR) {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Debug,
                        "ReadConfig: Setting scene scale to %f\n", scale
                    );

                    sgct_core::ClusterManager::instance()->setSceneScale(scale);
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

    void parseNode(tinyxml2::XMLElement* element) {
        using namespace sgct_core;

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
            const char* childVal = child->Value();
            if (strcmp("Window", childVal) == 0) {
                sgct::SGCTWindow win(static_cast<int>(node.getNumberOfWindows()));

                if (child->Attribute("name")) {
                    win.setName(child->Attribute("name"));
                }

                if (child->Attribute("tags")) {
                    std::string tags = child->Attribute("tags");

                    std::vector<std::string> t;
                    std::stringstream ss(std::move(tags));
                    while (ss.good()) {
                        std::string substr;
                        getline(ss, substr, ',');
                        t.push_back(substr);
                    }

                    win.setTags(std::move(t));
                }

                if (child->Attribute("bufferBitDepth")) {
                    win.setColorBitDepth(
                        getBufferColorBitDepth(child->Attribute("bufferBitDepth"))
                    );
                }

                if (child->Attribute("preferBGR")) {
                    win.setPreferBGR(
                        strcmp(child->Attribute("preferBGR"), "true") == 0
                    );
                }

                //compability with older versions
                if (child->Attribute("fullScreen")) {
                    bool v = strcmp(child->Attribute("fullScreen"), "true") == 0;
                    win.setWindowMode(v);
                }

                if (child->Attribute("fullscreen")) {
                    bool v = strcmp(child->Attribute("fullscreen"), "true") == 0;
                    win.setWindowMode(v);
                }

                if (child->Attribute("floating")) {
                    bool v = strcmp(child->Attribute("floating"), "true") == 0;
                    win.setFloating(v);
                }

                if (child->Attribute("alwaysRender")) {
                    bool v = strcmp(child->Attribute("alwaysRender"), "true") == 0;
                    win.setRenderWhileHidden(v);
                }

                if (child->Attribute("hidden")) {
                    bool v = strcmp(child->Attribute("hidden"), "true") == 0;
                    win.setVisibility(!v);
                }

                if (child->Attribute("dbuffered")) {
                    bool v = strcmp(child->Attribute("dbuffered"), "true") == 0;
                    win.setDoubleBuffered(v);
                }

                float gamma = 0.f;
                XMLError gammaErr = child->QueryFloatAttribute("gamma", &gamma);
                if (gammaErr == XML_NO_ERROR && gamma > 0.1f) {
                    win.setGamma(gamma);
                }

                float contrast = -1.f;
                XMLError contrastErr = child->QueryFloatAttribute(
                    "contrast",
                    &contrast
                );
                if (contrastErr == XML_NO_ERROR && contrast > 0.f) {
                    win.setContrast(contrast);
                }

                float brightness = -1.f;
                XMLError brightnessErr = child->QueryFloatAttribute(
                    "brightness",
                    &brightness
                );
                if (brightnessErr == XML_NO_ERROR && brightness > 0.f) {
                    win.setBrightness(brightness);
                }

                int tmpSamples = 0;
                //compability with older versions

                XMLError sampleErr = child->QueryIntAttribute(
                    "numberOfSamples",
                    &tmpSamples
                );
                XMLError msaaErr = child->QueryIntAttribute("msaa", &tmpSamples);
                XMLError msErr = child->QueryIntAttribute("MSAA", &tmpSamples);

                const bool hasSample = sampleErr == XML_NO_ERROR ||
                    msaaErr == XML_NO_ERROR ||
                    msErr == XML_NO_ERROR;

                if (hasSample && tmpSamples <= 128) {
                    win.setNumberOfAASamples(tmpSamples);
                }

                if (child->Attribute("alpha")) {
                    bool v = strcmp(child->Attribute("alpha"), "true") == 0;
                    win.setAlpha(v);
                }

                if (child->Attribute("fxaa")) {
                    bool v = strcmp(child->Attribute("fxaa"), "true") == 0;
                    win.setUseFXAA(v);
                }

                if (child->Attribute("FXAA")) {
                    bool v = strcmp(child->Attribute("FXAA"), "true") == 0;
                    win.setUseFXAA(v);
                }

                if (child->Attribute("decorated")) {
                    bool v = strcmp(child->Attribute("decorated"), "true") == 0;
                    win.setWindowDecoration(v);
                }

                if (child->Attribute("border")) {
                    bool v = strcmp(child->Attribute("border"), "true") == 0;
                    win.setWindowDecoration(v);
                }

                if (child->Attribute("draw2D")) {
                    bool v = strcmp(child->Attribute("draw2D"), "true") == 0;
                    win.setCallDraw2DFunction(v);
                }

                if (child->Attribute("draw3D")) {
                    bool v = strcmp(child->Attribute("draw3D"), "true") == 0;
                    win.setCallDraw3DFunction(v);
                }

                if (child->Attribute("copyPreviousWindowToCurrentWindow")) {
                    bool v = strcmp(
                        child->Attribute("copyPreviousWindowToCurrentWindow"),
                        "true"
                    ) == 0;
                    win.setCopyPreviousWindowToCurrentWindow(v);
                }

                int index = 0;
                XMLError monitorErr = child->QueryIntAttribute("monitor", &index);
                if (monitorErr == tinyxml2::XML_NO_ERROR) {
                    win.setFullScreenMonitorIndex(index);
                }

                if (child->Attribute("mpcdi")) {
                    std::string path;
                    size_t lastSlashPos = xmlFileName.find_last_of("/");
                    if (lastSlashPos != std::string::npos) {
                        path = xmlFileName.substr(0, lastSlashPos) + "/";
                    }
                    path += child->Attribute("mpcdi");
                    //replace all backslashes with slashes
                    std::replace(path.begin(), path.end(), '\\', '/');
                    bool parse = sgct_core::SGCTMpcdi().parseConfiguration(
                        path,
                        node,
                        win
                    );
                    if (!parse) {
                        return false;
                    }
                }

                XMLElement* grandChild = child->FirstChildElement();
                while (grandChild) {
                    const char* grandChildVal = grandChild->Value();

                    if (strcmp("Stereo", grandChildVal) == 0) {
                        sgct::SGCTWindow::StereoMode v = getStereoType(
                            grandChild->Attribute("type")
                        );
                        win.setStereoMode(v);
                    }
                    else if (strcmp("Pos", grandChildVal) == 0) {
                        glm::ivec2 pos;
                        XMLError xErr = grandChild->QueryIntAttribute("x", &pos[0]);
                        XMLError yErr = grandChild->QueryIntAttribute("y", &pos[1]);
                        if (xErr == XML_NO_ERROR && yErr == XML_NO_ERROR) {
                            win.setWindowPosition(std::move(pos));
                        }
                        else {
                            sgct::MessageHandler::instance()->print(
                                sgct::MessageHandler::Level::Error,
                                "ReadConfig: Failed to parse window position from XML!\n"
                            );
                        }
                    }
                    else if (strcmp("Size", grandChildVal) == 0) {
                        glm::ivec2 size;
                        XMLError xErr = grandChild->QueryIntAttribute("x", &size[0]);
                        XMLError yErr = grandChild->QueryIntAttribute("y", &size[1]);
                        if (xErr == XML_NO_ERROR && yErr == XML_NO_ERROR) {
                            win.initWindowResolution(size);
                        }
                        else {
                            sgct::MessageHandler::instance()->print(
                                sgct::MessageHandler::Level::Error,
                                "ReadConfig: Failed to parse window resolution from XML!\n"
                            );
                        }
                    }
                    else if (strcmp("Res", grandChildVal) == 0) {
                        glm::ivec2 res;
                        XMLError xErr = grandChild->QueryIntAttribute("x", &res[0]);
                        XMLError yErr = grandChild->QueryIntAttribute("y", &res[1]);
                        if (xErr == XML_NO_ERROR && yErr == XML_NO_ERROR) {
                            win.setFramebufferResolution(std::move(res));
                            win.setFixResolution(true);
                        }
                        else {
                            sgct::MessageHandler::instance()->print(
                                sgct::MessageHandler::Level::Error,
                                "ReadConfig: Failed to parse frame buffer resolution from XML!\n"
                            );
                        }
                    }
                    else if (strcmp("Viewport", grandChildVal) == 0) {
                        std::unique_ptr<Viewport> vp = std::make_unique<Viewport>();
                        vp->configure(grandChild);
                        win.addViewport(std::move(vp));
                    }
                    grandChild = grandChild->NextSiblingElement();
                }
                node.addWindow(std::move(win));
            }
            child = child->NextSiblingElement();
        }
        ClusterManager::instance()->addNode(std::move(node));
    }

} // namespace

namespace sgct_core {

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
    if (xmlDoc.LoadFile(xmlFileName.c_str()) != tinyxml2::XML_NO_ERROR) {
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

    return readAndParseXML(xmlDoc);
}

bool ReadConfig::readAndParseXMLString() {
    tinyxml2::XMLDocument xmlDoc;
    bool loadSuccess = xmlDoc.Parse(
        DefaultSingleConfiguration,
        strlen(DefaultSingleConfiguration)
    ) == tinyxml2::XML_NO_ERROR;
    
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

    return readAndParseXML(xmlDoc);
}

bool ReadConfig::readAndParseXML(tinyxml2::XMLDocument& xmlDoc) {
    using namespace tinyxml2;

    XMLElement* XMLroot = xmlDoc.FirstChildElement("Cluster");
    if (XMLroot == nullptr) {
        mErrorMsg = "Cannot find XML root!";
        return false;
    }
    
    const char* masterAddress = XMLroot->Attribute("masterAddress");
    if (masterAddress) {
        ClusterManager::instance()->setMasterAddress(masterAddress);
    }
    else {
        mErrorMsg = "Cannot find master address or DNS name in XML!";
        return false;
    }
    
    const char* debugMode = XMLroot->Attribute("debug");
    if (debugMode) {
        const bool m = strcmp(debugMode, "true") == 0;
        sgct::MessageHandler::instance()->setNotifyLevel(
             m ? sgct::MessageHandler::Level::Debug :sgct::MessageHandler::Level::Warning
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
        const char* val = element->Value();
        
        if (strcmp("Scene", val) == 0) {
            parseScene(element);
        }
        else if (strcmp("Node", val) == 0) {
            parseNode(element);
        }
        else if (strcmp("User", val) == 0) {
            SGCTUser* usrPtr;
            if (element->Attribute("name") != nullptr) {
                std::string name(element->Attribute("name"));
                ClusterManager::instance()->addUser(std::make_unique<SGCTUser>(name));
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Info,
                    "ReadConfig: Adding user '%s'!\n", name.c_str()
                );
            }
            else {
                usrPtr = ClusterManager::instance()->getDefaultUserPtr();
            }

            float eyeSep;
            XMLError eyeSepErr = element->QueryFloatAttribute("eyeSeparation", &eyeSep);
            if (eyeSepErr == XML_NO_ERROR) {
                usrPtr->setEyeSeparation(eyeSep);
            }
            
            XMLElement* child = element->FirstChildElement();
            while (child) {
                const char* childVal = child->Value();
                
                if (strcmp("Pos", childVal) == 0) {
                    glm::vec3 pos;
                    XMLError xe = child->QueryFloatAttribute("x", &pos[0]);
                    XMLError ye = child->QueryFloatAttribute("y", &pos[1]);
                    XMLError ze = child->QueryFloatAttribute("z", &pos[2]);
                    if (xe == XML_NO_ERROR && ye == XML_NO_ERROR && ze == XML_NO_ERROR) {
                        usrPtr->setPos(std::move(pos));
                    }
                    else {
                        sgct::MessageHandler::instance()->print(
                            sgct::MessageHandler::Level::Error,
                            "ReadConfig: Failed to parse user position from XML!\n"
                        );
                    }
                }
                else if (strcmp("Orientation", childVal) == 0) {
                    usrPtr->setOrientation(parseOrientationNode(child));
                }
                else if (strcmp("Quaternion", childVal) == 0) {
                    glm::quat q;
                    
                    // Yes, this order is correct;  the constructor takes w,x,y,z but the
                    // layout in memory is x,y,z,w
                    XMLError we = child->QueryFloatAttribute("w", &q[3]);
                    XMLError xe = child->QueryFloatAttribute("x", &q[0]);
                    XMLError ye = child->QueryFloatAttribute("y", &q[1]);
                    XMLError ze = child->QueryFloatAttribute("z", &q[2]);
                    if (we == tinyxml2::XML_NO_ERROR && xe == tinyxml2::XML_NO_ERROR &&
                        ye == tinyxml2::XML_NO_ERROR && ze == tinyxml2::XML_NO_ERROR)
                    {
                        usrPtr->setOrientation(q);
                    }
                    else {
                        sgct::MessageHandler::instance()->print(
                            sgct::MessageHandler::Level::Error,
                            "ReadConfig: Failed to parse device orientation in XML!\n"
                        );
                    }
                }
                else if (strcmp("Matrix", childVal) == 0) {
                    bool transpose = true;
                    if (child->Attribute("transpose")) {
                        transpose = strcmp(child->Attribute("transpose"), "true") == 0;
                    }

                    float tmpf[16];
                    XMLError err[16] = {
                        child->QueryFloatAttribute("x0", &tmpf[ 0]),
                        child->QueryFloatAttribute("y0", &tmpf[ 1]),
                        child->QueryFloatAttribute("z0", &tmpf[ 2]),
                        child->QueryFloatAttribute("w0", &tmpf[ 3]),
                        child->QueryFloatAttribute("x1", &tmpf[ 4]),
                        child->QueryFloatAttribute("y1", &tmpf[ 5]),
                        child->QueryFloatAttribute("z1", &tmpf[ 6]),
                        child->QueryFloatAttribute("w1", &tmpf[ 7]),
                        child->QueryFloatAttribute("x2", &tmpf[ 8]),
                        child->QueryFloatAttribute("y2", &tmpf[ 9]),
                        child->QueryFloatAttribute("z2", &tmpf[10]),
                        child->QueryFloatAttribute("w2", &tmpf[11]),
                        child->QueryFloatAttribute("x3", &tmpf[12]),
                        child->QueryFloatAttribute("y3", &tmpf[13]),
                        child->QueryFloatAttribute("z3", &tmpf[14]),
                        child->QueryFloatAttribute("w3", &tmpf[15])
                    };
                    if (err[ 0] == XML_NO_ERROR && err[ 1] == XML_NO_ERROR &&
                        err[ 2] == XML_NO_ERROR && err[ 3] == XML_NO_ERROR &&
                        err[ 4] == XML_NO_ERROR && err[ 5] == XML_NO_ERROR &&
                        err[ 6] == XML_NO_ERROR && err[ 7] == XML_NO_ERROR &&
                        err[ 8] == XML_NO_ERROR && err[ 9] == XML_NO_ERROR &&
                        err[10] == XML_NO_ERROR && err[11] == XML_NO_ERROR &&
                        err[12] == XML_NO_ERROR && err[13] == XML_NO_ERROR &&
                        err[14] == XML_NO_ERROR && err[15] == XML_NO_ERROR)
                    {
                        // glm & opengl uses column major order (normally row major
                        // order is used in linear algebra)
                        glm::mat4 mat = glm::make_mat4(tmpf);
                        if (transpose) {
                            mat = glm::transpose(mat);
                        }
                        usrPtr->setTransform(mat);
                    }
                    else {
                        sgct::MessageHandler::instance()->print(
                            sgct::MessageHandler::Level::Error,
                            "ReadConfig: Failed to parse user matrix in XML!\n"
                        );
                    }
                }
                else if (strcmp("Tracking", childVal) == 0) {
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
        else if (strcmp("Settings", val) == 0) {
            sgct::SGCTSettings::instance()->configure(element);
        }
        else if (strcmp("Capture", val) == 0) {
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
        else if (strcmp("Tracker", val) == 0 && element->Attribute("name")) {
            ClusterManager& cm = *ClusterManager::instance();
            cm.getTrackingManagerPtr().addTracker(
                std::string(element->Attribute("name"))
            );
            
            XMLElement* child = element->FirstChildElement();
            while (child) {
                const char* childVal = child->Value();
                
                if (strcmp("Device", childVal) == 0 && child->Attribute("name")) {
                    cm.getTrackingManagerPtr().addDeviceToCurrentTracker(
                        std::string(child->Attribute("name"))
                    );
                    
                    XMLElement* grandChild = child->FirstChildElement();
                    
                    while (grandChild) {
                        const char* grandChildVal = grandChild->Value();
                        
                        if (strcmp("Sensor", grandChildVal) == 0) {
                            int id;
                            XMLError err = grandChild->QueryIntAttribute("id", &id);
                            if (grandChild->Attribute("vrpnAddress") &&
                                err == XML_NO_ERROR)
                            {
                                cm.getTrackingManagerPtr().addSensorToCurrentDevice(
                                    grandChild->Attribute("vrpnAddress"),
                                    id
                                );
                            }
                        }
                        else if (strcmp("Buttons", grandChildVal) == 0) {
                            unsigned int count = 0;
                            XMLError err = grandChild->QueryUnsignedAttribute(
                                "count",
                                &count
                            );
                            if (grandChild->Attribute("vrpnAddress") &&
                                err == XML_NO_ERROR)
                            {
                                cm.getTrackingManagerPtr().addButtonsToCurrentDevice(
                                    grandChild->Attribute("vrpnAddress"),
                                    count
                                );
                            }
                            
                        }
                        else if (strcmp("Axes", grandChildVal) == 0) {
                            unsigned int count = 0;
                            XMLError err = grandChild->QueryUnsignedAttribute(
                                "count",
                                &count
                            );
                            if (grandChild->Attribute("vrpnAddress") &&
                               err == tinyxml2::XML_NO_ERROR)
                            {
                                cm.getTrackingManagerPtr().addAnalogsToCurrentDevice(
                                    grandChild->Attribute("vrpnAddress"),
                                    count
                                );
                            }
                        }
                        else if (strcmp("Offset", grandChildVal) == 0) {
                            glm::vec3 offset;
                            XMLError err[3] = {
                                grandChild->QueryFloatAttribute("x", &offset[0]),
                                grandChild->QueryFloatAttribute("y", &offset[1]),
                                grandChild->QueryFloatAttribute("z", &offset[2])
                            };

                            if (err[0] == XML_NO_ERROR && err[1] == XML_NO_ERROR &&
                                err[2] == XML_NO_ERROR)
                            {
                                using namespace sgct;
                                SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                                SGCTTracker& tr = *m.getLastTrackerPtr();
                                SGCTTrackingDevice& device = *tr.getLastDevicePtr();

                                device.setOffset(std::move(offset));
                            }
                            else {
                                sgct::MessageHandler::instance()->print(
                                    sgct::MessageHandler::Level::Error,
                                    "ReadConfig: Failed to parse device offset in XML!\n"
                                );
                            }
                        }
                        else if (strcmp("Orientation", grandChildVal) == 0) {
                            using namespace sgct;
                            SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                            SGCTTracker& tr = *m.getLastTrackerPtr();
                            SGCTTrackingDevice& device = *tr.getLastDevicePtr();

                            device.setOrientation(parseOrientationNode(grandChild));
                        }
                        else if (strcmp("Quaternion", grandChildVal) == 0) {
                            glm::quat q;
                            // Yes, this order is correct;  the constructor takes w,x,y,z
                            //but the layout in memory is x,y,z,w
                            XMLError err[4] = {
                                grandChild->QueryFloatAttribute("w", &q[3]),
                                grandChild->QueryFloatAttribute("x", &q[0]),
                                grandChild->QueryFloatAttribute("y", &q[1]),
                                grandChild->QueryFloatAttribute("z", &q[2])
                            };
                            if (err[0] == XML_NO_ERROR && err[1] == XML_NO_ERROR &&
                                err[2] == XML_NO_ERROR && err[3] == XML_NO_ERROR)
                            {
                                using namespace sgct;
                                SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                                SGCTTracker& tr = *m.getLastTrackerPtr();
                                SGCTTrackingDevice& device = *tr.getLastDevicePtr();

                                device.setOrientation(std::move(q));
                            }
                            else {
                                sgct::MessageHandler::instance()->print(
                                    sgct::MessageHandler::Level::Error,
                                    "ReadConfig: Failed to parse device orientation in XML!\n"
                                );
                            }
                        }
                        else if (strcmp("Matrix", grandChildVal) == 0) {
                            bool transpose = true;
                            if (grandChild->Attribute("transpose") != nullptr) {
                                bool v = strcmp(
                                    grandChild->Attribute("transpose"),
                                    "true"
                                ) == 0;
                                transpose = v;
                            }
                            
                            float tmpf[16];
                            XMLError err[16] = {
                                grandChild->QueryFloatAttribute("x0", &tmpf[0]),
                                grandChild->QueryFloatAttribute("y0", &tmpf[1]),
                                grandChild->QueryFloatAttribute("z0", &tmpf[2]),
                                grandChild->QueryFloatAttribute("w0", &tmpf[3]),
                                grandChild->QueryFloatAttribute("x1", &tmpf[4]),
                                grandChild->QueryFloatAttribute("y1", &tmpf[5]),
                                grandChild->QueryFloatAttribute("z1", &tmpf[6]),
                                grandChild->QueryFloatAttribute("w1", &tmpf[7]),
                                grandChild->QueryFloatAttribute("x2", &tmpf[8]),
                                grandChild->QueryFloatAttribute("y2", &tmpf[9]),
                                grandChild->QueryFloatAttribute("z2", &tmpf[10]),
                                grandChild->QueryFloatAttribute("w2", &tmpf[11]),
                                grandChild->QueryFloatAttribute("x3", &tmpf[12]),
                                grandChild->QueryFloatAttribute("y3", &tmpf[13]),
                                grandChild->QueryFloatAttribute("z3", &tmpf[14]),
                                grandChild->QueryFloatAttribute("w3", &tmpf[15])
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
                                // glm & opengl uses column major order (normally row
                                // major order is used in linear algebra)
                                glm::mat4 mat = glm::make_mat4(tmpf);
                                if (transpose) {
                                    mat = glm::transpose(mat);
                                }
                                using namespace sgct;
                                SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                                SGCTTracker& tr = *m.getLastTrackerPtr();
                                SGCTTrackingDevice& device = *tr.getLastDevicePtr();

                                device.setTransform(std::move(mat));
                            }
                            else {
                                sgct::MessageHandler::instance()->print(
                                    sgct::MessageHandler::Level::Error,
                                    "ReadConfig: Failed to parse device matrix in XML!\n"
                                );
                            }
                        }
                        grandChild = grandChild->NextSiblingElement();
                    }
                    
                }
                else if (strcmp("Offset", childVal) == 0) {
                    glm::vec3 offset;
                    XMLError err[3] = {
                        child->QueryFloatAttribute("x", &offset[0]),
                        child->QueryFloatAttribute("y", &offset[1]),
                        child->QueryFloatAttribute("z", &offset[2])
                    };
                    if (err[0] == XML_NO_ERROR && err[1] == XML_NO_ERROR &&
                        err[2] == XML_NO_ERROR)
                    {
                        using namespace sgct;
                        SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                        SGCTTracker& tr = *m.getLastTrackerPtr();
                        SGCTTrackingDevice& device = *tr.getLastDevicePtr();

                        device.setOffset(std::move(offset));
                    }
                    else {
                        sgct::MessageHandler::instance()->print(
                            sgct::MessageHandler::Level::Error,
                            "ReadConfig: Failed to parse tracker offset in XML!\n"
                        );
                    }
                }
                else if (strcmp("Orientation", childVal) == 0) {
                    using namespace sgct;
                    SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                    SGCTTracker& tr = *m.getLastTrackerPtr();
                    SGCTTrackingDevice& device = *tr.getLastDevicePtr();

                    device.setOrientation(parseOrientationNode(child));
                }
                else if (strcmp("Quaternion", childVal) == 0) {
                    glm::quat q;
                    // Yes, this order is correct;  the constructor takes w,x,y,z
                    // but the layout in memory is x,y,z,w
                    XMLError err[4] = {
                        child->QueryFloatAttribute("w", &q[3]),
                        child->QueryFloatAttribute("x", &q[0]),
                        child->QueryFloatAttribute("y", &q[1]),
                        child->QueryFloatAttribute("z", &q[2])
                    };
                    if (err[0] == XML_NO_ERROR && err[1] == XML_NO_ERROR &&
                        err[2] == XML_NO_ERROR && err[3] == XML_NO_ERROR)
                    {
                        using namespace sgct;
                        SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                        SGCTTracker& tr = *m.getLastTrackerPtr();
                        SGCTTrackingDevice& device = *tr.getLastDevicePtr();

                        device.setOrientation(std::move(q));
                    }
                    else {
                        sgct::MessageHandler::instance()->print(
                            sgct::MessageHandler::Level::Error,
                            "ReadConfig: Failed to parse tracker orientation quaternion in XML!\n"
                        );
                    }
                }
                else if (strcmp("Scale", childVal) == 0) {
                    double value;
                    XMLError err = child->QueryDoubleAttribute("value", &value);
                    if (err == tinyxml2::XML_NO_ERROR) {
                        using namespace sgct;
                        SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                        SGCTTracker& tr = *m.getLastTrackerPtr();

                        tr.setScale(value);
                    }
                    else {
                        sgct::MessageHandler::instance()->print(
                            sgct::MessageHandler::Level::Error,
                            "ReadConfig: Failed to parse tracker scale in XML!\n"
                        );
                    }
                }
                else if (strcmp("Matrix", childVal) == 0) {
                    bool transpose = true;
                    if (child->Attribute("transpose")) {
                        bool v = strcmp(child->Attribute("transpose"), "true") == 0;
                        transpose = v;
                    }

                    float tmpf[16];
                    XMLError err[16] = {
                        child->QueryFloatAttribute("x0", &tmpf[0]),
                        child->QueryFloatAttribute("y0", &tmpf[1]),
                        child->QueryFloatAttribute("z0", &tmpf[2]),
                        child->QueryFloatAttribute("w0", &tmpf[3]),
                        child->QueryFloatAttribute("x1", &tmpf[4]),
                        child->QueryFloatAttribute("y1", &tmpf[5]),
                        child->QueryFloatAttribute("z1", &tmpf[6]),
                        child->QueryFloatAttribute("w1", &tmpf[7]),
                        child->QueryFloatAttribute("x2", &tmpf[8]),
                        child->QueryFloatAttribute("y2", &tmpf[9]),
                        child->QueryFloatAttribute("z2", &tmpf[10]),
                        child->QueryFloatAttribute("w2", &tmpf[11]),
                        child->QueryFloatAttribute("x3", &tmpf[12]),
                        child->QueryFloatAttribute("y3", &tmpf[13]),
                        child->QueryFloatAttribute("z3", &tmpf[14]),
                        child->QueryFloatAttribute("w3", &tmpf[15])
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
                        if (transpose) {
                            mat = glm::transpose(mat);
                        }
                        using namespace sgct;
                        SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                        SGCTTracker& tr = *m.getLastTrackerPtr();

                        tr.setTransform(std::move(mat));
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
        element = element->NextSiblingElement();
    }

    return true;
}

bool ReadConfig::isValid() const {
    return mIsValid;
}

glm::quat ReadConfig::parseOrientationNode(tinyxml2::XMLElement* element) {
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

glm::quat ReadConfig::parseMpcdiOrientationNode(float yaw, float pitch, float roll) {
    float x = pitch;
    float y = -yaw;
    float z = -roll;

    glm::quat quat;
    quat = glm::rotate(quat, glm::radians(y), glm::vec3(0.f, 1.f, 0.f));
    quat = glm::rotate(quat, glm::radians(x), glm::vec3(1.f, 0.f, 0.f));
    quat = glm::rotate(quat, glm::radians(z), glm::vec3(0.f, 0.f, 1.f));

    return quat;
}

} // namespace sgct_config
