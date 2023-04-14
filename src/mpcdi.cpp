/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/mpcdi.h>

#include <sgct/error.h>
#include <sgct/fmt.h>
#include <sgct/log.h>
#include <sgct/math.h>
#include <sgct/tinyxml.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <minizip/unzip.h>
#include <algorithm>
#include <string_view>

#define Error(code, msg) Error(Error::Component::MPCDI, code, msg)

namespace sgct::mpcdi {

namespace {
    struct SubFile {
        std::string fileName;
        std::vector<char> buffer;
    };

    [[nodiscard]] config::MpcdiProjection parseRegion(const tinyxml2::XMLElement& elem) {
        config::MpcdiProjection proj;
        if (const char* a = elem.Attribute("id"); a) {
            proj.id = a;
        }

        vec2 vpPosition;
        if (elem.QueryFloatAttribute("x", &vpPosition.x) != tinyxml2::XML_SUCCESS ||
            elem.QueryFloatAttribute("y", &vpPosition.y) != tinyxml2::XML_SUCCESS)
        {
            throw Error(4000, "Failed to parse position from XML");
        }
        proj.position = vpPosition;

        vec2 vpSize;
        if (elem.QueryFloatAttribute("xSize", &vpSize.x) != tinyxml2::XML_SUCCESS ||
            elem.QueryFloatAttribute("ySize", &vpSize.y) != tinyxml2::XML_SUCCESS)
        {
            throw Error(4001, "Failed to parse size from XML");
        }
        proj.size = vpSize;

        const tinyxml2::XMLElement* child = elem.FirstChildElement("frustum");
        if (child == nullptr) {
            throw Error(4002, "Missing child element 'frustum'");
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
            throw Error(4003, "Failed to parse frustum element. Missing element");
        }

        config::MpcdiProjection::Frustum frustum;
        glm::quat quat = glm::quat(1.f, 0.f, 0.f, 0.f);
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
            throw Error(4004, "Failed to parse frustum element. Conversion error");
        }

        proj.frustum = frustum;
        proj.orientation = sgct::quat(quat.x, quat.y, quat.z, quat.w);

        return proj;
    }

