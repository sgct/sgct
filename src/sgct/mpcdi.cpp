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
#include <stdexcept>
#include <tinyxml2.h>
#include "unzip.h"
#include <zip.h>

namespace sgct::core::mpcdi {

namespace {
    class ParsingError : public std::runtime_error {
    public:
        ParsingError(const std::string& msg) : std::runtime_error("[MPCDI] " + msg) {}
    };

    struct SubFile {
        bool isFound = false;
        std::string fileName;
        std::vector<char> buffer;
    };

    [[nodiscard]] glm::quat parseOrientationNode(float yaw, float pitch, float roll) {
        glm::quat quat;
        quat = glm::rotate(quat, glm::radians(-yaw), glm::vec3(0.f, 1.f, 0.f));
        quat = glm::rotate(quat, glm::radians(pitch), glm::vec3(1.f, 0.f, 0.f));
        quat = glm::rotate(quat, glm::radians(-roll), glm::vec3(0.f, 0.f, 1.f));
        return quat;
    }

    [[nodiscard]] config::MpcdiProjection parseMpcdi(const tinyxml2::XMLElement& elem) {
        config::MpcdiProjection proj;
        if (elem.Attribute("id")) {
            proj.id = elem.Attribute("id");
        }

        glm::vec2 vpPosition;
        if (elem.QueryFloatAttribute("x", &vpPosition[0]) != tinyxml2::XML_NO_ERROR ||
            elem.QueryFloatAttribute("y", &vpPosition[1]) != tinyxml2::XML_NO_ERROR)
        {
            throw ParsingError("Failed to parse position from XML");
        }
        proj.position = vpPosition;

        glm::vec2 vpSize;
        if (elem.QueryFloatAttribute("xSize", &vpSize[0]) != tinyxml2::XML_NO_ERROR ||
            elem.QueryFloatAttribute("ySize", &vpSize[1]) != tinyxml2::XML_NO_ERROR)
        {
            throw ParsingError("Failed to parse size from XML");
        }
        proj.size = vpSize;

        float yaw = 0.f;
        float pitch = 0.f;
        float roll = 0.f;
        const tinyxml2::XMLElement* child = elem.FirstChildElement();
        while (child) {
            const tinyxml2::XMLElement& c = *child;

            std::string_view val = c.Value();
            if (val == "frustum") {
                bool hasRight = false;
                bool hasLeft = false;
                bool hasUp = false;
                bool hasDown = false;
                bool hasYaw = false;
                bool hasPitch = false;
                bool hasRoll = false;
                config::MpcdiProjection::Frustum frustum;
                const tinyxml2::XMLElement* grandChild = c.FirstChildElement();
                while (grandChild) {
                    const tinyxml2::XMLElement& gc = *grandChild;
                    try {
                        std::string_view grandChildVal = gc.Value();
                        if (grandChildVal == "rightAngle") {
                            frustum.right = std::stof(gc.GetText());
                            hasRight = true;
                        }
                        else if (grandChildVal == "leftAngle") {
                            frustum.left = std::stof(gc.GetText());
                            hasLeft = true;
                        }
                        else if (grandChildVal == "upAngle") {
                            frustum.up = std::stof(gc.GetText());
                            hasUp = true;
                        }
                        else if (grandChildVal == "downAngle") {
                            frustum.down = std::stof(gc.GetText());
                            hasDown = true;
                        }
                        else if (grandChildVal == "yaw") {
                            yaw = std::stof(gc.GetText());
                            hasYaw = true;
                        }
                        else if (grandChildVal == "pitch") {
                            pitch = std::stof(gc.GetText());
                            hasPitch = true;
                        }
                        else if (grandChildVal == "roll") {
                            roll = std::stof(gc.GetText());
                            hasRoll = true;
                        }
                    }
                    catch (const std::invalid_argument&) {
                        throw ParsingError("Failed to parse frustum element");
                    }

                    grandChild = gc.NextSiblingElement();
                }

                const bool hasMissingField = !hasDown || !hasUp || !hasLeft ||
                    !hasRight || !hasYaw || !hasPitch || !hasRoll;
                if (hasMissingField) {
                    throw ParsingError("Failed to parse MPCDI projection FOV");
                }

                proj.frustum = frustum;
                proj.orientation = parseOrientationNode(yaw, pitch, roll);
            }
            child = c.NextSiblingElement();
        }

        return proj;
    }

    void unsupportedFeatureCheck(std::string_view tag, const std::string& feature) {
        if (feature == tag) {
            MessageHandler::printWarning(("Unsupported feature: " + feature).c_str());
        }
    }

