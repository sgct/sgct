/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/correction/domeprojection.h>

#include <sgct/error.h>
#include <sgct/fmt.h>
#include <sgct/log.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <glm/glm.hpp>
#include <scn/scn.h>
#include <algorithm>
#include <fstream>

namespace sgct::correction {

Buffer generateDomeProjectionMesh(const std::filesystem::path& path, const vec2& pos,
                                  const vec2& size)
{
    ZoneScoped;

    Log::Info(fmt::format("Reading DomeProjection mesh data from '{}'", path));

    std::ifstream meshFile(path);
    if (!meshFile.good()) {
        throw Error(
            Error::Component::DomeProjection, 2010,
            fmt::format("Failed to open {}", path)
        );
    }

    Buffer buf;

    unsigned int nCols = 0;
    unsigned int nRows = 0;
    std::string line;
    while (std::getline(meshFile, line)) {
        float x;
        float y;
        float u;
        float v;
        unsigned int col;
        unsigned int row;

        auto r = scn::scan(line, "{};{};{};{};{};{}", x, y, u, v, col, row);
        if (r) {
            // init to max intensity (opaque white)
            Buffer::Vertex vertex;
            vertex.r = 1.f;
            vertex.g = 1.f;
            vertex.b = 1.f;
            vertex.a = 1.f;

            // find dimensions of meshdata
            nCols = std::max(nCols, col);
            nRows = std::max(nRows, row);

            x = std::clamp(x, 0.f, 1.f);
            y = std::clamp(y, 0.f, 1.f);

            // convert to [-1, 1]
            vertex.x = 2.f * (pos.x + x * size.x) - 1.f;

            // (abock, 2019-08-30); I'm not sure why the y inversion happens
            // here. It seems like a mistake, but who knows
            vertex.y = 2.f * (pos.y + (1.f - y) * size.y) - 1.f;

            // scale to viewport coordinates
            vertex.s = pos.x + u * size.x;
            vertex.t = pos.y + (1.f - v) * size.y;

            buf.vertices.push_back(std::move(vertex));
        }
    }

    nCols++;
    nRows++;

    for (unsigned int c = 0; c < nCols; ++c) {
        for (unsigned int r = 0; r < nRows; ++r) {
            // 3      2
            //  x____x
            //  |   /|
            //  |  / |
            //  | /  |
            //  |/   |
            //  x----x
            // 0      1

            // add one to actually store the dimensions instead of the largest index
            const unsigned int i0 = r * (nCols + 1)+ c;
            const unsigned int i1 = r * (nCols + 1) + (c + 1);
            const unsigned int i2 = (r + 1) * (nCols + 1) + (c + 1);
            const unsigned int i3 = (r + 1) * (nCols + 1) + c;

            buf.indices.push_back(i0);
            buf.indices.push_back(i1);
            buf.indices.push_back(i2);

            buf.indices.push_back(i0);
            buf.indices.push_back(i2);
            buf.indices.push_back(i3);
        }
    }

    buf.geometryType = GL_TRIANGLES;
    return buf;
}

} // namespace sgct::correction