    void parseBuffer(const tinyxml2::XMLElement& elem, ReturnValue& res) {
        ivec2 resolution = ivec2{ -1, -1 };
        if (elem.Attribute("xResolution")) {
            elem.QueryAttribute("xResolution", &resolution.x);
        }
        if (elem.Attribute("yResolution")) {
            elem.QueryAttribute("yResolution", &resolution.y);
        }
        if (resolution.x <= 0 && resolution.y <= 0) {
            throw Error(4005, "Require both xResolution and yResolution values");
        }
        res.resolution = resolution;

        if (elem.FirstChildElement("coordinateFrame")) {
            Log::Warning("Unsupported feature: coordinateFrame");
        }
        if (elem.FirstChildElement("color")) {
            Log::Warning("Unsupported feature: color");
        }

        const tinyxml2::XMLElement* region = elem.FirstChildElement("region");
        while (region) {
            // Require an 'id' attribute for each region. These will be compared later
            // to the fileset, in which there must be a matching 'id'.
            if (!region->Attribute("id")) {
                throw Error(4006, "No 'id' attribute provided for region");
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
            throw Error(4007, "Multiple 'buffer' elements not supported");
        }
        parseBuffer(*buffer, res);
    }

    void parseGeoWarpFile(const tinyxml2::XMLElement& element, const std::string& region,
                          const SubFile& pfmFile, ReturnValue& res)
    {
        const tinyxml2::XMLElement* interp = element.FirstChildElement("interpolation");
        if (interp == nullptr) {
            throw Error(4008, "GeometryWarpFile requires interpolation");
        }
        if (std::string_view(interp->GetText()) != "linear") {
            throw Error(4009, "Only linear interpolation is supported");
        }

        const tinyxml2::XMLElement* path = element.FirstChildElement("path");
        if (path == nullptr) {
            throw Error(4010, "GeometryWarpFile requires path");
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
            throw Error(4011, "No matching geometryWarpFile found");
        }
    }

    void parseFiles(const tinyxml2::XMLElement& e, const SubFile& pfm, ReturnValue& res) {
        std::string fileRegion;

        const tinyxml2::XMLElement* child = e.FirstChildElement("fileset");
        while (child) {
            // @TODO (abock, 2019-12-06) This layout will mean that subsequent 'fileset'
            // children will overwrite the fileRegion. I'm not sure if this is desired or
            // not, but it's how it is
            if (const char* a = child->Attribute("region"); a) {
                fileRegion = a;
            }

            const tinyxml2::XMLElement* c = child->FirstChildElement("geometryWarpFile");
            while (c) {
                if (c->FirstChildElement("alphaMap")) {
                    Log::Warning("Unsupported feature: alphaMap");
                }
                if (c->FirstChildElement("betaMap")) {
                    Log::Warning("Unsupported feature: betaMap");
                }
                if (c->FirstChildElement("distortionMap")) {
                    Log::Warning("Unsupported feature: distortionMap");
                }
                if (c->FirstChildElement("decodeLUT")) {
                    Log::Warning("Unsupported feature: decodeLUT");
                }
                if (c->FirstChildElement("correctLUT")) {
                    Log::Warning("Unsupported feature: correctLUT");
                }
                if (child->FirstChildElement("encodeLUT")) {
                    Log::Warning("Unsupported feature: encodeLUT");
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
            throw Error(4012, "Cannot find XML root");
        }
        const tinyxml2::XMLElement& root = *rootNode;

        // Verify that we have a correctly formed MPCDI file
        const char* profileAttr = root.Attribute("profile");
        if (profileAttr == nullptr || std::string_view(profileAttr) != "3d") {
            throw Error(4013, "Error parsing MPCDI, missing or wrong 'profile'");
        }
        const char* geometryAttr = root.Attribute("geometry");
        if (geometryAttr == nullptr || std::string_view(geometryAttr) != "1") {
            throw Error(4014, "Error parsing MPCDI, missing or wrong 'geometry'");
        }
        const char* versionAttr = root.Attribute("version");
        if (versionAttr == nullptr || std::string_view(versionAttr) != "2.0") {
            throw Error(4015, "Error parsing MPCDI, missing or wrong 'version'");
        }

        ReturnValue res;
        // Parse the display subtree
        const tinyxml2::XMLElement* displayElement = root.FirstChildElement("display");
        if (displayElement == nullptr) {
            throw Error(4016, "Missing 'display' element");
        }
        if (displayElement->NextSiblingElement("display") != nullptr) {
            throw Error(4017, "Multiple 'display' elements not supported");
        }
        parseDisplay(*displayElement, res);

        // Parse the files subtree
        const tinyxml2::XMLElement* filesElement = root.FirstChildElement("files");
        if (filesElement == nullptr) {
            throw Error(4018, "Missing 'files' element");
        }
        parseFiles(*filesElement, pfmFile, res);

        // Check for unsupported features that we might want to warn about
        const tinyxml2::XMLElement* extSetElem = root.FirstChildElement("extensionSet");
        if (extSetElem) {
            Log::Warning(fmt::format("Unsupported feature: {}", extSetElem->Value()));
        }

        return res;
    }
} // namespace

ReturnValue parseMpcdiConfiguration(const std::string& filename) {
    unzFile zip = unzOpen(filename.c_str());
    if (zip == nullptr) {
        throw Error(4019, fmt::format("Unable to open zip archive file {}", filename));
    }
    // Get info about the zip file
    unz_global_info globalInfo;
    const int globalInfoRet = unzGetGlobalInfo(zip, &globalInfo);
    if (globalInfoRet != UNZ_OK) {
        unzClose(zip);
        throw Error(
            4020, fmt::format("Unable to get zip archive info from {}", filename)
        );
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
                throw Error(4021, fmt::format("Unable to get info on file {}", i));
            }
            std::string fileName(buf);

            constexpr auto endsWith = [](std::string_view str, std::string_view ext) {
                const size_t s = str.size();
                const size_t e = ext.size();
                return s >= e && str.compare(s - e, e, ext) == 0;
            };

            const unsigned int uncompSize = static_cast<unsigned int>(
                info.uncompressed_size
            );
            if (endsWith(fileName, "xml")) {
                // Uncompress the XML file
                const int openCurrentFile = unzOpenCurrentFile(zip);
                if (openCurrentFile != UNZ_OK) {
                    throw Error(4022, fmt::format("Unable to open {}", fileName));
                }
                xmlBuffer.resize(uncompSize);
                const int size = unzReadCurrentFile(zip, xmlBuffer.data(), uncompSize);
                if (size < 0) {
                    throw Error(4023, fmt::format("Read from {} failed", fileName));
                }
            }
            else if (endsWith(fileName, "pfm")) {
                // Uncompress the PFM file
                if (!pfmComponent.buffer.empty()) {
                    Log::Warning(
                        fmt::format("Duplicate file {} found in MPCDI", fileName)
                    );
                }

                const int openCurrentFile = unzOpenCurrentFile(zip);
                if (openCurrentFile != UNZ_OK) {
                    throw Error(4024, fmt::format("Unable to open {}", fileName));
                }
                std::vector<char> buffer(uncompSize);
                const int size = unzReadCurrentFile(zip, buffer.data(), uncompSize);
                if (size < 0) {
                    throw Error(4025, fmt::format("Read from {} failed", fileName));
                }
                pfmComponent.fileName = fileName;
                pfmComponent.buffer = std::move(buffer);
            }
            else {
                Log::Warning(fmt::format("Ignoring extension {}", fileName));
            }

            if (i < globalInfo.number_entry - 1) {
                const int success = unzGoToNextFile(zip);
                if (success != UNZ_OK) {
                    Log::Warning("Unable to get next file in archive");
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
        throw Error(
            4026, fmt::format("{} does not contain the XML and/or PFM file", filename)
        );
    }

    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError result = xmlDoc.Parse(xmlBuffer.data(), xmlBuffer.size());

    if (result != tinyxml2::XML_SUCCESS) {
        throw Error(4027, fmt::format(
            "Error parsing file. Parsing failed after: {}", xmlDoc.ErrorStr()
        ));
    }

    ReturnValue res = parseMpcdi(xmlDoc, pfmComponent);
    return res;
}

} // namespace sgct::mpcdi
