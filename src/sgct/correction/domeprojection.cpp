/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/correction/domeprojection.h>

#include <sgct/error.h>
#include <sgct/logger.h>
#include <glm/glm.hpp>

namespace sgct::core::correction {

Buffer generateDomeProjectionMesh(const std::string& path, const glm::ivec2& pos,
                                  const glm::ivec2& size)
{
    Logger::Info("Reading DomeProjection mesh data from '%s'", path.c_str());

    FILE* meshFile = nullptr;
    bool loadSuccess = false;
    meshFile = fopen(path.c_str(), "r");
    loadSuccess = meshFile != nullptr;
    if (!loadSuccess) {
        throw Error(Error::Component::DomeProjection, 2002, "Failed to open " + path);
    }

    Buffer buf;

    unsigned int nCols = 0;
    unsigned int nRows = 0;
    while (!feof(meshFile)) {
        constexpr const int MaxLineLength = 1024;
        char lineBuf[MaxLineLength];
        if (fgets(lineBuf, MaxLineLength, meshFile) != nullptr) {
            float x;
            float y;
            float u;
            float v;
            unsigned int col;
            unsigned int row;

            if (sscanf(lineBuf, "%f;%f;%f;%f;%u;%u", &x, &y, &u, &v, &col, &row) == 6) {
                // init to max intensity (opaque white)
                CorrectionMeshVertex vertex;
                vertex.r = 1.f;
                vertex.g = 1.f;
                vertex.b = 1.f;
                vertex.a = 1.f;

                // find dimensions of meshdata
                nCols = std::max(nCols, col);
                nRows = std::max(nRows, row);

                glm::clamp(x, 0.f, 1.f);
                glm::clamp(y, 0.f, 1.f);

                // convert to [-1, 1]
                vertex.x = 2.f * (pos.x + x * size.x) - 1.f;

                // @TODO (abock, 2019-08-30); I'm not sure why the y inversion happens
                // here. It seems like a mistake, but who knows
                vertex.y = 2.f * (pos.y + (1.f - y) * size.y) - 1.f;

                // scale to viewport coordinates
                vertex.s = pos.x + u * size.x;
                vertex.t = pos.y + (1.f - v) * size.y;

                buf.vertices.push_back(std::move(vertex));
            }
        }
    }

    if (meshFile) {
        fclose(meshFile);
    }

    // add one to actually store the dimensions instread of largest index
    nCols++;
    nRows++;

    for (unsigned int c = 0; c < (nCols - 1); ++c) {
        for (unsigned int r = 0; r < (nRows - 1); ++r) {
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

} // namespace sgct::core::correction
