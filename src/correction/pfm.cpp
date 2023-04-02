/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/correction/pfm.h>

#include <sgct/error.h>
#include <sgct/fmt.h>
#include <sgct/log.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <glm/glm.hpp>
#include <scn/scn.h>
#include <fstream>

namespace sgct::correction {

Buffer generatePerEyeMeshFromPFMImage(const std::filesystem::path& path, const vec2& pos,
                                      const vec2& size)
{
    ZoneScoped;

    Buffer buf;

    Log::Info(fmt::format("Reading 3D/stereo mesh data (in PFM image) from {}", path));

    std::ifstream meshFile(path, std::ifstream::binary);
    if (!meshFile.good()) {
        throw Error(Error::Component::Pfm, 2050, fmt::format("Failed to open {}", path));
    }

    // Read the first three lines
    std::string header;
    std::string dummy;
    std::getline(meshFile, header);
    std::getline(meshFile, dummy);
    std::getline(meshFile, dummy);

    std::string fileFormatHeader;
    unsigned int nCols = 0;
    unsigned int nRows = 0;
    float endiannessIndicator = 0;
    auto result = scn::scan(
        header,
        "{} {} {} {}", fileFormatHeader, nCols, nRows, endiannessIndicator
    );
    if (!result || fileFormatHeader.size() == 2) {
        throw Error(
            Error::Component::Pfm, 2052,
            fmt::format("Invalid header syntax in file {}", path)
        );
    }

    if (fileFormatHeader[0] != 'P' || fileFormatHeader[1] != 'F') {
        throw Error(
            Error::Component::Pfm, 2053,
            fmt::format("Incorrect file type in file {}", path)
        );
    }

    const int numCorrectionValues = nCols * nRows;
    std::vector<float> xcorrections;
    xcorrections.resize(numCorrectionValues);
    std::vector<float> ycorrections;
    ycorrections.resize(numCorrectionValues);

    for (int i = 0; i < numCorrectionValues; ++i) {
        meshFile.read(reinterpret_cast<char*>(xcorrections.data() + i), sizeof(float));
        meshFile.read(reinterpret_cast<char*>(ycorrections.data() + i), sizeof(float));
        float dumpValue;
        meshFile.read(reinterpret_cast<char*>(&dumpValue), sizeof(float));

        if (!meshFile.good()) {
            throw Error(
                Error::Component::Pfm, 2054,
                fmt::format("Error reading correction values in file {}", path)
            );
        }
    }

    nCols /= 2;

    // Images are stored with X 0-1 (left to right), but Y 1 to 0 (top-bottom)

    // We assume we loaded side-by-side images, i.e. different warp per eye
    for (size_t e = 0; e < 2; e++) {
        Buffer::Vertex vertex;
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
                float x = xcorrections[i + (e * nCols)];
                float y = ycorrections[i + (e * nCols)];

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
