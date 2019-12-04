/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/correction/paulbourke.h>

#include <sgct/engine.h>
#include <sgct/error.h>
#include <sgct/logger.h>
#include <sgct/window.h>
#include <glm/glm.hpp>

namespace sgct::core::correction {

Buffer generatePaulBourkeMesh(const std::string& path, const glm::ivec2& pos,
                              const glm::ivec2& size, float aspectRatio)
{
    Buffer buf;

    Logger::Info("Reading Paul Bourke spherical mirror mesh from '%s'", path.c_str());

    FILE* meshFile = fopen(path.c_str(), "r");
    if (meshFile == nullptr) {
        throw Error(Error::Component::PaulBourke, 2040, "Failed to open " + path);
    }

    constexpr const int MaxLineLength = 1024;
    char lineBuffer[MaxLineLength];

    // get the fist line containing the mapping type _id
    int mappingType = -1;
    if (fgets(lineBuffer, MaxLineLength, meshFile) != nullptr) {
        int r = sscanf(lineBuffer, "%d", &mappingType);
        if (r != 1) {
            throw Error(Error::Component::PaulBourke, 2041, "Error reading mapping type");
        }
    }

    // get the mesh dimensions
    glm::ivec2 meshSize = glm::ivec2(-1, -1);
    if (fgets(lineBuffer, MaxLineLength, meshFile) != nullptr) {
        if (sscanf(lineBuffer, "%d %d", &meshSize[0], &meshSize[1]) == 2) {
            buf.vertices.reserve(meshSize.x * meshSize.y);
        }
    }

    // check if everyting useful is set
    if (mappingType == -1 || meshSize.x == -1 || meshSize.y == -1) {
        throw Error(Error::Component::PaulBourke, 2042, "Invalid data");
    }

    // get all data
    float x, y, s, t, intensity;
    while (!feof(meshFile)) {
        if (fgets(lineBuffer, MaxLineLength, meshFile) != nullptr) {
            if (sscanf(lineBuffer, "%f %f %f %f %f", &x, &y, &s, &t, &intensity) == 5) {
                CorrectionMeshVertex vertex;
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
    }

    // generate indices
    for (int c = 0; c < (meshSize.x - 1); c++) {
        for (int r = 0; r < (meshSize.y - 1); r++) {
            const int i0 = r * meshSize.x + c;
            const int i1 = r * meshSize.x + (c + 1);
            const int i2 = (r + 1) * meshSize.x + (c + 1);
            const int i3 = (r + 1) * meshSize.x + c;

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
    //const float aspect = Engine::instance().getCurrentWindow().getAspectRatio() *
    //               (size.x / size.y);
    for (CorrectionMeshVertex& vertex : buf.vertices) {
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

} // namespace sgct::core::correction
