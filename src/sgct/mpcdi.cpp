/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/mpcdi.h>

#include <sgct/clustermanager.h>
#include <sgct/messagehandler.h>
#include <sgct/readconfig.h>
#include <sgct/viewport.h>
#include <algorithm>
#include <sstream>
#include "unzip.h"
#include <zip.h>

namespace {
    [[nodiscard]] glm::quat parseOrientationNode(float yaw, float pitch, float roll) {
        const float x = pitch;
        const float y = -yaw;
        const float z = -roll;

        glm::quat quat;
        quat = glm::rotate(quat, glm::radians(y), glm::vec3(0.f, 1.f, 0.f));
        quat = glm::rotate(quat, glm::radians(x), glm::vec3(1.f, 0.f, 0.f));
        quat = glm::rotate(quat, glm::radians(z), glm::vec3(0.f, 0.f, 1.f));

        return quat;
    }

    [[nodiscard]] sgct::config::MpcdiProjection parseMpcdi(tinyxml2::XMLElement* element) {
        using namespace tinyxml2;

        sgct::config::MpcdiProjection proj;
        if (element->Attribute("id")) {
            proj.id = element->Attribute("id");
        }

        glm::vec2 vpPosition;
        if (element->QueryFloatAttribute("x", &vpPosition[0]) == XML_NO_ERROR &&
            element->QueryFloatAttribute("y", &vpPosition[1]) == XML_NO_ERROR)
        {
            proj.position = vpPosition;
        }
        else {
            sgct::MessageHandler::printError(
                "Viewport: Failed to parse position from XML"
            );
        }

        glm::vec2 vpSize;
        if (element->QueryFloatAttribute("xSize", &vpSize[0]) == XML_NO_ERROR &&
            element->QueryFloatAttribute("ySize", &vpSize[1]) == XML_NO_ERROR)
        {
            proj.size = vpSize;
        }
        else {
            sgct::MessageHandler::printError(
                "Viewport: Failed to parse size from XML"
            );
        }

        // @TODO (abock, 2019-09-23)  Do something with the resolution
        //glm::vec2 vpResolution;
        //if (element->QueryFloatAttribute("xResolution", &vpResolution[0]) == XML_NO_ERROR &&
        //    element->QueryFloatAttribute("yResolution", &vpResolution[1]) == XML_NO_ERROR)
        //{
        //    float expectedResolutionX = std::floor(vpResolution.x * winResX);
        //    float expectedResolutionY = std::floor(vpResolution.y * winResY);

        //    if (expectedResolutionX != vpResolution.x ||
        //        expectedResolutionY != vpResolution.y)
        //    {
        //        MessageHandler::instance()->printWarning(
        //            "Viewport: MPCDI region expected resolution does not match window"
        //        );
        //    }

        //}
        //else {
        //    MessageHandler::instance()->printError(
        //        "Viewport: Failed to parse resolution from XML"
        //    );
        //}

        float yaw = 0.f;
        float pitch = 0.f;
        float roll = 0.f;
        tinyxml2::XMLElement* child = element->FirstChildElement();
        while (child) {
            std::string_view val = child->Value();
            if (val == "frustum") {
                bool hasRight = false;
                bool hasLeft = false;
                bool hasUp = false;
                bool hasDown = false;
                bool hasYaw = false;
                bool hasPitch = false;
                bool hasRoll = false;
                sgct::config::MpcdiProjection::Frustum frustum;
                tinyxml2::XMLElement* grandChild = child->FirstChildElement();
                while (grandChild) {
                    try {
                        std::string_view grandChildVal = grandChild->Value();
                        if (grandChildVal == "rightAngle") {
                            frustum.right = std::stof(grandChild->GetText());
                            hasRight = true;
                        }
                        else if (grandChildVal == "leftAngle") {
                            frustum.left = std::stof(grandChild->GetText());
                            hasLeft = true;
                        }
                        else if (grandChildVal == "upAngle") {
                            frustum.up = std::stof(grandChild->GetText());
                            hasUp = true;
                        }
                        else if (grandChildVal == "downAngle") {
                            frustum.down = std::stof(grandChild->GetText());
                            hasDown = true;
                        }
                        else if (grandChildVal == "yaw") {
                            yaw = std::stof(grandChild->GetText());
                            hasYaw = true;
                        }
                        else if (grandChildVal == "pitch") {
                            pitch = std::stof(grandChild->GetText());
                            hasPitch = true;
                        }
                        else if (grandChildVal == "roll") {
                            roll = std::stof(grandChild->GetText());
                            hasRoll = true;
                        }
                    }
                    catch (const std::invalid_argument&) {
                        sgct::MessageHandler::printError(
                            "Viewport: Failed to parse frustum element from MPCDI XML"
                        );
                    }

                    grandChild = grandChild->NextSiblingElement();
                }


                const bool hasMissingField = !hasDown || !hasUp || !hasLeft || !hasRight ||
                    !hasYaw || !hasPitch || !hasRoll;

                if (hasMissingField) {
                    sgct::MessageHandler::printError(
                        "Viewport: Failed to parse mpcdi projection FOV from XML"
                    );
                    return {};
                }

                const float x = pitch;
                const float y = -yaw;
                const float z = -roll;
                glm::quat quat;
                quat = glm::rotate(quat, glm::radians(y), glm::vec3(0.f, 1.f, 0.f));
                quat = glm::rotate(quat, glm::radians(x), glm::vec3(1.f, 0.f, 0.f));
                quat = glm::rotate(quat, glm::radians(z), glm::vec3(0.f, 0.f, 1.f));
                proj.orientation = quat;
            }
            child = child->NextSiblingElement();
        }

        return proj;
    }



