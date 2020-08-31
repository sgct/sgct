/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/correction/scalable.h>

#include <sgct/error.h>
#include <sgct/fmt.h>
#include <sgct/log.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <glm/glm.hpp>

namespace sgct::correction {

Buffer generateScalableMesh(const std::string& path, const vec2& pos, const vec2& size) {
    ZoneScoped

    Buffer buf;

    Log::Info(fmt::format("Reading scalable mesh data from '{}'", path));

    FILE* meshFile = fopen(path.c_str(), "r");
    if (meshFile == nullptr) {
        throw Error(
            Error::Component::Scalable, 2060,
            fmt::format("Failed to open '{}'", path)
        );
    }

    unsigned int numOfVerticesRead = 0;
    size_t numOfFacesRead = 0;
    unsigned int numberOfFaces = 0;
    unsigned int numberOfVertices = 0;
    unsigned int numberOfIndices = 0;

    double leftOrtho = 0.0;
    double rightOrtho = 0.0;
    double bottomOrtho = 0.0;
    double topOrtho = 0.0;
    glm::ivec2 res = glm::ivec2(0);

    while (!feof(meshFile)) {
        constexpr const int MaxLineLength = 1024;
        char lineBuffer[MaxLineLength];

        if (fgets(lineBuffer, MaxLineLength, meshFile)) {
            float x, y, s, t;
            unsigned int a, b, c;
            unsigned int intensity;
            if (sscanf(lineBuffer, "%f %f %u %f %f", &x, &y, &intensity, &s, &t) == 5) {
                if (!buf.vertices.empty() && res.x != 0 && res.y != 0) {
                    CorrectionMeshVertex& vertex = buf.vertices[numOfVerticesRead];
                    vertex.x = (x / static_cast<float>(res.x)) * size.x + pos.x;
                    vertex.y = (y / static_cast<float>(res.y)) * size.y + pos.y;
                    vertex.r = static_cast<float>(intensity) / 255.f;
                    vertex.g = static_cast<float>(intensity) / 255.f;
                    vertex.b = static_cast<float>(intensity) / 255.f;
                    vertex.a = 1.f;
                    vertex.s = (1.f - t) * size.x + pos.x;
                    vertex.t = (1.f - s) * size.y + pos.y;

                    numOfVerticesRead++;
                }
            }
            else if (sscanf(lineBuffer, "[ %u %u %u ]", &a, &b, &c) == 3) {
                if (!buf.indices.empty()) {
                    buf.indices[numOfFacesRead * 3] = a;
                    buf.indices[numOfFacesRead * 3 + 1] = b;
                    buf.indices[numOfFacesRead * 3 + 2] = c;
                }

                numOfFacesRead++;
            }
            else {
                char tmpBuf[16];
                tmpBuf[0] = '\0';
                double tmpD = 0.0;
                unsigned int tmpUI = 0;

                if (sscanf(lineBuffer, "VERTICES %u", &numberOfVertices) == 1) {
                    buf.vertices.resize(numberOfVertices);
                    std::fill(
                        buf.vertices.begin(),
                        buf.vertices.end(),
                        CorrectionMeshVertex()
                    );
                }
                else if (sscanf(lineBuffer, "FACES %u", &numberOfFaces) == 1) {
                    numberOfIndices = numberOfFaces * 3;
                    buf.indices.resize(numberOfIndices);
                    std::fill(buf.indices.begin(), buf.indices.end(), 0);
                }
                else if (sscanf(lineBuffer, "ORTHO_%s %lf", tmpBuf, &tmpD) == 2) {
                    std::string_view tmp = tmpBuf;
                    if (tmp == "LEFT") {
                        leftOrtho = tmpD;
                    }
                    else if (tmp == "RIGHT") {
                        rightOrtho = tmpD;
                    }
                    else if (tmp == "BOTTOM") {
                        bottomOrtho = tmpD;
                    }
                    else if (tmp == "TOP") {
                        topOrtho = tmpD;
                    }
                }
                else if (sscanf(lineBuffer, "NATIVEXRES %u", &tmpUI) == 1) {
                    res.x = tmpUI;
                }
                else if (sscanf(lineBuffer, "NATIVEYRES %u", &tmpUI) == 1) {
                    res.y = tmpUI;
                }
            }
        }
    }

    if (numberOfVertices != numOfVerticesRead || numberOfFaces != numOfFacesRead) {
        throw Error(
            Error::Component::Scalable, 2061,
            fmt::format("Incorrect mesh data geometry in file '{}'", path)
        );
    }

    // normalize
    for (unsigned int i = 0; i < numberOfVertices; i++) {
        const float xMin = static_cast<float>(leftOrtho);
        const float xMax = static_cast<float>(rightOrtho);
        const float yMin = static_cast<float>(bottomOrtho);
        const float yMax = static_cast<float>(topOrtho);

        // normalize between 0.0 and 1.0
        const float xVal = (buf.vertices[i].x - xMin) / (xMax - xMin);
        const float yVal = (buf.vertices[i].y - yMin) / (yMax - yMin);

        // normalize between -1.0 to 1.0
        buf.vertices[i].x = xVal * 2.f - 1.f;
        buf.vertices[i].y = yVal * 2.f - 1.f;
    }

    if (meshFile) {
        fclose(meshFile);
    }

    buf.geometryType = GL_TRIANGLES;
    return buf;
}

} // namespace sgct::correction
