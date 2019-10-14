/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/correction/scalable.h>

#include <sgct/correction/buffer.h>
#include <sgct/messagehandler.h>

#if (_MSC_VER >= 1400)
    #define _sscanf sscanf_s
#else
    #define _sscanf sscanf
#endif

namespace {
    constexpr const int MaxLineLength = 1024;
} // namespace

namespace sgct::core::correction {

Buffer generateDomeProjectionMesh(const std::string& path, const glm::ivec2& pos,
                                  const glm::ivec2& size)
{
    MessageHandler::instance()->printInfo(
        "CorrectionMesh: Reading DomeProjection mesh data from '%s'", path.c_str()
    );

    FILE* meshFile = nullptr;
    bool loadSuccess = false;
#if (_MSC_VER >= 1400)
    loadSuccess = fopen_s(&meshFile, path.c_str(), "r") == 0;
#else
    meshFile = fopen(path.c_str(), "r");
    loadSuccess = meshFile != nullptr;
#endif
    if (!loadSuccess) {
        MessageHandler::instance()->printError(
            "CorrectionMesh: Failed to open warping mesh file '%s'", path.c_str()
        );
        return Buffer();
    }

    Buffer buf;

    unsigned int numberOfCols = 0;
    unsigned int numberOfRows = 0;
    while (!feof(meshFile)) {
        char lineBuffer[MaxLineLength];
        if (fgets(lineBuffer, MaxLineLength, meshFile) != nullptr) {
            float x;
            float y;
            float u;
            float v;
            unsigned int col;
            unsigned int row;

            if (_sscanf(lineBuffer, "%f;%f;%f;%f;%u;%u", &x, &y, &u, &v, &col, &row) == 6)
            {
                // init to max intensity (opaque white)
                CorrectionMeshVertex vertex;
                vertex.r = 1.f;
                vertex.g = 1.f;
                vertex.b = 1.f;
                vertex.a = 1.f;

                // find dimensions of meshdata
                numberOfCols = std::max(numberOfCols, col);
                numberOfRows = std::max(numberOfRows, row);

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

    fclose(meshFile);

    // add one to actually store the dimensions instread of largest index
    numberOfCols++;
    numberOfRows++;

    for (unsigned int c = 0; c < (numberOfCols - 1); ++c) {
        for (unsigned int r = 0; r < (numberOfRows - 1); ++r) {
            // 3      2
            //  x____x
            //  |   /|
            //  |  / |
            //  | /  |
            //  |/   |
            //  x----x
            // 0      1

            const unsigned int i0 = r * numberOfCols + c;
            const unsigned int i1 = r * numberOfCols + (c + 1);
            const unsigned int i2 = (r + 1) * numberOfCols + (c + 1);
            const unsigned int i3 = (r + 1) * numberOfCols + c;

            buf.indices.push_back(i0);
            buf.indices.push_back(i1);
            buf.indices.push_back(i2);

            buf.indices.push_back(i0);
            buf.indices.push_back(i2);
            buf.indices.push_back(i3);
        }
    }

    buf.isComplete = true;
    buf.geometryType = GL_TRIANGLES;
    return buf;
}

} // namespace sgct::core::correction