    void parseBuffer(const tinyxml2::XMLElement& elem, ReturnValue& res) {
        glm::ivec2 resolution = glm::ivec2(-1);
        if (elem.Attribute("xResolution")) {
            elem.QueryAttribute("xResolution", &resolution.x);
        }
        if (elem.Attribute("yResolution")) {
            elem.QueryAttribute("yResolution", &resolution.y);
        }
        if (resolution.x <= 0 && resolution.y <= 0) {
            throw ParsingError("Require both xResolution and yResolution values");
        }
        res.resolution = resolution;

        // Assume a 0,0 offset for an MPCDI buffer, which maps to an SGCT window
        res.position = glm::ivec2(0, 0);

        const tinyxml2::XMLElement* child = elem.FirstChildElement();
        while (child) {
            std::string_view val = child->Value();
            if (val == "region") {
                // Require an 'id' attribute for each region. These will be compared later
                // to the fileset, in which there must be a matching 'id'.
                if (!child->Attribute("id")) {
                    throw ParsingError("No 'id' attribute provided for region");
                }

                ReturnValue::ViewportInfo v;
                v.proj = parseMpcdi(*child);
                res.viewports.push_back(v);
            }
            unsupportedFeatureCheck(val, "coordinateFrame");
            unsupportedFeatureCheck(val, "color");
            child = child->NextSiblingElement();
        }
    }

    void handleSubfile(SubFile& subfile, unzFile zip, const unz_file_info& fileInfo) {
        if (subfile.isFound) {
            MessageHandler::printWarning(
                "Duplicate file %s found in MPCDI", subfile.fileName.c_str()
            );
            return;
        }

        const int openCurrentFile = unzOpenCurrentFile(zip);
        if (openCurrentFile != UNZ_OK) {
            throw ParsingError("Unable to open " + subfile.fileName);
        }
        std::vector<char> buffer(fileInfo.uncompressed_size);
        int size = unzReadCurrentFile(zip, buffer.data(), fileInfo.uncompressed_size);
        if (size < 0) {
            throw ParsingError("Read from " + subfile.fileName + " failed");
        }
        subfile.isFound = true;
        subfile.buffer = std::move(buffer);
    }

    void parseDisplay(const tinyxml2::XMLElement& element, ReturnValue& res) {
        bool foundBufferElem = false;
        const tinyxml2::XMLElement* child = element.FirstChildElement();
        while (child) {
            std::string_view val = child->Value();
            if (val == "buffer") {
                if (foundBufferElem) {
                    throw ParsingError("Multiple 'buffer' elements not supported");
                }
                parseBuffer(*child, res);
                foundBufferElem = true;
            }
            child = child->NextSiblingElement();
        }
    }

    void parseGeoWarpFile(const tinyxml2::XMLElement& element, const std::string& region,
                          const SubFile& pfmFile, ReturnValue& res)
    {
        std::string pathWarpFile;
        bool hasFoundPath = false;
        bool hasFoundInterpolation = false;

        const tinyxml2::XMLElement* child = element.FirstChildElement();
        while (child) {
            std::string_view val = child->Value();
            if (val == "path") {
                pathWarpFile = child->GetText();
                hasFoundPath = true;
            }
            else if (val == "interpolation") {
                std::string_view interpolation = child->GetText();
                if (interpolation != "linear") {
                    throw ParsingError("Only linear interpolation is supported");
                }
                hasFoundInterpolation = true;
            }
            child = child->NextSiblingElement();
        }
        if (!hasFoundPath || !hasFoundInterpolation) {
            throw ParsingError("GeometryWarpFile requires path and interpolation");
        }

        // Look for matching MPCDI region (SGCT viewport) to pass
        // the warp field data to
        bool foundMatchingPfmBuffer = false;
        for (ReturnValue::ViewportInfo& vpInfo : res.viewports) {
            const std::string& winName = *vpInfo.proj.id;
            if (winName == region && pathWarpFile == pfmFile.fileName) {
                vpInfo.meshData = pfmFile.buffer;
                foundMatchingPfmBuffer = true;
            }
        }
        if (!foundMatchingPfmBuffer) {
            throw ParsingError("Matching geometryWarpFile not found");
        }
    }

