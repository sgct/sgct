/*************************************************************************
 Copyright (c) 2012-2015 Miroslav Andel
 All rights reserved.
 
 For conditions of distribution and use, see copyright notice in sgct.h
 *************************************************************************/

#define TIXML_USE_STL //needed for tinyXML lib to link properly in mingw
#define MAX_XML_DEPTH 16

#include <sgct/ReadConfig.h>

#include <sgct/ogl_headers.h>
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

} // namespace

namespace sgct_core {

ReadConfig::ReadConfig(std::string filename) {
    valid = false;
    
    if (filename.empty()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Warning,
            "ReadConfig: No file specified! Using default configuration...\n"
        );
        readAndParseXMLString();
        valid = true;
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
        valid = true;
    
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "ReadConfig: Config file '%s' read successfully!\n",
            xmlFileName.c_str()
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
    size_t foundIndex = filename.find('%');
    if (foundIndex != std::string::npos) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "ReadConfig: Error: SGCT doesn't support the usage of '%%' characters "
            "in path or file name.\n"
        );
        return false;
    }
    
    std::vector<size_t> beginEnvVar;
    std::vector<size_t> endEnvVar;
    
    foundIndex = 0;
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
            char* fetchedEnvVar = nullptr;
            
#if (_MSC_VER >= 1400) //visual studio 2005 or later
            size_t len;
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
            fetchedEnvVar = getenv(envVar.c_str());
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
        
        //replace all backslashes with slashes
        for (unsigned int i = 0; i < xmlFileName.size(); i++) {
            if (xmlFileName[i] == 92) {
                //backslash
                xmlFileName[i] = '/';
            }
        }
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
        std::stringstream ss;
        if (xmlDoc.GetErrorStr1() && xmlDoc.GetErrorStr2()) {
            ss << "Parsing failed after: " << xmlDoc.GetErrorStr1()
               << " " << xmlDoc.GetErrorStr2();
        }
        else if (xmlDoc.GetErrorStr1()) {
            ss << "Parsing failed after: " << xmlDoc.GetErrorStr1();
        }
        else if (xmlDoc.GetErrorStr2()) {
            ss << "Parsing failed after: " << xmlDoc.GetErrorStr2();
        }
        else {
            ss << "File not found";
        }
        mErrorMsg = ss.str();
        return false;
    }
    else {
        return readAndParseXML(xmlDoc);
    }
}

