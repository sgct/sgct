/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/mpcdi.h>

#include <sgct/messagehandler.h>
#include <sgct/viewport.h>
#include <tinyxml2.h>
#include <unzip.h>
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace sgct::core::mpcdi {

namespace {
    class ParsingError : public std::runtime_error {
    public:
        ParsingError(const std::string& msg) : std::runtime_error("[MPCDI] " + msg) {}
    };

    struct SubFile {
        std::string fileName;
        std::vector<char> buffer;
    };

    [[nodiscard]] config::MpcdiProjection parseRegion(const tinyxml2::XMLElement& elem) {
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

        const tinyxml2::XMLElement* child = elem.FirstChildElement("frustum");
        if (child == nullptr) {
            throw ParsingError("Missing child element 'frustum'");
        }
        const tinyxml2::XMLElement& c = *child;

        const tinyxml2::XMLElement* right = c.FirstChildElement("rightAngle");
        const tinyxml2::XMLElement* left = c.FirstChildElement("leftAngle");
        const tinyxml2::XMLElement* up = c.FirstChildElement("upAngle");
        const tinyxml2::XMLElement* down = c.FirstChildElement("downAngle");
        const tinyxml2::XMLElement* yaw = c.FirstChildElement("yaw");
        const tinyxml2::XMLElement* pitch = c.FirstChildElement("pitch");
        const tinyxml2::XMLElement* roll = c.FirstChildElement("roll");

        if (!(right && left && up && down && yaw && pitch && roll)) {
            throw ParsingError("Failed to parse frustum element. Missing element");
        }

        config::MpcdiProjection::Frustum frustum;
        glm::quat quat;
        try {
            frustum.down = std::stof(down->GetText());
            frustum.up = std::stof(up->GetText());
            frustum.left = std::stof(left->GetText());
            frustum.right = std::stof(right->GetText());

            const float y = std::stof(yaw->GetText());
            const float p = std::stof(pitch->GetText());
            const float r = std::stof(roll->GetText());
            quat = glm::rotate(quat, glm::radians(-y), glm::vec3(0.f, 1.f, 0.f));
            quat = glm::rotate(quat, glm::radians(p), glm::vec3(1.f, 0.f, 0.f));
            quat = glm::rotate(quat, glm::radians(-r), glm::vec3(0.f, 0.f, 1.f));
        }
        catch (const std::invalid_argument&) {
            throw ParsingError("Failed to parse frustum element. Conversion error");
        }
                        
        proj.frustum = frustum;
        proj.orientation = quat;

        return proj;
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

        if (elem.FirstChildElement("coordinateFrame")) {
            MessageHandler::printWarning("Unsupported feature: coordinateFrame");
        }
        if (elem.FirstChildElement("color")) {
            MessageHandler::printWarning("Unsupported feature: color");
        }

        const tinyxml2::XMLElement* region = elem.FirstChildElement("region");
        while (region) {
            // Require an 'id' attribute for each region. These will be compared later
            // to the fileset, in which there must be a matching 'id'.
            if (!region->Attribute("id")) {
                throw ParsingError("No 'id' attribute provided for region");
            }

            ReturnValue::ViewportInfo v;
            v.proj = parseRegion(*region);
            res.viewports.push_back(v);
            region = region->NextSiblingElement("region");
        }
    }

    void parseDisplay(const tinyxml2::XMLElement& element, ReturnValue& res) {
        const tinyxml2::XMLElement* buffer = element.FirstChildElement("buffer");
        if (buffer->NextSiblingElement("buffer")) {
            throw ParsingError("Multiple 'buffer' elements not supported");
        }
        parseBuffer(*buffer, res);
    }

    void parseGeoWarpFile(const tinyxml2::XMLElement& element, const std::string& region,
                          const SubFile& pfmFile, ReturnValue& res)
    {
        const tinyxml2::XMLElement* interp = element.FirstChildElement("interpolation");
        if (interp == nullptr) {
            throw ParsingError("GeometryWarpFile requires interpolation");
        }
        if (std::string_view(interp->GetText()) != "linear") {
            throw ParsingError("Only linear interpolation is supported");
        }

        const tinyxml2::XMLElement* path = element.FirstChildElement("path");
        if (path == nullptr) {
            throw ParsingError("GeometryWarpFile requires path");
        }
        std::string_view pathWarpFile(path->GetText());

        // Look for matching MPCDI region (SGCT viewport) to pass the warp field data to
        bool foundMatchingPfmBuffer = false;
        for (ReturnValue::ViewportInfo& vpInfo : res.viewports) {
            const std::string& viewportId = *vpInfo.proj.id;
            if (viewportId == region && pathWarpFile == pfmFile.fileName) {
                vpInfo.meshData = pfmFile.buffer;
                foundMatchingPfmBuffer = true;
            }
        }
        if (!foundMatchingPfmBuffer) {
            throw ParsingError("No matching geometryWarpFile found");
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

            const tinyxml2::XMLElement* c = child->FirstChildElement("geometryWarpFile");
            while (c) {
                if (c->FirstChildElement("alphaMap")) {
                    MessageHandler::printWarning("Unsupported feature: alphaMap");
                }
                if (c->FirstChildElement("betaMap")) {
                    MessageHandler::printWarning("Unsupported feature: betaMap");
                }
                if (c->FirstChildElement("distortionMap")) {
                    MessageHandler::printWarning("Unsupported feature: distortionMap");
                }
                if (c->FirstChildElement("decodeLUT")) {
                    MessageHandler::printWarning("Unsupported feature: decodeLUT");
                }
                if (c->FirstChildElement("correctLUT")) {
                    MessageHandler::printWarning("Unsupported feature: correctLUT");
                }
                if (child->FirstChildElement("encodeLUT")) {
                    MessageHandler::printWarning("Unsupported feature: encodeLUT");
                }

                parseGeoWarpFile(*c, fileRegion, pfm, res);

                c = c->NextSiblingElement("geometryWarpFile");
            }
            child = child->NextSiblingElement("fileset");
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
    unzFile zip = unzOpen(filename.c_str());
    if (zip == nullptr) {
        throw ParsingError("Unable to open zip archive file " + filename);
    }
    // Get info about the zip file
    unz_global_info globalInfo;
    const int globalInfoRet = unzGetGlobalInfo(zip, &globalInfo);
    if (globalInfoRet != UNZ_OK) {
        unzClose(zip);
        throw ParsingError("Unable to get zip archive info from " + filename);
    }

    // Search for required files inside mpcdi archive file
    std::vector<char> xmlBuffer;
    SubFile pfmComponent;
    try {
        for (unsigned int i = 0; i < globalInfo.number_entry; ++i) {
            unz_file_info info;
            char buf[500];
            int r = unzGetCurrentFileInfo(zip, &info, buf, 500, nullptr, 0, nullptr, 0);
            if (r != UNZ_OK) {
                throw ParsingError("Unable to get info on file " + std::to_string(i));
            }
            std::string fileName(buf);
            const unsigned long uncompSize = info.uncompressed_size;

            constexpr auto endsWith = [](const std::string& str, const std::string& ext) {
                const size_t s = str.size();
                const size_t e = ext.size();
                return s >= e && str.compare(s - e, e, ext) == 0;
            };

            if (endsWith(fileName, "xml")) {
                // Uncompress the XML file
                const int openCurrentFile = unzOpenCurrentFile(zip);
                if (openCurrentFile != UNZ_OK) {
                    throw ParsingError("Unable to open " + fileName);
                }
                xmlBuffer.resize(uncompSize);
                const int size = unzReadCurrentFile(zip, xmlBuffer.data(), uncompSize);
                if (size < 0) {
                    throw ParsingError("Read from " + fileName + " failed");
                }
            }
            else if (endsWith(fileName, "pfm")) {
                // Uncompress the PFM file
                if (!pfmComponent.buffer.empty()) {
                    MessageHandler::printWarning(
                        "Duplicate file %s found in MPCDI", fileName.c_str()
                    );
                }

                const int openCurrentFile = unzOpenCurrentFile(zip);
                if (openCurrentFile != UNZ_OK) {
                    throw ParsingError("Unable to open " + fileName);
                }
                std::vector<char> buffer(uncompSize);
                const int size = unzReadCurrentFile(zip, buffer.data(), uncompSize);
                if (size < 0) {
                    throw ParsingError("Read from " + fileName + " failed");
                }
                pfmComponent.fileName = fileName;
                pfmComponent.buffer = std::move(buffer);
            }
            else {
                MessageHandler::printWarning("Ignoring extension %s", fileName.c_str());
            }

            if (i < globalInfo.number_entry - 1) {
                const int success = unzGoToNextFile(zip);
                if (success != UNZ_OK) {
                    MessageHandler::printWarning("Unable to get next file in archive");
                }
            }
        }
    }
    catch (...) {
        unzClose(zip);
        throw;
    }

    unzClose(zip);
    if (xmlBuffer.empty() || pfmComponent.buffer.empty()) {
        throw ParsingError(filename + " does not contain the XML and/or PFM file");
    }

    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError result = xmlDoc.Parse(xmlBuffer.data(), xmlBuffer.size());

    if (result != tinyxml2::XML_NO_ERROR) {
        std::string str = "Parsing failed after: ";
        str += xmlDoc.GetErrorStr1() + ' ';
        str += xmlDoc.GetErrorStr2();
        throw ParsingError("Error parsing file. " + str);
    }

    ReturnValue res = parseMpcdi(xmlDoc, pfmComponent);
    return res;
}

} // namespace sgct::core::mpcdi
