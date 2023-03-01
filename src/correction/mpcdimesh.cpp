/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/correction/mpcdimesh.h>

#include <sgct/error.h>
#include <sgct/log.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <sgct/viewport.h>
#include <cstring>

#define Error(code, msg) sgct::Error(sgct::Error::Component::MPCDIMesh, code, msg)

namespace sgct::correction {

Buffer generateMpcdiMesh(const std::vector<char>& mpcdiMesh) {
    ZoneScoped;

    Buffer buf;

    Log::Info("Reading MPCDI mesh (PFM format) from buffer");

    const char* srcBuff = mpcdiMesh.data();
    size_t srcSizeBytes = mpcdiMesh.size();

    constexpr int MaxHeaderLineLength = 100;
    char headerBuffer[MaxHeaderLineLength];
    unsigned int srcIdx = 0;
    int index = 0;
    int nNewlines = 0;
    do {
        char headerChar = 0;

        if (srcIdx == srcSizeBytes) {
            throw Error(2021, "Error reading from file. Could not find lines");
        }

        headerChar = srcBuff[srcIdx++];
        headerBuffer[index++] = headerChar;
        if (headerChar == '\n') {
            nNewlines++;
        }
    } while (nNewlines < 3);

    char fileFormatHeader[2];
    unsigned int nCols = 0;
    unsigned int nRows = 0;
    const int res = sscanf(headerBuffer, "%2c %u %u", fileFormatHeader, &nCols, &nRows);
    if (res != 3) {
        throw Error(2022, "Invalid header information in MPCDI mesh");
    }

    if (fileFormatHeader[0] != 'P' || fileFormatHeader[1] != 'F') {
        //The 'Pf' header is invalid because PFM grayscale type is not supported.
        throw Error(2023, "Incorrect file type. Unknown header type");
    }
    const unsigned int nCorrectionValues = nCols * nRows;
    std::vector<vec2> corrGrid(nCorrectionValues);
    for (unsigned int i = 0; i < nCorrectionValues; ++i) {
        float x;
        std::memcpy(&x, &srcBuff[srcIdx], sizeof(float));
        srcIdx += sizeof(float);
        corrGrid[i].x = x;

        float y;
        std::memcpy(&y, &srcBuff[srcIdx], sizeof(float));
        srcIdx += sizeof(float);
        corrGrid[i].y = y;

        // error position; we skip those here
        srcIdx += sizeof(float);
    }

    std::vector<vec2> smoothPos(nCorrectionValues);
    std::vector<vec2> warpedPos(nCorrectionValues);
    for (unsigned int i = 0; i < nCorrectionValues; ++i) {
        const float gridIdxCol = static_cast<float>(i % nCols);
        const float gridIdxRow = static_cast<float>(i / nCols);
        // Compute XY positions for each point based on a normalized 0,0 to 1,1 grid,
        // add the correction offsets to each warp point
        smoothPos[i].x = gridIdxCol / static_cast<float>(nCols - 1);
        // Reverse the y position as the values from the PFM file are given in raster-scan
        // order, which is left to right but starts at upper-left rather than lower-left.
        smoothPos[i].y = 1.f - (gridIdxRow / static_cast<float>(nRows - 1));
        warpedPos[i].x = smoothPos[i].x + corrGrid[i].x;
        warpedPos[i].y = smoothPos[i].y + corrGrid[i].y;
    }

    corrGrid.clear();

    buf.vertices.reserve(nCorrectionValues);
    for (unsigned int i = 0; i < nCorrectionValues; ++i) {
        Buffer::Vertex vertex;
        // init to max intensity (opaque white)
        vertex.r = 1.f;
        vertex.g = 1.f;
        vertex.b = 1.f;
        vertex.a = 1.f;

        vertex.s = smoothPos[i].x;
        vertex.t = smoothPos[i].y;

        // scale to viewport coordinates
        vertex.x = 2.f * warpedPos[i].x - 1.f;
        vertex.y = 2.f * warpedPos[i].y - 1.f;
        buf.vertices.push_back(vertex);
    }
    warpedPos.clear();
    smoothPos.clear();

    buf.indices.reserve(6 * static_cast<size_t>(nCorrectionValues));
    for (unsigned int c = 0; c < (nCols - 1); ++c) {
        for (unsigned int r = 0; r < (nRows - 1); ++r) {
            const unsigned int i0 = r * nCols + c;
            const unsigned int i1 = r * nCols + (c + 1);
            const unsigned int i2 = (r + 1) * nCols + (c + 1);
            const unsigned int i3 = (r + 1) * nCols + c;

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

    buf.geometryType = GL_TRIANGLES;
    return buf;
}

} // namespace sgct::correction