    void parseFiles(const tinyxml2::XMLElement& elem, const SubFile& pfmFile,
                    ReturnValue& res)
    {
        std::string fileRegion;

        const tinyxml2::XMLElement* child = elem.FirstChildElement();
        while (child) {
            std::string_view val = child->Value();
            if (val != "fileset") {
                child = child->NextSiblingElement();
                continue;
            }

            if (child->Attribute("region")) {
                fileRegion = child->Attribute("region");
            }
            const tinyxml2::XMLElement* grandChild = child->FirstChildElement();
            while (grandChild) {
                std::string_view grandChildVal = grandChild->Value();
                if (grandChildVal == "geometryWarpFile") {
                    parseGeoWarpFile(*grandChild, fileRegion, pfmFile, res);
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
    }

    void parseMpcdi(const tinyxml2::XMLDocument& doc, const SubFile& pfmFile,
                    ReturnValue& res) {
        const tinyxml2::XMLElement* rootNode = doc.FirstChildElement("MPCDI");
        if (rootNode == nullptr) {
            throw ParsingError("Cannot find XML root");
        }
        const tinyxml2::XMLElement& root = *rootNode;

        // Verify that we have a correctly formed MPCDI file
        const char* profileAttr = root.Attribute("profile");
        if (profileAttr == nullptr || std::string(profileAttr) != "3d") {
            throw ParsingError("Error parsing MPCDI, missing or wrong 'profile'");
        }
        const char* geometryAttr = root.Attribute("geometry");
        if (geometryAttr == nullptr || std::string(geometryAttr) != "1") {
            throw ParsingError("Error parsing MPCDI, missing or wrong 'geometry'");
        }
        const char* versionAttr = root.Attribute("version");
        if (versionAttr == nullptr || std::string(versionAttr) != "2.0") {
            throw ParsingError("Error parsing MPCDI, missing or wrong 'version'");
        }

        bool foundDisplayElem = false;
        const tinyxml2::XMLElement* element = root.FirstChildElement();
        while (element) {
            std::string_view val = element->Value();
            if (val == "display") {
                if (foundDisplayElem) {
                    throw ParsingError("Multiple 'display' elements not supported");
                }
                parseDisplay(*element, res);
                foundDisplayElem = true;
            }
            else if (val == "files") {
                parseFiles(*element, pfmFile, res);
            }
            unsupportedFeatureCheck(val, "extensionSet");
            element = element->NextSiblingElement();
        }
    }

    void parseString(const SubFile& xmlFile, const SubFile& pfmFile, ReturnValue& res) {
        if (xmlFile.buffer.empty()) {
            throw ParsingError("Empty XML buffer");
        }
        tinyxml2::XMLDocument xmlDoc;
        tinyxml2::XMLError result = xmlDoc.Parse(
            xmlFile.buffer.data(),
            xmlFile.buffer.size()
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
            throw ParsingError("Error parsing file " + str);
        }

        parseMpcdi(xmlDoc, pfmFile, res);
    }
} // namespace

ReturnValue parseConfiguration(const std::string& filenameMpcdi) {
    ReturnValue res;

    unzFile zipfile = unzOpen(filenameMpcdi.c_str());
    if (zipfile == nullptr) {
        throw ParsingError("Unable to open zip archive file " + filenameMpcdi);
    }
    // Get info about the zip file
    unz_global_info globalInfo;
    int globalInfoRet = unzGetGlobalInfo(zipfile, &globalInfo);
    if (globalInfoRet != UNZ_OK) {
        unzClose(zipfile);
        throw ParsingError("Unable to get zip archive info from " + filenameMpcdi);
    }

    // Search for required files inside mpcdi archive file
    SubFile xmlFileContents;
    SubFile pfmFileContents;
    try {
        for (unsigned int i = 0; i < globalInfo.number_entry; ++i) {
            unz_file_info fileInfo;
            constexpr const int MaxFilenameSize = 500;
            char fileName[MaxFilenameSize];
            int result = unzGetCurrentFileInfo(
                zipfile,
                &fileInfo,
                fileName,
                MaxFilenameSize,
                nullptr,
                0,
                nullptr,
                0
            );
            if (result != UNZ_OK) {
                throw ParsingError("Unable to get info on file " + std::to_string(i));
            }

            constexpr auto endsWith = [](const std::string& str, const std::string& ext) {
                const size_t s = str.size();
                const size_t e = ext.size();
                return s >= e && str.compare(s - e, e, ext) == 0;
            };

            if (endsWith(fileName, "xml")) {
                xmlFileContents.fileName = fileName;
                handleSubfile(xmlFileContents, zipfile, fileInfo);
            }
            else if (endsWith(fileName, "pfm")) {
                pfmFileContents.fileName = fileName;
                handleSubfile(pfmFileContents, zipfile, fileInfo);
            }
            else {
                MessageHandler::printWarning("Ignoring unknown extension %s", fileName);
            }

            if (i < globalInfo.number_entry - 1) {
                int goToNextFileStatus = unzGoToNextFile(zipfile);
                if (goToNextFileStatus != UNZ_OK) {
                    MessageHandler::printWarning(
                        "parseMpcdiConfiguration: Unable to get next file in archive"
                    );
                }
            }
        }
    }
    catch (...) {
        unzClose(zipfile);
        throw;
    }

    unzClose(zipfile);
    if (!xmlFileContents.isFound && !pfmFileContents.isFound) {
        throw ParsingError(filenameMpcdi + " does not contain xml and/or pfm file");
    }

    parseString(xmlFileContents, pfmFileContents, res);
    return res;
}

} // namespace sgct::core::mpcdi
