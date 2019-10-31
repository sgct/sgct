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

        const tinyxml2::XMLElement* child = elem.FirstChildElement();
        while (child) {
            const tinyxml2::XMLElement& c = *child;

            std::string_view val = c.Value();
            if (val == "frustum") {
                std::optional<float> right;
                std::optional<float> left;
                std::optional<float> up;
                std::optional<float> down;
                std::optional<float> yaw;
                std::optional<float> pitch;
                std::optional<float> roll;
                const tinyxml2::XMLElement* grandChild = c.FirstChildElement();
                while (grandChild) {
                    const tinyxml2::XMLElement& gc = *grandChild;
                    try {
                        std::string_view grandChildVal = gc.Value();
                        if (grandChildVal == "rightAngle") {
                            right = std::stof(gc.GetText());
                        }
                        else if (grandChildVal == "leftAngle") {
                            left = std::stof(gc.GetText());
                        }
                        else if (grandChildVal == "upAngle") {
                            up = std::stof(gc.GetText());
                        }
                        else if (grandChildVal == "downAngle") {
                            down = std::stof(gc.GetText());
                        }
                        else if (grandChildVal == "yaw") {
                            yaw = std::stof(gc.GetText());
                        }
                        else if (grandChildVal == "pitch") {
                            pitch = std::stof(gc.GetText());
                        }
                        else if (grandChildVal == "roll") {
                            roll = std::stof(gc.GetText());
                        }
                    }
                    catch (const std::invalid_argument&) {
                        throw ParsingError("Failed to parse frustum element");
                    }

                    grandChild = gc.NextSiblingElement();
                }

                const bool hasAllFields = down.has_value() && up.has_value() &&
                    left.has_value() && right.has_value() && yaw.has_value() &&
                    pitch.has_value() && roll.has_value();
                if (!hasAllFields) {
                    throw ParsingError("Failed to parse MPCDI projection FOV");
                }
                
                proj.frustum = { *down, *up, *left, *right };
                glm::quat quat;
                quat = glm::rotate(quat, glm::radians(-*yaw), glm::vec3(0.f, 1.f, 0.f));
                quat = glm::rotate(quat, glm::radians(*pitch), glm::vec3(1.f, 0.f, 0.f));
                quat = glm::rotate(quat, glm::radians(-*roll), glm::vec3(0.f, 0.f, 1.f));
                proj.orientation = quat;
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

    void handleSubfile(SubFile& subfile, unzFile zip, unsigned long uncompressedSize) {
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
        std::vector<char> buffer(uncompressedSize);
        const int size = unzReadCurrentFile(zip, buffer.data(), uncompressedSize);
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
        std::optional<std::string> pathWarpFile;

        const tinyxml2::XMLElement* child = element.FirstChildElement();
        while (child) {
            std::string_view val = child->Value();
            if (val == "path") {
                pathWarpFile = child->GetText();
            }
            else if (val == "interpolation") {
                std::string_view interpolation = child->GetText();
                if (interpolation != "linear") {
                    throw ParsingError("Only linear interpolation is supported");
                }
            }
            child = child->NextSiblingElement();
        }
        if (!pathWarpFile.has_value()) {
            throw ParsingError("GeometryWarpFile requires path and interpolation");
        }

        // Look for matching MPCDI region (SGCT viewport) to pass the warp field data to
        bool foundMatchingPfmBuffer = false;
        for (ReturnValue::ViewportInfo& vpInfo : res.viewports) {
            const std::string& viewportId = *vpInfo.proj.id;
            if (viewportId == region && *pathWarpFile == pfmFile.fileName) {
                vpInfo.meshData = pfmFile.buffer;
                foundMatchingPfmBuffer = true;
            }
        }
        if (!foundMatchingPfmBuffer) {
            throw ParsingError("Matching geometryWarpFile not found");
        }
    }

    void parseFiles(const tinyxml2::XMLElement& elem, const SubFile& pfm,
                    ReturnValue& res)
    {
        std::string fileRegion;

        const tinyxml2::XMLElement* child = elem.FirstChildElement("fileset");
        while (child) {
            std::string_view val = child->Value();
            if (child->Attribute("region")) {
                fileRegion = child->Attribute("region");
            }
            const tinyxml2::XMLElement* grandChild = child->FirstChildElement();
            while (grandChild) {
                std::string_view grandChildVal = grandChild->Value();
                if (grandChildVal == "geometryWarpFile") {
                    parseGeoWarpFile(*grandChild, fileRegion, pfm, res);
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

    ReturnValue parseMpcdi(const tinyxml2::XMLDocument& doc, const SubFile& pfmFile) {
        const tinyxml2::XMLElement* rootNode = doc.FirstChildElement("MPCDI");
        if (rootNode == nullptr) {
            throw ParsingError("Cannot find XML root");
        }
        const tinyxml2::XMLElement& root = *rootNode;

        // Verify that we have a correctly formed MPCDI file
        const char* profileAttr = root.Attribute("profile");
        if (profileAttr == nullptr || std::string_view(profileAttr) != "3d") {
            throw ParsingError("Error parsing MPCDI, missing or wrong 'profile'");
        }
        const char* geometryAttr = root.Attribute("geometry");
        if (geometryAttr == nullptr || std::string_view(geometryAttr) != "1") {
            throw ParsingError("Error parsing MPCDI, missing or wrong 'geometry'");
        }
        const char* versionAttr = root.Attribute("version");
        if (versionAttr == nullptr || std::string_view(versionAttr) != "2.0") {
            throw ParsingError("Error parsing MPCDI, missing or wrong 'version'");
        }

        ReturnValue res;
        // Parse the display subtree
        const tinyxml2::XMLElement* displayElement = root.FirstChildElement("display");
        if (displayElement == nullptr) {
            throw ParsingError("Missing 'display' element");
        }
        if (displayElement->NextSiblingElement("display") != nullptr) {
            throw ParsingError("Multiple 'display' elements not supported");
        }
        parseDisplay(*displayElement, res);

        // Parse the files subtree
        const tinyxml2::XMLElement* filesElement = root.FirstChildElement("files");
        if (filesElement == nullptr) {
            throw ParsingError("Missing 'files' element");
        }
        parseFiles(*filesElement, pfmFile, res);

        // Check for unsupported features that we might want to warn about
        const tinyxml2::XMLElement* extSetElem = root.FirstChildElement("extensionSet");
        if (extSetElem) {
            MessageHandler::printWarning("Unsupported feature: %s", extSetElem->Value());
        }

        return res;
    }
} // namespace

ReturnValue parseMpcdiConfiguration(const std::string& filename) {
    unzFile zipfile = unzOpen(filename.c_str());
    if (zipfile == nullptr) {
        throw ParsingError("Unable to open zip archive file " + filename);
    }
    // Get info about the zip file
    unz_global_info globalInfo;
    const int globalInfoRet = unzGetGlobalInfo(zipfile, &globalInfo);
    if (globalInfoRet != UNZ_OK) {
        unzClose(zipfile);
        throw ParsingError("Unable to get zip archive info from " + filename);
    }

    // Search for required files inside mpcdi archive file
    SubFile xmlComponent;
    SubFile pfmComponent;
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
                xmlComponent.fileName = fileName;
                handleSubfile(xmlComponent, zipfile, fileInfo.uncompressed_size);
            }
            else if (endsWith(fileName, "pfm")) {
                pfmComponent.fileName = fileName;
                handleSubfile(pfmComponent, zipfile, fileInfo.uncompressed_size);
            }
            else {
                MessageHandler::printWarning("Ignoring unknown extension %s", fileName);
            }

            if (i < globalInfo.number_entry - 1) {
                const int status = unzGoToNextFile(zipfile);
                if (status != UNZ_OK) {
                    MessageHandler::printWarning("Unable to get next file in archive");
                }
            }
        }
    }
    catch (...) {
        unzClose(zipfile);
        throw;
    }

    unzClose(zipfile);
    if (!xmlComponent.isFound && !pfmComponent.isFound) {
        throw ParsingError(filename + " does not contain the XML and/or PFM file");
    }

    if (xmlComponent.buffer.empty()) {
        throw ParsingError("Empty XML buffer");
    }
    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError result = xmlDoc.Parse(
        xmlComponent.buffer.data(),
        xmlComponent.buffer.size()
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

    ReturnValue res = parseMpcdi(xmlDoc, pfmComponent);
    return res;
}

} // namespace sgct::core::mpcdi
