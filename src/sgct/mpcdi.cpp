/*
 * Mpcdi.cpp
 *
 *  Created on: Jul 3, 2017
 *      Author: Gene Payne
 */

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
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Viewport: Failed to parse position from XML\n"
            );
        }

        glm::vec2 vpSize;
        if (element->QueryFloatAttribute("xSize", &vpSize[0]) == XML_NO_ERROR &&
            element->QueryFloatAttribute("ySize", &vpSize[1]) == XML_NO_ERROR)
        {
            proj.size = vpSize;
        }
        else {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Viewport: Failed to parse size from XML\n"
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
        //        MessageHandler::instance()->print(
        //            MessageHandler::Level::Warning,
        //            "Viewport: MPCDI region expected resolution does not match window\n"
        //        );
        //    }

        //}
        //else {
        //    MessageHandler::instance()->print(
        //        MessageHandler::Level::Error,
        //        "Viewport: Failed to parse resolution from XML\n"
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
                        sgct::MessageHandler::instance()->print(
                            sgct::MessageHandler::Level::Error,
                            "Viewport: Failed to parse frustum element from MPCDI XML\n"
                        );
                    }

                    grandChild = grandChild->NextSiblingElement();
                }


                const bool hasMissingField = !hasDown || !hasUp || !hasLeft || !hasRight ||
                    !hasYaw || !hasPitch || !hasRoll;

                if (hasMissingField) {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "Viewport: Failed to parse mpcdi projection FOV from XML\n"
                    );
                    return {};
                }

                proj.orientation = sgct::core::readconfig::parseMpcdiOrientationNode(yaw, pitch, roll);
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
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "parseMpcdiConfiguration: Failed to open file %s\n", cfgFilePath.c_str()
            );
            return false;
        }
        // Open MPCDI file (zip compressed format)
        *zipfile = unzOpen(cfgFilePath.c_str());
        if (zipfile == nullptr) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "parseMpcdiConfiguration: Failed to open compressed mpcdi file %s\n",
                cfgFilePath.c_str()
            );
            return false;
        }
        return true;
    }

    void unsupportedFeatureCheck(std::string_view tag, const std::string& feature) {
        if (feature == tag) {
            std::string warn = "ReadConfigMpcdi: Unsupported feature: " + feature + " \n";
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Warning,
                warn.c_str()
            );
        }
    }

    bool checkAttributeForExpectedValue(tinyxml2::XMLElement* elem,
                                        const std::string& attrRequired,
                                        const std::string& tagDescription,
                                        const std::string& expectedTag)
    {
        const char* attr = elem->Attribute(attrRequired.c_str());
        if (attr == nullptr) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                ("parseMpcdiXml: No " + tagDescription + " attribute found\n").c_str()
            );
            return false;
        }

        if (expectedTag != attr) {
            std::string errorMsg = "parseMpcdiXml: Only " + tagDescription + " '" +
                expectedTag + "' is supported.\n";
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                errorMsg.c_str()
            );
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
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "parseMpcdiConfiguration: Unable to open zip archive file %s\n",
            filenameMpcdi.c_str()
        );
        return false;
    }
    // Get info about the zip file
    unz_global_info globalInfo;
    int globalInfoRet = unzGetGlobalInfo(zipfile, &globalInfo);
    if (globalInfoRet != UNZ_OK) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "parseMpcdiConfiguration: Unable to get zip archive info from %s\n",
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
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "parseMpcdiConfiguration: Unable to get info on compressed file #%d\n", i
            );
            unzClose(zipfile);
            return false;
        }

        bool suc = processSubFile(mXmlFileContents, "xml", fileName, zipfile, fileInfo);
        if (!suc) {
            suc = processSubFile(mPfmFileContents, "pfm", fileName, zipfile, fileInfo);
        }
        if (!suc) {
            unzClose(zipfile);
            return false;
        }
        if ((i + 1) < globalInfo.number_entry) {
            int goToNextFileStatus = unzGoToNextFile(zipfile);
            if (goToNextFileStatus != UNZ_OK) {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Warning,
                    "parseMpcdiConfiguration: Unable to get next file in archive\n"
                );
            }
        }
    }
    unzClose(zipfile);
    if (!mXmlFileContents.isFound && !mPfmFileContents.isFound) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "parseMpcdiConfiguration: file %s does not contain xml and/or pfm file\n",
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
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "parseMpcdiConfiguration: Unable to open %s\n", filename.c_str()
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
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "parseMpcdiConfiguration: %s read from %s failed\n",
            suffix.c_str(), filename.c_str()
        );
        unzClose(zipfile);
        return false;
    }
    return true;
}

