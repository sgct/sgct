/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/correction/simcad.h>

#include <sgct/error.h>
#include <sgct/fmt.h>
#include <sgct/log.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <sgct/tinyxml.h>
#include <sgct/viewport.h>
#include <glm/glm.hpp>
#include <sstream>

#define Error(code, msg) Error(Error::Component::SimCAD, code, msg)

namespace {
    std::vector<std::string> split(std::string str, char delimiter) {
        std::vector<std::string> res;
        std::stringstream ss(std::move(str));
        std::string part;
        while (std::getline(ss, part, delimiter)) {
            res.push_back(part);
        }
        return res;
    }
} // namespace

namespace sgct::correction {

Buffer generateSimCADMesh(const std::filesystem::path& path, const vec2& pos,
                          const vec2& size)
{
    ZoneScoped;

    // During projector alignment of 33x33 matrix is used to define geometry correction.
    // The corrections are stored in the warp file. This explains why this file only
    // contains zero's when no warp is applied.

    Buffer buf;

    Log::Info(fmt::format("Reading simcad warp data from '{}'", path));

    tinyxml2::XMLDocument xmlDoc;
    std::string p = path.string();
    if (xmlDoc.LoadFile(p.c_str()) != tinyxml2::XML_SUCCESS) {
        std::string s1 = xmlDoc.ErrorName() ? xmlDoc.ErrorName() : "";
        std::string s2 = xmlDoc.ErrorStr() ? xmlDoc.ErrorStr() : "";
        throw Error(2080, fmt::format("Error loading file {}. {} {}", path, s1, s2));
    }

    tinyxml2::XMLElement* XMLroot = xmlDoc.FirstChildElement("GeometryFile");
    if (XMLroot == nullptr) {
        throw Error(
            2081,
            fmt::format("Error reading file '{}'. Missing 'GeometryFile'", path)
        );
    }

    using namespace tinyxml2;
    XMLElement* element = XMLroot->FirstChildElement("GeometryDefinition");
    if (element == nullptr) {
        throw Error(
            2082,
            fmt::format("Error reading file '{}'. Missing 'GeometryDefinition'", path)
        );
    }

    std::vector<float> xcorrections, ycorrections;
    XMLElement* child = element->FirstChildElement();
    while (child) {
        std::string_view childVal = child->Value();

        if (childVal == "X-FlatParameters") {
            float xrange = 1.f;
            if (child->QueryFloatAttribute("range", &xrange) == XML_SUCCESS) {
                std::string xcoordstr(child->GetText());
                std::vector<std::string> xcoords = split(xcoordstr, ' ');
                for (const std::string& x : xcoords) {
                    xcorrections.push_back(std::stof(x) / xrange);
                }
            }
        }
        else if (childVal == "Y-FlatParameters") {
            float yrange = 1.f;
            if (child->QueryFloatAttribute("range", &yrange) == XML_SUCCESS) {
                std::string ycoordstr(child->GetText());
                std::vector<std::string> ycoords = split(ycoordstr, ' ');
                for (const std::string& y : ycoords) {
                    ycorrections.push_back(std::stof(y) / yrange);
                }
            }
        }

        child = child->NextSiblingElement();
    }

    if (xcorrections.size() != ycorrections.size()) {
        throw Error(2083, "Not the same x coords as y coords");
    }

    const float nColumnsf = sqrt(static_cast<float>(xcorrections.size()));
    const float nRowsf = sqrt(static_cast<float>(ycorrections.size()));

    if (ceil(nColumnsf) != nColumnsf || ceil(nRowsf) != nRowsf) {
        throw Error(2084, "Not a valid squared matrix read from SimCAD file");
    }

    const size_t nCols = static_cast<unsigned int>(nColumnsf);
    const size_t nRows = static_cast<unsigned int>(nRowsf);

    // init to max intensity (opaque white)
    Buffer::Vertex vertex;
    vertex.r = 1.f;
    vertex.g = 1.f;
    vertex.b = 1.f;
    vertex.a = 1.f;

    size_t i = 0;
    for (size_t r = 0; r < nRows; r++) {
        for (size_t c = 0; c < nCols; c++) {
            // vertex-mapping
            const float u = (static_cast<float>(c) / (nCols - 1.f));

            // (abock, 2019-09-01);  Not sure why we are inverting the y coordinate for
            // this, but we do it multiple times in this file and I'm starting to get the
            // impression that there might be a flipping issue going on somewhere deeper
            const float v = 1.f - (static_cast<float>(r) / (nRows - 1.f));

            const float x = u + xcorrections[i];
            const float y = v - ycorrections[i];

            // convert to [-1, 1]
            vertex.x = 2.f * (x * size.x + pos.x) - 1.f;
            vertex.y = 2.f * (y * size.y + pos.y) - 1.f;

            // scale to viewport coordinates
            vertex.s = u * size.x + pos.x;
            vertex.t = v * size.y + pos.y;

            buf.vertices.push_back(vertex);
            i++;
        }
    }

    // Make a triangle strip index list
    buf.indices.reserve(4 * nRows * nCols);
    for (size_t r = 0; r < nRows - 1; r++) {
        if ((r % 2) == 0) {
            // even rows
            for (size_t c = 0; c < nCols; c++) {
                buf.indices.push_back(static_cast<unsigned int>(c + r * nCols));
                buf.indices.push_back(static_cast<unsigned int>(c + (r + 1) * nCols));
            }
        }
        else {
            // odd rows
            for (size_t c = nCols - 1; c > 0; c--) {
                buf.indices.push_back(static_cast<unsigned int>(c + (r + 1) * nCols));
                buf.indices.push_back(static_cast<unsigned int>(c - 1 + r * nCols));
            }
        }
    }

    buf.geometryType = GL_TRIANGLE_STRIP;
    return buf;
}

} // namespace sgct::correction
