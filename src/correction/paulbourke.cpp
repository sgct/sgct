/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/correction/paulbourke.h>

#include <sgct/engine.h>
#include <sgct/error.h>
#include <sgct/format.h>
#include <sgct/log.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <sgct/window.h>
#include <glm/glm.hpp>
#include <scn/scan.h>
#include <fstream>

namespace sgct::correction {

Buffer generatePaulBourkeMesh(const std::filesystem::path& path, const vec2& pos,
                              const vec2& size, float aspectRatio)
{
    ZoneScoped;

    Buffer buf;

    Log::Info(std::format("Reading Paul Bourke spherical mirror mesh from '{}'", path.string()));

    std::ifstream meshFile = std::ifstream(path);
    if (!meshFile.good()) {
        throw Error(
            Error::Component::PaulBourke, 2040,
            std::format("Failed to open '{}'", path.string())
        );
    }

    std::string line;

    // get the first line containing the mapping type _id
    if (std::getline(meshFile, line)) {
        auto r = scn::scan_value<int>(line);
        if (!r) {
            throw Error(
                Error::Component::PaulBourke, 2041,
                std::format("Error reading mapping type in file '{}'", path.string())
            );
        }
    }

    // get the mesh dimensions
    std::optional<glm::ivec2> meshSize;
    if (std::getline(meshFile, line)) {
        auto r = scn::scan<int, int>(line, "{} {}");
        if (!r) {
            throw Error(
                Error::Component::PaulBourke, 2042,
                std::format("Invalid data in file '{}'", path.string())
            );
        }
        const auto& [valX, valY] = r->values();
        buf.vertices.reserve(static_cast<size_t>(valX) * static_cast<size_t>(valY));
        meshSize = glm::ivec2(valX, valY);
    }

    // get all data
    while (std::getline(meshFile, line)) {
        auto r = scn::scan<float, float, float, float, float>(line, "{} {} {} {} {}");
        if (r) {
            const auto& [x, y, s, t, intensity] = r->values();

            Buffer::Vertex vertex;
            vertex.x = x;
            vertex.y = y;
            vertex.s = s;
            vertex.t = t;

            vertex.r = intensity;
            vertex.g = intensity;
            vertex.b = intensity;
            vertex.a = 1.f;

            buf.vertices.push_back(vertex);
        }
    }

    // generate indices
    for (int c = 0; c < (meshSize->x - 1); c++) {
        for (int r = 0; r < (meshSize->y - 1); r++) {
            const int i0 = r * meshSize->x + c;
            const int i1 = r * meshSize->x + (c + 1);
            const int i2 = (r + 1) * meshSize->x + (c + 1);
            const int i3 = (r + 1) * meshSize->x + c;

            // triangle 1
            buf.indices.push_back(i0);
            buf.indices.push_back(i1);
            buf.indices.push_back(i2);

            // triangle 2
            buf.indices.push_back(i0);
            buf.indices.push_back(i2);
            buf.indices.push_back(i3);
        }
    }

    const float aspect = aspectRatio * (size.x / size.y);
    for (Buffer::Vertex& vertex : buf.vertices) {
        // convert to [0, 1] (normalize)
        vertex.x /= aspect;
        vertex.x = (vertex.x + 1.f) / 2.f;
        vertex.y = (vertex.y + 1.f) / 2.f;

        // scale, re-position and convert to [-1, 1]
        vertex.x = (vertex.x * size.x + pos.x) * 2.f - 1.f;
        vertex.y = (vertex.y * size.y + pos.y) * 2.f - 1.f;

        // convert to viewport coordinates
        vertex.s = vertex.s * size.x + pos.x;
        vertex.t = vertex.t * size.y + pos.y;
    }

    buf.geometryType = GL_TRIANGLES;
    return buf;
}

} // namespace sgct::correction
