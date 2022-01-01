/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2022                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/correction/pfm.h>

#include <sgct/error.h>
#include <sgct/fmt.h>
#include <sgct/log.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <glm/glm.hpp>

namespace sgct::correction {

Buffer generatePerEyeMeshFromPFMImage(const std::string& path, const vec2& pos,
                                      const vec2& size)
{
    ZoneScoped

    Buffer buf;

    Log::Info(fmt::format("Reading 3D/stereo mesh data (in PFM image) from '{}'", path));

    FILE* meshFile = fopen(path.c_str(), "rb");
    if (meshFile == nullptr) {
        throw Error(
            Error::Component::Pfm, 2050,
            fmt::format("Failed to open '{}'", path)
        );
    }

    constexpr const int MaxLineLength = 1024;
    char headerBuffer[MaxLineLength];
    int index = 0;
    int nNewlines = 0;
    constexpr const int read3lines = 3;
    do {
        char headerChar;
        size_t retval = fread(&headerChar, sizeof(char), 1, meshFile);
        if (retval != 1) {
            fclose(meshFile);
            throw Error(
                Error::Component::Pfm, 2051,
                fmt::format("Error reading from file '{}'", path)
            );
        }
        headerBuffer[index++] = headerChar;
        if (headerChar == '\n') {
            nNewlines++;
        }
    } while (nNewlines < read3lines);

    char fileFormatHeader[2];
    unsigned int nCols = 0;
    unsigned int nRows = 0;
    float endiannessIndicator = 0;

    const int scanRes = sscanf(
        headerBuffer,
        "%2c %u %u %f",
        fileFormatHeader,
        &nCols,
        &nRows,
        &endiannessIndicator
    );
    if (scanRes != 4) {
        fclose(meshFile);
        throw Error(
            Error::Component::Pfm, 2052,
            fmt::format("Invalid header syntax in file '{}'", path)
        );
    }

    if (fileFormatHeader[0] != 'P' || fileFormatHeader[1] != 'F') {
        throw Error(
            Error::Component::Pfm, 2053,
            fmt::format("Incorrect file type in file '{}'", path)
        );
    }

    const int numCorrectionValues = nCols * nRows;
    float* xcorrections = new float[numCorrectionValues];
    float* ycorrections = new float[numCorrectionValues];
    const int value32bit = 4;

    for (int i = 0; i < numCorrectionValues; ++i) {
        size_t r0 = fread(xcorrections + i, value32bit, 1, meshFile);
        size_t r1 = fread(ycorrections + i, value32bit, 1, meshFile);
        float dumpValue;
        size_t r2 = fread(&dumpValue, value32bit, 1, meshFile);

        if (r0 != 1 || r1 != 1 || r2 != 1) {
            throw Error(
                Error::Component::Pfm, 2054,
                fmt::format("Error reading correction values in file '{}'")
            );
        }
    }

    fclose(meshFile);

    nCols /= 2;

    // Images are stored with X 0-1 (left to right), but Y 1 to 0 (top-bottom)

    // We assume we loaded side-by-side images, i.e. different warp per eye
    for (size_t e = 0; e < 2; e++) {
        CorrectionMeshVertex vertex;
        vertex.r = 1.f;
        vertex.g = 1.f;
        vertex.b = 1.f;
        vertex.a = 1.f;

        size_t i = 0;

        for (unsigned int r = 0; r < nRows; r++) {
            for (unsigned int c = 0; c < nCols; c++) {
                // vertex-mapping
                const float u =
                    (static_cast<float>(c) / (static_cast<float>(nCols + 1) - 1.f)) +
                    ((1.f / nCols) * 0.5f);
                const float v =
                    1.f - ((static_cast<float>(r) / (static_cast<float>(nRows + 1) - 1.f))
                    + ((1.f / nRows) * 0.5f));

                // @TODO (abock, 2020-01-10) Not sure about this one; it will always be
                // true for the loop but it looks like a fork for the e==0 and e==1 paths
                float x;
                float y;
                if (e < 2) {
                    x = xcorrections[i + (e * nCols)];
                    y = ycorrections[i + (e * nCols)];
                }
                else {
                    x = u;
                    y = v;
                }

                // convert to [-1, 1]
                vertex.x = 2.f * (x * size.x + pos.x) - 1.f;
                vertex.y = 2.f * (y * size.y + pos.y) - 1.f;

                // scale to viewport coordinates
                vertex.s = u * size.x + pos.x;
                vertex.t = v * size.y + pos.y;

                buf.vertices.push_back(vertex);

                i++;
            }
            i += nCols;
        }

        // Make a triangle strip index list
        for (unsigned int r = 0; r < nRows - 1; r++) {
            if ((r & 1) == 0) {
                // even rows
                for (unsigned int c = 0; c < nCols; c++) {
                    buf.indices.push_back(c + r * nCols);
                    buf.indices.push_back(c + (r + 1) * nCols);
                }
            }
            else {
                // odd rows
                for (unsigned int c = nCols - 1; c > 0; c--) {
                    buf.indices.push_back(c + (r + 1) * nCols);
                    buf.indices.push_back(c - 1 + r * nCols);
                }
            }
        }
    }

    buf.geometryType = GL_TRIANGLE_STRIP;
    return buf;
}

} // namespace sgct::correction