    bool doesStringHaveSuffix(const std::string& str, const std::string& suffix) {
        return str.size() >= suffix.size() &&
            str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    bool openZipFile(FILE* cfgFile, const std::string& cfgFilePath, unzFile* zipfile) {
#if (_MSC_VER >= 1400) //visual studio 2005 or later
        const bool success = fopen_s(&cfgFile, cfgFilePath.c_str(), "r") == 0 && cfgFile;
#else
        cfgFile = fopen(cfgFilePath.c_str(), "r");
        const bool success = cfgFile != nullptr;
#endif
        if (!success) {
            sgct::MessageHandler::printError(
                "parseMpcdiConfiguration: Failed to open file %s", cfgFilePath.c_str()
            );
            return false;
        }
        // Open MPCDI file (zip compressed format)
        *zipfile = unzOpen(cfgFilePath.c_str());
        if (zipfile == nullptr) {
            sgct::MessageHandler::printError(
                "parseMpcdiConfiguration: Failed to open compressed mpcdi file %s",
                cfgFilePath.c_str()
            );
            return false;
        }
        return true;
    }

    void unsupportedFeatureCheck(std::string_view tag, const std::string& feature) {
        if (feature == tag) {
            std::string warn = "ReadConfigMpcdi: Unsupported feature: " + feature;
            sgct::MessageHandler::printWarning(warn.c_str());
        }
    }

    bool checkAttributeForExpectedValue(tinyxml2::XMLElement* elem,
                                        const std::string& attrRequired,
                                        const std::string& tagDescription,
                                        const std::string& expectedTag)
    {
        const char* attr = elem->Attribute(attrRequired.c_str());
        if (attr == nullptr) {
            sgct::MessageHandler::printError(
                ("parseMpcdiXml: No " + tagDescription + " attribute found").c_str()
            );
            return false;
        }

        if (expectedTag != attr) {
            std::string errorMsg = "parseMpcdiXml: Only " + tagDescription + " '" +
                expectedTag + "' is supported";
            sgct::MessageHandler::printError(errorMsg.c_str());
            return false;
        }

        return true;
    }
} // namespace

namespace sgct::core {

bool Mpcdi::parseConfiguration(const std::string& filenameMpcdi, Node& node,
                               Window& window)
{
    FILE* cfgFile = nullptr;
    unzFile zipfile;

    bool fileOpenSuccess = openZipFile(cfgFile, filenameMpcdi, &zipfile);
    if (!fileOpenSuccess) {
        MessageHandler::printError(
            "parseMpcdiConfiguration: Unable to open zip archive file %s",
            filenameMpcdi.c_str()
        );
        return false;
    }
    // Get info about the zip file
    unz_global_info globalInfo;
    int globalInfoRet = unzGetGlobalInfo(zipfile, &globalInfo);
    if (globalInfoRet != UNZ_OK) {
        MessageHandler::printError(
            "parseMpcdiConfiguration: Unable to get zip archive info from %s",
            filenameMpcdi.c_str()
        );
        unzClose(zipfile);
        return false;
    }

    // Search for required files inside mpcdi archive file
    for (unsigned int i = 0; i < globalInfo.number_entry; ++i) {
        unz_file_info fileInfo;
        constexpr const int MaxFilenameSize = 500;
        char fileName[MaxFilenameSize];
        int getCurrentFileInfo = unzGetCurrentFileInfo(
            zipfile,
            &fileInfo,
            fileName,
            MaxFilenameSize,
            nullptr,
            0,
            nullptr,
            0
        );
        if (getCurrentFileInfo != UNZ_OK) {
            MessageHandler::printError(
                "parseMpcdiConfiguration: Unable to get info on compressed file #%d", i
            );
            unzClose(zipfile);
            return false;
        }

        bool suc = processSubFile(_xmlFileContents, "xml", fileName, zipfile, fileInfo);
        if (!suc) {
            suc = processSubFile(_pfmFileContents, "pfm", fileName, zipfile, fileInfo);
        }
        if (!suc) {
            unzClose(zipfile);
            return false;
        }
        if ((i + 1) < globalInfo.number_entry) {
            int goToNextFileStatus = unzGoToNextFile(zipfile);
            if (goToNextFileStatus != UNZ_OK) {
                MessageHandler::printWarning(
                    "parseMpcdiConfiguration: Unable to get next file in archive"
                );
            }
        }
    }
    unzClose(zipfile);
    if (!_xmlFileContents.isFound && !_pfmFileContents.isFound) {
        MessageHandler::printError(
            "parseMpcdiConfiguration: file %s does not contain xml and/or pfm file",
            filenameMpcdi.c_str()
        );
        return false;
    }

    const bool parseSuccess = readAndParseString(node, window);
    return parseSuccess;
}

bool Mpcdi::processSubFile(SubFile& sf, const std::string& suffix,
                               const std::string& filename, unzFile zipfile,
                               const unz_file_info& fileInfo)
{
    if (sf.isFound || !doesStringHaveSuffix(filename, suffix)) {
        return true;
    }

    sf.isFound = true;
    sf.fileName = filename;
    int openCurrentFile = unzOpenCurrentFile(zipfile);
    if (openCurrentFile != UNZ_OK) {
        MessageHandler::printError(
            "parseMpcdiConfiguration: Unable to open %s", filename.c_str()
        );
        unzClose(zipfile);
        return false;
    }
    sf.buffer.resize(fileInfo.uncompressed_size);
    int err = unzReadCurrentFile(
        zipfile,
        sf.buffer.data(),
        static_cast<unsigned int>(sf.buffer.size())
    );
    if (err < 0) {
        MessageHandler::printError(
            "parseMpcdiConfiguration: %s read from %s failed",
            suffix.c_str(), filename.c_str()
        );
        unzClose(zipfile);
        return false;
    }
    return true;
}

bool Mpcdi::readAndParseString(Node& node, Window& win) {
    if (_xmlFileContents.buffer.empty()) {
        return false;
    }
    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError result = xmlDoc.Parse(
        _xmlFileContents.buffer.data(),
        _xmlFileContents.buffer.size()
    );

    if (result != tinyxml2::XML_NO_ERROR) {
        std::string str = "Parsing failed after: ";
        if (xmlDoc.GetErrorStr1() && xmlDoc.GetErrorStr2()) {
            str += xmlDoc.GetErrorStr1();
            str += ' ';
            str += xmlDoc.GetErrorStr2();
        }
        else if (xmlDoc.GetErrorStr1()) {
            str += xmlDoc.GetErrorStr1();
        }
        else if (xmlDoc.GetErrorStr2()) {
            str += xmlDoc.GetErrorStr2();
        }
        else {
            str = "File not found";
        }

        MessageHandler::printError(
            "readAndParseXMLString: error parsing file %s", str.c_str()
        );
        return false;
    }
    else {
        const bool success = readAndParseMpcdi(xmlDoc, node, win);
        return success;
    }
}

bool Mpcdi::readAndParseMpcdi(tinyxml2::XMLDocument& xmlDoc, Node& node, Window& win) {
    tinyxml2::XMLElement* XMLroot = xmlDoc.FirstChildElement("MPCDI");
    if (XMLroot == nullptr) {
        MessageHandler::printError("readAndParseMpcdi: Cannot find XML root");
        return false;
    }

    const bool hasProfile = checkAttributeForExpectedValue(
        XMLroot,
        "profile",
        "MPCDI profile",
        "3d"
    );
    if (!hasProfile) {
        MessageHandler::printError(
            "readAndParseMpcdi: Problem with 'MPCDI' attribute in XML"
        );
        return false;
    }
    const bool hasGeometry = checkAttributeForExpectedValue(
        XMLroot,
        "geometry",
        "MPCDI geometry level",
        "1"
    );
    if (!hasGeometry) {
        MessageHandler::printError(
            "readAndParseMpcdi: Problem with 'geometry' attribute in XML"
        );
        return false;
    }
    const bool hasVersion = checkAttributeForExpectedValue(
        XMLroot,
        "version",
        "MPCDI version",
        "2.0"
    );
    if (!hasVersion) {
        MessageHandler::printError(
            "readAndParseMpcdi: Problem with 'version' attribute in XML"
        );
        return false;
    }

    MpcdiFoundItems parsedItems;
    tinyxml2::XMLElement* element = XMLroot->FirstChildElement();
    while (element) {
        std::string_view val = element->Value();
        if (val == "display") {
            const bool success = readAndParseDisplay(element, node, win, parsedItems);
            if (!success) {
                return false;
            }
        }
        else if (val == "files") {
            const bool success = readAndParseFiles(element, win);
            if (!success) {
                return false;
            }
        }
        unsupportedFeatureCheck(val, "extensionSet");
        element = element->NextSiblingElement();
    }

    return true;
}

bool Mpcdi::readAndParseDisplay(tinyxml2::XMLElement* element, Node& node, Window& win,
                                MpcdiFoundItems& parsedItems)
{
    if (parsedItems.hasDisplayElem) {
        MessageHandler::printError(
            "parseMpcdiXml: Multiple 'display' elements not supported"
        );
        return false;
    }
    else {
        parsedItems.hasDisplayElem = true;
    }
    tinyxml2::XMLElement* child = element->FirstChildElement();
    while (child) {
        std::string_view val = child->Value();
        if (val == "buffer") {
            const bool success = readAndParseBuffer(child, win, parsedItems);
            if (!success) {
                return false;
            }
            node.addWindow(std::move(win));
        }
        child = child->NextSiblingElement();
    }
    return true;
}

bool Mpcdi::readAndParseFiles(tinyxml2::XMLElement* element, sgct::Window& win) {
    std::string fileRegion;

    tinyxml2::XMLElement* child = element->FirstChildElement();
    while (child) {
        std::string_view val = child->Value();
        if (val == "fileset") {
            child = child->NextSiblingElement();
            continue;
        }

        if (child->Attribute("region")) {
            fileRegion = child->Attribute("region");
        }
        tinyxml2::XMLElement* grandChild = child->FirstChildElement();
        while (grandChild) {
            std::string_view grandChildVal = grandChild->Value();
            if (grandChildVal == "geometryWarpFile") {
                bool success = readAndParseGeoWarpFile(grandChild, win, fileRegion);
                if (!success) {
                    return false;
                }
            }
            unsupportedFeatureCheck(grandChildVal, "alphaMap");
            unsupportedFeatureCheck(grandChildVal, "betaMap");
            unsupportedFeatureCheck(grandChildVal, "distortionMap");
            unsupportedFeatureCheck(grandChildVal, "decodeLUT");
            unsupportedFeatureCheck(grandChildVal, "correctLUT");
            unsupportedFeatureCheck(grandChildVal, "encodeLUT");

            grandChild = grandChild->NextSiblingElement();
        }
        child = child->NextSiblingElement();
    }
    return true;
}

bool Mpcdi::readAndParseGeoWarpFile(tinyxml2::XMLElement* element, Window& win,
                                    std::string filesetRegionId)
{
    std::unique_ptr<MpcdiWarp> warp = std::make_unique<MpcdiWarp>();
    warp->id = filesetRegionId;
    tinyxml2::XMLElement* child = element->FirstChildElement();
    while (child) {
        std::string_view val = child->Value();
        if (val == "path") {
            warp->pathWarpFile = child->GetText();
            warp->hasFoundPath = true;
        }
        else if (val == "interpolation") {
            std::string_view interpolation = child->GetText();
            if (interpolation != "linear") {
                MessageHandler::printWarning(
                    "parseMpcdiXml: only linear interpolation is supported"
                );
            }
            warp->hasFoundInterpolation = true;
        }
        child = child->NextSiblingElement();
    }
    if (warp->hasFoundPath && warp->hasFoundInterpolation) {
        // Look for matching MPCDI region (SGCT viewport) to pass
        // the warp field data to
        bool foundMatchingPfmBuffer = false;
        for (int i = 0; i < win.getNumberOfViewports(); ++i) {
            // @TODO (abock, 2019-10-13); This is the only place where we need the name of
            // a viewport. Is there a better way to determine whether we are currently
            // looking at the correct viewport so we can remove the name?
            const std::string& winName = win.getViewport(i).getName();
            const std::string& warpName = warp->id;
            if (winName == warpName && warp->pathWarpFile == _pfmFileContents.fileName) {
                std::vector<unsigned char> meshData(
                    _pfmFileContents.buffer.begin(),
                    _pfmFileContents.buffer.end()
                );
                win.getViewport(i).setMpcdiWarpMesh(std::move(meshData));
                foundMatchingPfmBuffer = true;
            }
        }
        if (!foundMatchingPfmBuffer) {
            MessageHandler::printError(
                "parseMpcdiXml: matching geometryWarpFile not found"
            );
            return false;
        }
    }
    else {
        MessageHandler::printError(
            "parseMpcdiXml: geometryWarpFile requires both path and interpolation"
        );
        return false;
    }

    _warp.push_back(std::move(warp));
    return true;
}

bool Mpcdi::readAndParseBuffer(tinyxml2::XMLElement* element, Window& win,
                               MpcdiFoundItems& parsedItems)
{
    if (parsedItems.hasBufferElem) {
        MessageHandler::printError(
            "parseMpcdiXml: Multiple 'buffer' elements unsupported"
        );
        return false;
    }
    else {
        parsedItems.hasBufferElem = true;
    }
    if (element->Attribute("xResolution")) {
        element->QueryAttribute("xResolution", &parsedItems.resolution.x);
    }
    if (element->Attribute("yResolution")) {
        element->QueryAttribute("yResolution", &parsedItems.resolution.y);
    }
    if (parsedItems.resolution.x >= 0 && parsedItems.resolution.y >= 0) {
        win.initWindowResolution(parsedItems.resolution);
        win.setFramebufferResolution(parsedItems.resolution);
        win.setFixResolution(true);
    }
    else {
        MessageHandler::printError(
            "parseMpcdiXml: Require both xResolution and yResolution values"
        );
        return false;
    }
    // Assume a 0,0 offset for an MPCDI buffer, which maps to an SGCT window
    win.setWindowPosition(glm::ivec2(0, 0));

    tinyxml2::XMLElement* child = element->FirstChildElement();
    while (child) {
        std::string_view val = child->Value();
        if (val == "region") {
            if (!readAndParseRegion(child, win, parsedItems)) {
                return false;
            }
        }
        unsupportedFeatureCheck(val, "coordinateFrame");
        unsupportedFeatureCheck(val, "color");
        child = child->NextSiblingElement();
    }
    return true;
}

bool Mpcdi::readAndParseRegion(tinyxml2::XMLElement* element, Window& win,
                               MpcdiFoundItems&)
{
    // Require an 'id' attribute for each region. These will be compared later to the
    // fileset, in which there must be a matching 'id'. The _bufferRegions vector is
    // intended for use with MPCDI files containing multiple regions, but currently
    // only is tested with single region files.
    if (element->Attribute("id")) {
        _bufferRegions.push_back(element->Attribute("id"));
    }
    else {
        MessageHandler::printError(
            "parseMpcdiXml: No 'id' attribute provided for region"
        );
        return false;
    }
    std::unique_ptr<Viewport> vp = std::make_unique<Viewport>();
    sgct::config::MpcdiProjection proj = parseMpcdi(element);
    vp->applySettings(proj);
    win.addViewport(std::move(vp));
    return true;
}

} // namespace sgct::core
