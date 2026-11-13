/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/correction/domeprojection.h>

#include <sgct/correction/buffer.h>
#include <sgct/error.h>
#include <sgct/format.h>
#include <sgct/log.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <scn/scan.h>
#include <algorithm>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace sgct::correction {

Buffer generateDomeProjectionMesh(const std::filesystem::path& path,
                                  const glm::vec2& pos, const glm::vec2& size)
{
    ZoneScoped;

    Log::Info(std::format("Reading DomeProjection mesh data from '{}'", path));

    std::ifstream meshFile = std::ifstream(path);
    if (!meshFile.good()) {
        throw Error(
            Error::Component::DomeProjection, 2010,
            std::format("Failed to open '{}'", path)
        );
    }

    Buffer buf;

    // The column and row of each vertex in the order in which they appear in the file
    std::vector<std::pair<unsigned int, unsigned int>> gridPositions;
    unsigned int nCols = 0;
    unsigned int nRows = 0;
    std::string line;
    while (std::getline(meshFile, line)) {
        auto r = scn::scan<float, float, float, float, unsigned int, unsigned int>(
            line, "{};{};{};{};{};{}"
        );
        if (r) {
            auto [x, y, u, v, col, row] = r->values();

            // Init to max intensity (opaque white)
            Buffer::Vertex vertex;
            vertex.r = 1.f;
            vertex.g = 1.f;
            vertex.b = 1.f;
            vertex.a = 1.f;

            // Find dimensions of meshdata
            nCols = std::max(nCols, col);
            nRows = std::max(nRows, row);

            x = std::clamp(x, 0.f, 1.f);
            y = std::clamp(y, 0.f, 1.f);

            // Convert to [-1, 1]
            vertex.x = 2.f * (pos.x + x * size.x) - 1.f;

            // (abock, 2019-08-30); I'm not sure why the y inversion happens here. It
            // seems like a mistake, but who knows
            vertex.y = 2.f * (pos.y + (1.f - y) * size.y) - 1.f;

            // scale to viewport coordinates
            vertex.s = pos.x + u * size.x;
            vertex.t = pos.y + (1.f - v) * size.y;

            buf.vertices.push_back(std::move(vertex));
            gridPositions.emplace_back(col, row);
        }
    }

    if (buf.vertices.empty()) {
        throw Error(
            Error::Component::DomeProjection, 2011,
            std::format("No mesh data found in '{}'", path)
        );
    }

    // Add one to actually store the dimensions instead of the largest index
    nCols++;
    nRows++;

    if (static_cast<size_t>(nCols) * static_cast<size_t>(nRows) != buf.vertices.size()) {
        throw Error(
            Error::Component::DomeProjection, 2012,
            std::format(
                "Mesh in '{}' has {} vertices, but the columns and rows require {}x{}",
                path, buf.vertices.size(), nCols, nRows
            )
        );
    }

    // Sort the vertices into a row-major grid as we can't rely on the order in the file
    std::vector<Buffer::Vertex> grid(buf.vertices.size());
    std::vector<bool> isSet(buf.vertices.size(), false);
    for (size_t i = 0; i < buf.vertices.size(); i++) {
        const auto [col, row] = gridPositions[i];
        const size_t idx = static_cast<size_t>(row) * nCols + col;
        if (isSet[idx]) {
            throw Error(
                Error::Component::DomeProjection, 2012,
                std::format(
                    "Mesh in '{}' contains column {} and row {} multiple times",
                    path, col, row
                )
            );
        }
        grid[idx] = buf.vertices[i];
        isSet[idx] = true;
    }
    buf.vertices = std::move(grid);

    for (unsigned int c = 0; c < nCols - 1; c++) {
        for (unsigned int r = 0; r < nRows - 1; r++) {
            // 3      2
            //  x____x
            //  |   /|
            //  |  / |
            //  | /  |
            //  |/   |
            //  x----x
            // 0      1

            const unsigned int i0 = r * nCols + c;
            const unsigned int i1 = r * nCols + (c + 1);
            const unsigned int i2 = (r + 1) * nCols + (c + 1);
            const unsigned int i3 = (r + 1) * nCols + c;

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