bool Mpcdi::readAndParseString(Node& node, Window& win) {
    if (mXmlFileContents.buffer.empty()) {
        return false;
    }
    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError result = xmlDoc.Parse(
        mXmlFileContents.buffer.data(),
        mXmlFileContents.buffer.size()
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

        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "readAndParseXMLString: error parsing file %s\n", str.c_str()
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
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "readAndParseMpcdi: Cannot find XML root"
        );
        return false;
    }

    const bool hasProfile = checkAttributeForExpectedValue(
        XMLroot,
        "profile",
        "MPCDI profile",
        "3d"
    );
    if (!hasProfile) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "readAndParseMpcdi: Problem with 'MPCDI' attribute in XML\n"
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
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "readAndParseMpcdi: Problem with 'geometry' attribute in XML\n"
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
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "readAndParseMpcdi: Problem with 'version' attribute in XML\n"
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
    if (parsedItems.haveDisplayElem) {
        MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "parseMpcdiXml: Multiple 'display' elements not supported\n"
        );
        return false;
    }
    else {
        parsedItems.haveDisplayElem = true;
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
            warp->haveFoundPath = true;
        }
        else if (val == "interpolation") {
            std::string_view interpolation = child->GetText();
            if (interpolation != "linear") {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Warning,
                    "parseMpcdiXml: only linear interpolation is supported\n"
                );
            }
            warp->haveFoundInterpolation = true;
        }
        child = child->NextSiblingElement();
    }
    if (warp->haveFoundPath && warp->haveFoundInterpolation) {
        // Look for matching MPCDI region (SGCT viewport) to pass
        // the warp field data to
        bool foundMatchingPfmBuffer = false;
        for (int i = 0; i < win.getNumberOfViewports(); ++i) {
            const std::string& winName = win.getViewport(i).getName();
            const std::string& warpName = warp->id;
            if (winName == warpName && warp->pathWarpFile == mPfmFileContents.fileName) {
                std::vector<unsigned char> meshData(
                    mPfmFileContents.buffer.begin(),
                    mPfmFileContents.buffer.end()
                );
                win.getViewport(i).setMpcdiWarpMesh(std::move(meshData));
                foundMatchingPfmBuffer = true;
            }
        }
        if (!foundMatchingPfmBuffer) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "parseMpcdiXml: matching geometryWarpFile not found\n"
            );
            return false;
        }
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "parseMpcdiXml: geometryWarpFile requires both path and interpolation\n"
        );
        return false;
    }

    mWarp.push_back(std::move(warp));
    return true;
}

bool Mpcdi::readAndParseBuffer(tinyxml2::XMLElement* element, Window& win,
                               MpcdiFoundItems& parsedItems)
{
    if (parsedItems.haveBufferElem) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "parseMpcdiXml: Multiple 'buffer' elements unsupported\n"
        );
        return false;
    }
    else {
        parsedItems.haveBufferElem = true;
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
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "parseMpcdiXml: Require both xResolution and yResolution values\n"
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
                               MpcdiFoundItems& parsedItems)
{
    // Require an 'id' attribute for each region. These will be compared later to the
    // fileset, in which there must be a matching 'id'. The mBufferRegions vector is
    // intended for use with MPCDI files containing multiple regions, but currently
    // only is tested with single region files.
    if (element->Attribute("id")) {
        mBufferRegions.push_back(element->Attribute("id"));
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "parseMpcdiXml: No 'id' attribute provided for region\n"
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