bool ReadConfig::readAndParseXMLString() {
    tinyxml2::XMLDocument xmlDoc;
    bool loadSuccess = xmlDoc.Parse(
        DefaultSingleConfiguration,
        strlen(DefaultSingleConfiguration)
    ) == tinyxml2::XML_NO_ERROR;
    
    if (!loadSuccess) {
        std::stringstream ss;
        if (xmlDoc.GetErrorStr1() && xmlDoc.GetErrorStr2()) {
            ss << "Parsing failed after: " << xmlDoc.GetErrorStr1()
               << " " << xmlDoc.GetErrorStr2();
        }
        else if (xmlDoc.GetErrorStr1()) {
            ss << "Parsing failed after: " << xmlDoc.GetErrorStr1();
        }
        else if (xmlDoc.GetErrorStr2()) {
            ss << "Parsing failed after: " << xmlDoc.GetErrorStr2();
        }
        else {
            ss << "File not found";
        }
        mErrorMsg = ss.str();
        assert(false);
        return false;
    }
    else {
        return readAndParseXML(xmlDoc);
    }
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
    if (debugMode != nullptr) {
        sgct::MessageHandler::instance()->setNotifyLevel(
            strcmp(debugMode, "true") == 0 ?
                sgct::MessageHandler::Level::Debug :
                sgct::MessageHandler::Level::Warning
        );
    }
    
    if (XMLroot->Attribute("externalControlPort") != nullptr) {
        std::string tmpStr(XMLroot->Attribute("externalControlPort"));
        ClusterManager::instance()->setExternalControlPort(tmpStr);
    }
    
    if (XMLroot->Attribute("firmSync") != nullptr) {
        ClusterManager::instance()->setFirmFrameLockSyncStatus(
            strcmp(XMLroot->Attribute("firmSync"), "true") == 0
        );
    }
    
    XMLElement* element[MAX_XML_DEPTH];
    for (unsigned int i = 0; i < MAX_XML_DEPTH; i++) {
        element[i] = nullptr;
    }
    const char* val[MAX_XML_DEPTH];
    element[0] = XMLroot->FirstChildElement();
    while (element[0] != nullptr) {
        val[0] = element[0]->Value();
        
        if (strcmp("Scene", val[0]) == 0) {
            element[1] = element[0]->FirstChildElement();
            while (element[1] != nullptr) {
                val[1] = element[1]->Value();
                
                if (strcmp("Offset", val[1]) == 0) {
                    float tmpOffset[] = { 0.f, 0.f, 0.f };
                    XMLError xValue = element[1]->QueryFloatAttribute("x", &tmpOffset[0]);
                    XMLError yValue = element[1]->QueryFloatAttribute("y", &tmpOffset[1]);
                    XMLError zValue = element[1]->QueryFloatAttribute("z", &tmpOffset[2]);
                    if (xValue == XML_NO_ERROR &&
                        yValue == XML_NO_ERROR &&
                        zValue == XML_NO_ERROR)
                    {
                        glm::vec3 sceneOffset = {
                            tmpOffset[0],
                            tmpOffset[1],
                            tmpOffset[2]
                        };
                        sgct::MessageHandler::instance()->print(
                            sgct::MessageHandler::Level::Debug,
                            "ReadConfig: Setting scene offset to (%f, %f, %f)\n",
                            sceneOffset.x, sceneOffset.y, sceneOffset.z
                        );
                        
                        ClusterManager::instance()->setSceneOffset(sceneOffset);
                    }
                    else {
                        sgct::MessageHandler::instance()->print(
                            sgct::MessageHandler::Level::Error,
                            "ReadConfig: Failed to parse scene offset from XML!\n"
                        );
                    }
                }
                else if (strcmp("Orientation", val[1]) == 0) {
                    ClusterManager::instance()->setSceneRotation(
                        glm::mat4_cast(parseOrientationNode(element[1]))
                    );
                }
                else if (strcmp("Scale", val[1]) == 0) {
                    float tmpScale = 1.f;
                    XMLError value = element[1]->QueryFloatAttribute("value", &tmpScale);
                    if (value  == tinyxml2::XML_NO_ERROR) {
                        sgct::MessageHandler::instance()->print(
                            sgct::MessageHandler::Level::Debug,
                            "ReadConfig: Setting scene scale to %f\n",
                            tmpScale
                        );
                       
                        ClusterManager::instance()->setSceneScale(tmpScale);
                    }
                    else {
                        sgct::MessageHandler::instance()->print(
                            sgct::MessageHandler::Level::Error,
                            "ReadConfig: Failed to parse scene orientation from XML!\n"
                        );
                    }
                }
                
                //iterate
                element[1] = element[1]->NextSiblingElement();
            }
        }
        else if (strcmp("Node", val[0]) == 0) {
            SGCTNode tmpNode;
            
            if (element[0]->Attribute("address")) {
                tmpNode.setAddress(element[0]->Attribute("address"));
            }
            if (element[0]->Attribute("name")) {
                tmpNode.setName(element[0]->Attribute("name"));
            }
            if (element[0]->Attribute("ip")) {
                //backward compability with older versions of SGCT config files
                tmpNode.setAddress(element[0]->Attribute("ip"));
            }
            if (element[0]->Attribute("port")) {
                tmpNode.setSyncPort(element[0]->Attribute("port"));
            }
            if (element[0]->Attribute("syncPort")) {
                tmpNode.setSyncPort(element[0]->Attribute("syncPort"));
            }
            if (element[0]->Attribute("dataTransferPort")) {
                tmpNode.setDataTransferPort(element[0]->Attribute("dataTransferPort"));
            }
            
            if (element[0]->Attribute("swapLock") != nullptr) {
                bool useSwapLock = strcmp(element[0]->Attribute("swapLock"), "true") == 0;
                tmpNode.setUseSwapGroups(useSwapLock);
            }
            
            element[1] = element[0]->FirstChildElement();
            while (element[1] != nullptr) {
                val[1] = element[1]->Value();
                if (strcmp("Window", val[1]) == 0) {
                    sgct::SGCTWindow tmpWin(
                        static_cast<int>(tmpNode.getNumberOfWindows())
                    );
                    
                    if (element[1]->Attribute("name") != nullptr) {
                        tmpWin.setName(element[1]->Attribute("name"));
                    }

                    if (element[1]->Attribute("tags") != nullptr) {
                        tmpWin.setTags(element[1]->Attribute("tags"));
                    }

                    if (element[1]->Attribute("bufferBitDepth") != nullptr) {
                        tmpWin.setColorBitDepth(
                            getBufferColorBitDepth(element[1]->Attribute("bufferBitDepth"))
                        );
                    }

                    if (element[1]->Attribute("preferBGR") != nullptr) {
                        tmpWin.setPreferBGR(
                            strcmp(element[1]->Attribute("preferBGR"), "true") == 0
                        );
                    }
                        
                    //compability with older versions
                    if (element[1]->Attribute("fullScreen") != nullptr) {
                        bool v = strcmp(element[1]->Attribute("fullScreen"), "true") == 0;
                        tmpWin.setWindowMode(v);
                    }

                    if (element[1]->Attribute("fullscreen") != nullptr) {
                        bool v = strcmp(element[1]->Attribute("fullscreen"), "true") == 0;
                        tmpWin.setWindowMode(v);
                    }
                    
                    if (element[1]->Attribute("floating") != nullptr) {
                        bool v = strcmp(element[1]->Attribute("floating"), "true") == 0;
                        tmpWin.setFloating(v);
                    }

                    if (element[1]->Attribute("alwaysRender") != nullptr) {
                        bool v =
                            strcmp(element[1]->Attribute("alwaysRender"), "true") == 0;
                        tmpWin.setRenderWhileHidden(v);
                    }

                    if (element[1]->Attribute("hidden") != nullptr) {
                        bool v = strcmp(element[1]->Attribute("hidden"), "true") == 0;
                        tmpWin.setVisibility(!v);
                    }

                    if (element[1]->Attribute("dbuffered") != nullptr) {
                        bool v = strcmp(element[1]->Attribute("dbuffered"), "true") == 0;
                        tmpWin.setDoubleBuffered(v);
                    }

                    float gamma = 0.f;
                    XMLError gammaErr = element[1]->QueryFloatAttribute("gamma", &gamma);
                    if (gammaErr == XML_NO_ERROR && gamma > 0.1f) {
                        tmpWin.setGamma(gamma);
                    }

                    float contrast = -1.f;
                    XMLError contrastErr = element[1]->QueryFloatAttribute(
                        "contrast",
                        &contrast
                    );
                    if (contrastErr == XML_NO_ERROR && contrast > 0.f) {
                        tmpWin.setContrast(contrast);
                    }

                    float brightness = -1.f;
                    XMLError brightnessErr = element[1]->QueryFloatAttribute(
                        "brightness",
                        &brightness
                    );
                    if (brightnessErr == XML_NO_ERROR && brightness > 0.f) {
                        tmpWin.setBrightness(brightness);
                    }
                    
                    int tmpSamples = 0;
                    //compability with older versions

                    XMLError sampleErr = element[1]->QueryIntAttribute(
                        "numberOfSamples",
                        &tmpSamples
                    );
                    XMLError msaaErr = element[1]->QueryIntAttribute("msaa", &tmpSamples);
                    XMLError msErr = element[1]->QueryIntAttribute("MSAA", &tmpSamples);

                    if (sampleErr == XML_NO_ERROR && tmpSamples <= 128) {
                        tmpWin.setNumberOfAASamples(tmpSamples);
                    }
                    else if (msaaErr == XML_NO_ERROR && tmpSamples <= 128) {
                        tmpWin.setNumberOfAASamples(tmpSamples);
                    }
                    else if (msErr == XML_NO_ERROR && tmpSamples <= 128) {
                        tmpWin.setNumberOfAASamples(tmpSamples);
                    }
                    
                    if (element[1]->Attribute("alpha") != nullptr) {
                        bool v = strcmp(element[1]->Attribute("alpha"), "true") == 0;
                        tmpWin.setAlpha(v);
                    }
                    
                    if (element[1]->Attribute("fxaa") != nullptr) {
                        bool v = strcmp(element[1]->Attribute("fxaa"), "true") == 0;
                        tmpWin.setUseFXAA(v);
                    }
                    
                    if (element[1]->Attribute("FXAA") != nullptr) {
                        bool v = strcmp(element[1]->Attribute("FXAA"), "true") == 0;
                        tmpWin.setUseFXAA(v);
                    }
                    
                    if (element[1]->Attribute("decorated") != nullptr) {
                        bool v = strcmp(element[1]->Attribute("decorated"), "true") == 0;
                        tmpWin.setWindowDecoration(v);
                    }
                    
                    if (element[1]->Attribute("border") != nullptr) {
                        bool v = strcmp(element[1]->Attribute("border"), "true") == 0;
                        tmpWin.setWindowDecoration(v);
                    }

                    if (element[1]->Attribute("draw2D") != nullptr) {
                        bool v = strcmp(element[1]->Attribute("draw2D"), "true") == 0;
                        tmpWin.setCallDraw2DFunction(v);
                    }

                    if (element[1]->Attribute("draw3D") != nullptr) {
                        bool v = strcmp(element[1]->Attribute("draw3D"), "true") == 0;
                        tmpWin.setCallDraw3DFunction(v);
                    }

                    if (element[1]->Attribute("copyPreviousWindowToCurrentWindow") != nullptr) {
                        bool v = strcmp(
                            element[1]->Attribute("copyPreviousWindowToCurrentWindow"),
                            "true"
                        ) == 0;
                        tmpWin.setCopyPreviousWindowToCurrentWindow(v);
                    }
                    
                    int tmpMonitorIndex = 0;
                    XMLError monitorErr = element[1]->QueryIntAttribute(
                        "monitor",
                        &tmpMonitorIndex
                    );
                    if (monitorErr == tinyxml2::XML_NO_ERROR) {
                        tmpWin.setFullScreenMonitorIndex(tmpMonitorIndex);
                    }
                    
                    if (element[1]->Attribute("mpcdi") != nullptr) {
                        sgct_core::SGCTMpcdi mpcdiHandler(mErrorMsg);
                        std::string pathToMpcdiFile;
                        size_t lastSlashPos = xmlFileName.find_last_of("/");
                        if (lastSlashPos != std::string::npos) {
                            pathToMpcdiFile = xmlFileName.substr(0, lastSlashPos) + "/";
                        }
                        pathToMpcdiFile += element[1]->Attribute("mpcdi");
                        //replace all backslashes with slashes
                        std::replace(
                            pathToMpcdiFile.begin(),
                            pathToMpcdiFile.end(),
                            '\\',
                            '/'
                        );
                        bool parse = mpcdiHandler.parseConfiguration(
                            pathToMpcdiFile,
                            tmpNode,
                            tmpWin
                        );
                        if (!parse) {
                            return false;
                        }
                    }

                    element[2] = element[1]->FirstChildElement();
                    while( element[2] != nullptr) {
                        val[2] = element[2]->Value();
                        int tmpWinData[2];
                        memset(tmpWinData, 0, 4);
                        
                        if (strcmp("Stereo", val[2]) == 0) {
                            sgct::SGCTWindow::StereoMode v = getStereoType(
                                element[2]->Attribute("type")
                            );
                            tmpWin.setStereoMode(v);
                        }
                        else if (strcmp("Pos", val[2]) == 0) {
                            XMLError xErr = element[2]->QueryIntAttribute(
                                "x",
                                &tmpWinData[0]
                            );
                            XMLError yErr = element[2]->QueryIntAttribute(
                                "y",
                                &tmpWinData[1]
                            );
                            if (xErr == XML_NO_ERROR && yErr == XML_NO_ERROR) {
                                tmpWin.setWindowPosition(tmpWinData[0], tmpWinData[1]);
                            }
                            else {
                                sgct::MessageHandler::instance()->print(
                                    sgct::MessageHandler::Level::Error,
                                    "ReadConfig: Failed to parse window position from XML!\n"
                                );
                            }
                        }
                        else if (strcmp("Size", val[2]) == 0) {
                            XMLError xErr = element[2]->QueryIntAttribute(
                                "x",
                                &tmpWinData[0]
                            );
                            XMLError yErr = element[2]->QueryIntAttribute(
                                "y",
                                &tmpWinData[1]
                            );

                            if (xErr == XML_NO_ERROR && yErr == XML_NO_ERROR) {
                                tmpWin.initWindowResolution(tmpWinData[0], tmpWinData[1]);
                            }
                            else {
                                sgct::MessageHandler::instance()->print(
                                    sgct::MessageHandler::Level::Error,
                                    "ReadConfig: Failed to parse window resolution from XML!\n"
                                );
                            }
                        }
                        else if (strcmp("Res", val[2]) == 0) {
                            XMLError xErr = element[2]->QueryIntAttribute(
                                "x",
                                &tmpWinData[0]
                            );
                            XMLError yErr = element[2]->QueryIntAttribute(
                                "y",
                                &tmpWinData[1]
                            );

                            if (xErr == XML_NO_ERROR && yErr == XML_NO_ERROR) {
                                tmpWin.setFramebufferResolution(
                                    tmpWinData[0],
                                    tmpWinData[1]
                                );
                                tmpWin.setFixResolution(true);
                            }
                            else {
                                sgct::MessageHandler::instance()->print(
                                    sgct::MessageHandler::Level::Error,
                                    "ReadConfig: Failed to parse frame buffer resolution from XML!\n"
                                );
                            }
                        }
                        else if (strcmp("Viewport", val[2]) == 0) {
                            Viewport* vpPtr = new sgct_core::Viewport();
                            vpPtr->configure(element[2]);
                            tmpWin.addViewport(vpPtr);
                        }
                        
                        //iterate
                        element[2] = element[2]->NextSiblingElement();
                    }
                    
                    tmpNode.addWindow(tmpWin);
                }//end window
                
                //iterate
                element[1] = element[1]->NextSiblingElement();
                
            }//end while
            
            ClusterManager::instance()->addNode(tmpNode);
        }//end if node
        else if (strcmp("User", val[0]) == 0) {
            SGCTUser* usrPtr;
            if (element[0]->Attribute("name") != nullptr) {
                std::string name(element[0]->Attribute("name"));
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
            XMLError eyeSepErr = element[0]->QueryFloatAttribute("eyeSeparation", &eyeSep);

            if (eyeSepErr == XML_NO_ERROR) {
                usrPtr->setEyeSeparation(eyeSep);
            }
            
            element[1] = element[0]->FirstChildElement();
            while (element[1] != nullptr) {
                val[1] = element[1]->Value();
                
                if (strcmp("Pos", val[1]) == 0) {
                    float fTmp[3];
                    XMLError xErr = element[1]->QueryFloatAttribute("x", &fTmp[0]);
                    XMLError yErr = element[1]->QueryFloatAttribute("y", &fTmp[1]);
                    XMLError zErr = element[1]->QueryFloatAttribute("z", &fTmp[2]);
                    if (xErr == XML_NO_ERROR &&
                        yErr == XML_NO_ERROR &&
                        zErr == XML_NO_ERROR)
                    {
                        usrPtr->setPos(fTmp);
                    }
                    else {
                        sgct::MessageHandler::instance()->print(
                            sgct::MessageHandler::Level::Error,
                            "ReadConfig: Failed to parse user position from XML!\n"
                        );
                    }
                }
                else if (strcmp("Orientation", val[1]) == 0) {
                    usrPtr->setOrientation(parseOrientationNode(element[1]));
                }
                else if (strcmp("Quaternion", val[1]) == 0) {
                    float tmpd[4];
                    XMLError wErr = element[1]->QueryFloatAttribute("w", &tmpd[0]);
                    XMLError xErr = element[1]->QueryFloatAttribute("x", &tmpd[1]);
                    XMLError yErr = element[1]->QueryFloatAttribute("y", &tmpd[2]);
                    XMLError zErr = element[1]->QueryFloatAttribute("z", &tmpd[3]);
                    if (wErr == tinyxml2::XML_NO_ERROR &&
                        xErr == tinyxml2::XML_NO_ERROR &&
                        yErr == tinyxml2::XML_NO_ERROR &&
                        zErr == tinyxml2::XML_NO_ERROR)
                    {
                        glm::quat q(tmpd[0], tmpd[1], tmpd[2], tmpd[3]);
                        usrPtr->setOrientation(q);
                    }
                    else {
                        sgct::MessageHandler::instance()->print(
                            sgct::MessageHandler::Level::Error,
                            "ReadConfig: Failed to parse device orientation in XML!\n"
                        );
                    }
                }
                else if (strcmp("Matrix", val[1]) == 0) {
                    bool transpose = true;
                    if (element[1]->Attribute("transpose") != nullptr) {
                        bool v = strcmp(element[1]->Attribute("transpose"), "true") == 0;
                        transpose = v;
                    }

                    float tmpf[16];
                    XMLError err[16] = {
                        element[1]->QueryFloatAttribute("x0", &tmpf[ 0]),
                        element[1]->QueryFloatAttribute("y0", &tmpf[ 1]),
                        element[1]->QueryFloatAttribute("z0", &tmpf[ 2]),
                        element[1]->QueryFloatAttribute("w0", &tmpf[ 3]),
                        element[1]->QueryFloatAttribute("x1", &tmpf[ 4]),
                        element[1]->QueryFloatAttribute("y1", &tmpf[ 5]),
                        element[1]->QueryFloatAttribute("z1", &tmpf[ 6]),
                        element[1]->QueryFloatAttribute("w1", &tmpf[ 7]),
                        element[1]->QueryFloatAttribute("x2", &tmpf[ 8]),
                        element[1]->QueryFloatAttribute("y2", &tmpf[ 9]),
                        element[1]->QueryFloatAttribute("z2", &tmpf[10]),
                        element[1]->QueryFloatAttribute("w2", &tmpf[11]),
                        element[1]->QueryFloatAttribute("x3", &tmpf[12]),
                        element[1]->QueryFloatAttribute("y3", &tmpf[13]),
                        element[1]->QueryFloatAttribute("z3", &tmpf[14]),
                        element[1]->QueryFloatAttribute("w3", &tmpf[15])
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
                        //glm & opengl uses column major order (normally row major
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
                else if (strcmp("Tracking", val[1]) == 0) {
                    const char* tracker = element[1]->Attribute("tracker");
                    const char* device = element[1]->Attribute("device");

                    if (tracker != nullptr && device != nullptr) {
                        usrPtr->setHeadTracker(tracker, device);
                    }
                    else {
                        sgct::MessageHandler::instance()->print(
                            sgct::MessageHandler::Level::Error,
                            "ReadConfig: Failed to parse user tracking data from XML!\n"
                        );
                    }
                }
                
                //iterate
                element[1] = element[1]->NextSiblingElement();
            }
        } //end user
        else if (strcmp("Settings", val[0]) == 0) {
            sgct::SGCTSettings::instance()->configure(element[0]);
        } //end settings
        else if (strcmp("Capture", val[0]) == 0) {
            if (element[0]->Attribute("path") != nullptr) {
                using namespace sgct;

                const char* p = element[0]->Attribute("path");
                SGCTSettings::instance()->setCapturePath(p, SGCTSettings::Mono);
                SGCTSettings::instance()->setCapturePath(p, SGCTSettings::LeftStereo);
                SGCTSettings::instance()->setCapturePath(p, SGCTSettings::RightStereo);
            }
            if (element[0]->Attribute("monoPath") != nullptr) {
                using namespace sgct;
                const char* p = element[0]->Attribute("monoPath");
                SGCTSettings::instance()->setCapturePath(p, SGCTSettings::Mono);
            }
            if (element[0]->Attribute("leftPath") != nullptr) {
                using namespace sgct;
                const char* p = element[0]->Attribute("leftPath");
                SGCTSettings::instance()->setCapturePath(p, SGCTSettings::LeftStereo);
            }
            if (element[0]->Attribute("rightPath") != nullptr) {
                using namespace sgct;
                const char* rPath = element[0]->Attribute("rightPath");
                SGCTSettings::instance()->setCapturePath(rPath, SGCTSettings::RightStereo);
            }
            
            if (element[0]->Attribute("format") != nullptr) {
                const char* f = element[0]->Attribute("format");
                sgct::SGCTSettings::instance()->setCaptureFormat(f);
            }
        }
        else if (strcmp("Tracker", val[0]) == 0 &&
                 element[0]->Attribute("name") != nullptr)
        {
        ClusterManager& cm = *ClusterManager::instance();
            cm.getTrackingManagerPtr().addTracker(
                std::string(element[0]->Attribute("name"))
            );
            
            element[1] = element[0]->FirstChildElement();
            while( element[1] != nullptr) {
                val[1] = element[1]->Value();
                
                if (strcmp("Device", val[1]) == 0 &&
                    element[1]->Attribute("name") != nullptr)
                {
                    cm.getTrackingManagerPtr().addDeviceToCurrentTracker(
                        std::string(element[1]->Attribute("name"))
                    );
                    
                    element[2] = element[1]->FirstChildElement();
                    
                    while( element[2] != nullptr) {
                        val[2] = element[2]->Value();
                        unsigned int tmpUI = 0;
                        int tmpi = -1;
                        
                        if (strcmp("Sensor", val[2]) == 0) {
                            XMLError err = element[2]->QueryIntAttribute("id", &tmpi);
                            if (element[2]->Attribute("vrpnAddress") != nullptr &&
                                err == XML_NO_ERROR )
                            {
                                cm.getTrackingManagerPtr().addSensorToCurrentDevice(
                                    element[2]->Attribute("vrpnAddress"),
                                    tmpi
                                );
                            }
                        }
                        else if (strcmp("Buttons", val[2]) == 0) {
                            XMLError err = element[2]->QueryUnsignedAttribute(
                                "count",
                                &tmpUI
                            );
                            if (element[2]->Attribute("vrpnAddress") != nullptr &&
                               err == XML_NO_ERROR)
                            {
                                cm.getTrackingManagerPtr().addButtonsToCurrentDevice(
                                    element[2]->Attribute("vrpnAddress"),
                                    tmpUI
                                );
                            }
                            
                        }
                        else if (strcmp("Axes", val[2]) == 0) {
                            XMLError err = element[2]->QueryUnsignedAttribute(
                                "count",
                                &tmpUI
                            );
                            if (element[2]->Attribute("vrpnAddress") != nullptr &&
                               err == tinyxml2::XML_NO_ERROR)
                            {
                                cm.getTrackingManagerPtr().addAnalogsToCurrentDevice(
                                    element[2]->Attribute("vrpnAddress"),
                                    tmpUI
                                );
                            }
                        }
                        else if (strcmp("Offset", val[2]) == 0) {
                            float tmpf[3];
                            XMLError err[3] = {
                                element[2]->QueryFloatAttribute("x", &tmpf[0]),
                                element[2]->QueryFloatAttribute("y", &tmpf[1]),
                                element[2]->QueryFloatAttribute("z", &tmpf[2])
                            };

                            if (err[0] == XML_NO_ERROR && err[1] == XML_NO_ERROR &&
                                err[2] == XML_NO_ERROR)
                            {
                                using namespace sgct;
                                SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                                SGCTTracker& tr = *m.getLastTrackerPtr();
                                SGCTTrackingDevice& device = *tr.getLastDevicePtr();

                                device.setOffset(tmpf[0], tmpf[1], tmpf[2]);
                            }
                            else {
                                sgct::MessageHandler::instance()->print(
                                    sgct::MessageHandler::Level::Error,
                                    "ReadConfig: Failed to parse device offset in XML!\n"
                                );
                            }
                        }
                        else if (strcmp("Orientation", val[2]) == 0) {
                            using namespace sgct;
                            SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                            SGCTTracker& tr = *m.getLastTrackerPtr();
                            SGCTTrackingDevice& device = *tr.getLastDevicePtr();

                            device.setOrientation(parseOrientationNode(element[2]));
                        }
                        else if (strcmp("Quaternion", val[2]) == 0) {
                            float tmpf[4];
                            XMLError err[4] = {
                                element[2]->QueryFloatAttribute("w", &tmpf[0]),
                                element[2]->QueryFloatAttribute("x", &tmpf[1]),
                                element[2]->QueryFloatAttribute("y", &tmpf[2]),
                                element[2]->QueryFloatAttribute("z", &tmpf[3])
                            };
                            if (err[0] == XML_NO_ERROR && err[1] == XML_NO_ERROR &&
                                err[2] == XML_NO_ERROR && err[3] == XML_NO_ERROR)
                            {
                                using namespace sgct;
                                SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                                SGCTTracker& tr = *m.getLastTrackerPtr();
                                SGCTTrackingDevice& device = *tr.getLastDevicePtr();

                                device.setOrientation(tmpf[0], tmpf[1], tmpf[2], tmpf[3]);
                            }
                            else {
                                sgct::MessageHandler::instance()->print(
                                    sgct::MessageHandler::Level::Error,
                                    "ReadConfig: Failed to parse device orientation in XML!\n"
                                );
                            }
                        }
                        else if (strcmp("Matrix", val[2]) == 0) {
                            bool transpose = true;
                            if (element[2]->Attribute("transpose") != nullptr) {
                                bool v = strcmp(
                                    element[2]->Attribute("transpose"),
                                    "true"
                                ) == 0;
                                transpose = v;
                            }
                            
                            float tmpf[16];
                            XMLError err[16] = {
                                element[2]->QueryFloatAttribute("x0", &tmpf[0]),
                                element[2]->QueryFloatAttribute("y0", &tmpf[1]),
                                element[2]->QueryFloatAttribute("z0", &tmpf[2]),
                                element[2]->QueryFloatAttribute("w0", &tmpf[3]),
                                element[2]->QueryFloatAttribute("x1", &tmpf[4]),
                                element[2]->QueryFloatAttribute("y1", &tmpf[5]),
                                element[2]->QueryFloatAttribute("z1", &tmpf[6]),
                                element[2]->QueryFloatAttribute("w1", &tmpf[7]),
                                element[2]->QueryFloatAttribute("x2", &tmpf[8]),
                                element[2]->QueryFloatAttribute("y2", &tmpf[9]),
                                element[2]->QueryFloatAttribute("z2", &tmpf[10]),
                                element[2]->QueryFloatAttribute("w2", &tmpf[11]),
                                element[2]->QueryFloatAttribute("x3", &tmpf[12]),
                                element[2]->QueryFloatAttribute("y3", &tmpf[13]),
                                element[2]->QueryFloatAttribute("z3", &tmpf[14]),
                                element[2]->QueryFloatAttribute("w3", &tmpf[15])
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

                                device.setTransform(mat);
                            }
                            else {
                                sgct::MessageHandler::instance()->print(
                                    sgct::MessageHandler::Level::Error,
                                    "ReadConfig: Failed to parse device matrix in XML!\n"
                                );
                            }
                        }
                        
                        //iterate
                        element[2] = element[2]->NextSiblingElement();
                    }
                    
                }
                else if ( strcmp("Offset", val[1]) == 0) {
                    float tmpf[3];
                    XMLError err[3] = {
                        element[1]->QueryFloatAttribute("x", &tmpf[0]),
                        element[1]->QueryFloatAttribute("z", &tmpf[1]),
                        element[1]->QueryFloatAttribute("y", &tmpf[2])
                    };
                    if (err[0] == XML_NO_ERROR && err[1] == XML_NO_ERROR &&
                        err[2] == XML_NO_ERROR)
                    {
                        using namespace sgct;
                        SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                        SGCTTracker& tr = *m.getLastTrackerPtr();
                        SGCTTrackingDevice& device = *tr.getLastDevicePtr();

                        device.setOffset(tmpf[0], tmpf[1], tmpf[2]);
                    }
                    else {
                        sgct::MessageHandler::instance()->print(
                            sgct::MessageHandler::Level::Error,
                            "ReadConfig: Failed to parse tracker offset in XML!\n"
                        );
                    }
                }
                else if (strcmp("Orientation", val[1]) == 0) {
                    using namespace sgct;
                    SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                    SGCTTracker& tr = *m.getLastTrackerPtr();
                    SGCTTrackingDevice& device = *tr.getLastDevicePtr();

                    device.setOrientation(parseOrientationNode(element[1]));
                }
                else if (strcmp("Quaternion", val[1]) == 0) {
                    float tmpf[4];
                    XMLError err[4] = {
                        element[1]->QueryFloatAttribute("w", &tmpf[0]),
                        element[1]->QueryFloatAttribute("x", &tmpf[1]),
                        element[1]->QueryFloatAttribute("y", &tmpf[2]),
                        element[1]->QueryFloatAttribute("z", &tmpf[3])
                    };
                    if (err[0] == XML_NO_ERROR && err[1] == XML_NO_ERROR &&
                        err[2] == XML_NO_ERROR && err[3] == XML_NO_ERROR)
                    {
                        using namespace sgct;
                        SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                        SGCTTracker& tr = *m.getLastTrackerPtr();
                        SGCTTrackingDevice& device = *tr.getLastDevicePtr();

                        device.setOrientation(tmpf[0], tmpf[1], tmpf[2], tmpf[3]);
                    }
                    else {
                        sgct::MessageHandler::instance()->print(
                            sgct::MessageHandler::Level::Error,
                            "ReadConfig: Failed to parse tracker orientation quaternion in XML!\n"
                        );
                    }
                }
                else if (strcmp("Scale", val[1]) == 0) {
                    double scaleVal;
                    XMLError err = element[1]->QueryDoubleAttribute("value", &scaleVal);
                    if (err == tinyxml2::XML_NO_ERROR) {
                        using namespace sgct;
                        SGCTTrackingManager& m = cm.getTrackingManagerPtr();
                        SGCTTracker& tr = *m.getLastTrackerPtr();

                        tr.setScale(scaleVal);
                    }
                    else {
                        sgct::MessageHandler::instance()->print(
                            sgct::MessageHandler::Level::Error,
                            "ReadConfig: Failed to parse tracker scale in XML!\n"
                        );
                    }
                }
                else if (strcmp("Matrix", val[1]) == 0) {
                    bool transpose = true;
                    if (element[1]->Attribute("transpose") != nullptr) {
                        bool v = strcmp(element[1]->Attribute("transpose"), "true") == 0;
                        transpose = v;
                    }

                    float tmpf[16];
                    XMLError err[16] = {
                        element[1]->QueryFloatAttribute("x0", &tmpf[0]),
                        element[1]->QueryFloatAttribute("y0", &tmpf[1]),
                        element[1]->QueryFloatAttribute("z0", &tmpf[2]),
                        element[1]->QueryFloatAttribute("w0", &tmpf[3]),
                        element[1]->QueryFloatAttribute("x1", &tmpf[4]),
                        element[1]->QueryFloatAttribute("y1", &tmpf[5]),
                        element[1]->QueryFloatAttribute("z1", &tmpf[6]),
                        element[1]->QueryFloatAttribute("w1", &tmpf[7]),
                        element[1]->QueryFloatAttribute("x2", &tmpf[8]),
                        element[1]->QueryFloatAttribute("y2", &tmpf[9]),
                        element[1]->QueryFloatAttribute("z2", &tmpf[10]),
                        element[1]->QueryFloatAttribute("w2", &tmpf[11]),
                        element[1]->QueryFloatAttribute("x3", &tmpf[12]),
                        element[1]->QueryFloatAttribute("y3", &tmpf[13]),
                        element[1]->QueryFloatAttribute("z3", &tmpf[14]),
                        element[1]->QueryFloatAttribute("w3", &tmpf[15])
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

                        tr.setTransform(mat);
                    }
                    else {
                        sgct::MessageHandler::instance()->print(
                            sgct::MessageHandler::Level::Error,
                            "ReadConfig: Failed to parse tracker matrix in XML!\n"
                        );
                    }
                }
                
                // iterate
                element[1] = element[1]->NextSiblingElement();
            }
        } // end tracking part
        
        //iterate
        element[0] = element[0]->NextSiblingElement();
    }

    return true;
}

sgct::SGCTWindow::StereoMode ReadConfig::getStereoType(std::string type) {
    std::transform(type.begin(), type.end(), type.begin(), ::tolower);
    
    if (type == "none" || type == "no_stereo") {
        return sgct::SGCTWindow::No_Stereo;
    }
    else if (type == "active" || type == "quadbuffer") {
        return sgct::SGCTWindow::Active_Stereo;
    }
    else if (type == "checkerboard") {
        return sgct::SGCTWindow::Checkerboard_Stereo;
    }
    else if (type == "checkerboard_inverted") {
        return sgct::SGCTWindow::Checkerboard_Inverted_Stereo;
    }
    else if (type == "anaglyph_red_cyan") {
        return sgct::SGCTWindow::Anaglyph_Red_Cyan_Stereo;
    }
    else if (type == "anaglyph_amber_blue") {
        return sgct::SGCTWindow::Anaglyph_Amber_Blue_Stereo;
    }
    else if (type == "anaglyph_wimmer") {
        return sgct::SGCTWindow::Anaglyph_Red_Cyan_Wimmer_Stereo;
    }
    else if (type == "vertical_interlaced") {
        return sgct::SGCTWindow::Vertical_Interlaced_Stereo;
    }
    else if (type == "vertical_interlaced_inverted") {
        return sgct::SGCTWindow::Vertical_Interlaced_Inverted_Stereo;
    }
    else if (type == "test" || type == "dummy") {
        return sgct::SGCTWindow::Dummy_Stereo;
    }
    else if (type == "side_by_side") {
        return sgct::SGCTWindow::Side_By_Side_Stereo;
    }
    else if (type == "side_by_side_inverted") {
        return sgct::SGCTWindow::Side_By_Side_Inverted_Stereo;
    }
    else if (type == "top_bottom") {
        return sgct::SGCTWindow::Top_Bottom_Stereo;
    }
    else if (type == "top_bottom_inverted") {
        return sgct::SGCTWindow::Top_Bottom_Inverted_Stereo;
    }
    
    //if match not found
    return sgct::SGCTWindow::No_Stereo;
}

sgct::SGCTWindow::ColorBitDepth ReadConfig::getBufferColorBitDepth(std::string type) {
    std::transform(type.begin(), type.end(), type.begin(), ::tolower);

    if (type == "8") {
        return sgct::SGCTWindow::BufferColorBitDepth8;
    }
    else if (type == "16") {
        return sgct::SGCTWindow::BufferColorBitDepth16;
    }
    
    else if (type == "16f") {
        return sgct::SGCTWindow::BufferColorBitDepth16Float;
    }
    else if (type == "32f") {
        return sgct::SGCTWindow::BufferColorBitDepth32Float;
    }

    else if (type == "16i") {
        return sgct::SGCTWindow::BufferColorBitDepth16Int;
    }
    else if (type == "32i") {
        return sgct::SGCTWindow::BufferColorBitDepth32Int;
    }

    else if (type == "16ui") {
        return sgct::SGCTWindow::BufferColorBitDepth16UInt;
    }
    else if (type == "32ui") {
        return sgct::SGCTWindow::BufferColorBitDepth32UInt;
    }

    //default
    return sgct::SGCTWindow::BufferColorBitDepth8;
}

bool ReadConfig::isValid() const {
    return valid;
}

glm::quat ReadConfig::parseOrientationNode(tinyxml2::XMLElement* element) {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
    float tmpf;

    bool eulerMode = false;
    bool quatMode = false;

    glm::quat quat;

    if (element->QueryFloatAttribute("w", &tmpf) == tinyxml2::XML_NO_ERROR) {
        quat.w = tmpf;
        quatMode = true;
    }

    if (element->QueryFloatAttribute("y", &tmpf) == tinyxml2::XML_NO_ERROR) {
        y = tmpf;
        eulerMode = true;
    }

    if (element->QueryFloatAttribute("yaw", &tmpf) == tinyxml2::XML_NO_ERROR) {
        y = -tmpf;
    }

    if (element->QueryFloatAttribute("heading", &tmpf) == tinyxml2::XML_NO_ERROR) {
        y = -tmpf;
    }

    if (element->QueryFloatAttribute("azimuth", &tmpf) == tinyxml2::XML_NO_ERROR) {
        y = -tmpf;
    }

    if (element->QueryFloatAttribute("x", &tmpf) == tinyxml2::XML_NO_ERROR) {
        x = tmpf;
        eulerMode = true;
    }

    if (element->QueryFloatAttribute("pitch", &tmpf) == tinyxml2::XML_NO_ERROR) {
        x = tmpf;
    }

    if (element->QueryFloatAttribute("elevation", &tmpf) == tinyxml2::XML_NO_ERROR) {
        x = tmpf;
    }

    if (element->QueryFloatAttribute("z", &tmpf) == tinyxml2::XML_NO_ERROR) {
        z = tmpf;
        eulerMode = true;
    }

    if (element->QueryFloatAttribute("roll", &tmpf) == tinyxml2::XML_NO_ERROR) {
        z = -tmpf;
    }

    if (element->QueryFloatAttribute("bank", &tmpf) == tinyxml2::XML_NO_ERROR) {
        z = -tmpf;
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
