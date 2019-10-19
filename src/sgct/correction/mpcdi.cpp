/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/correction/mpcdi.h>

#include <sgct/messagehandler.h>
#include <sgct/viewport.h>

#if (_MSC_VER >= 1400)
    #define _sscanf sscanf_s
#else
    #define _sscanf sscanf
#endif

namespace {
    bool readMeshBuffer(float* dest, unsigned int& idx, const unsigned char* src,
                        size_t srcSizeBytes, int readSizeBytes)
    {
        if ((idx + readSizeBytes) > srcSizeBytes) {
            sgct::MessageHandler::printError("CorrectionMesh: Reached EOF in buffer");
            return false;
        }
        float val;
        memcpy(&val, &src[idx], readSizeBytes);
        *dest = val;
        idx += readSizeBytes;
        return true;
    }
} // namespace

namespace sgct::core::correction {

Buffer generateMpcdiMesh(const std::string& path, const core::Viewport& parent) {
    Buffer buf;

    bool isReadingFile = !path.empty();

    FILE* meshFile = nullptr;
    size_t srcSizeBytes = 0;
    const unsigned char* srcBuff = nullptr;
    if (isReadingFile) {
        MessageHandler::printInfo(
            "CorrectionMesh: Reading MPCDI mesh (PFM format) data from '%s'", path.c_str()
        );
        bool loadSuccess = false;
#if (_MSC_VER >= 1400)
        loadSuccess = fopen_s(&meshFile, path.c_str(), "r") == 0;
#else
        meshFile = fopen(path.c_str(), "rb");
        loadSuccess = meshFile != nullptr;
#endif
        if (!loadSuccess) {
            char ErrorBuffer[1024];
            sprintf(ErrorBuffer, "Failed to open warping mesh file '%s'", path.c_str());
            throw std::runtime_error(ErrorBuffer);
        }
    }
    else {
        MessageHandler::printInfo(
            "CorrectionMesh: Reading MPCDI mesh (PFM format) from buffer", path.c_str()
        );
        srcBuff = parent.mpcdiWarpMesh().data();
        srcSizeBytes = parent.mpcdiWarpMesh().size();
    }

    constexpr const int MaxHeaderLineLength = 100;
    char headerBuffer[MaxHeaderLineLength];

    unsigned int srcIdx = 0;
    int index = 0;
    int nNewlines = 0;
    do {
        size_t retval;
        char headerChar = 0;
        if (isReadingFile) {
#if (_MSC_VER >= 1400)
            retval = fread_s(&headerChar, sizeof(char), sizeof(char), 1, meshFile);
#else
            retval = fread(&headerChar, sizeof(char), 1, meshFile);
#endif
        }
        else {
            if (srcIdx == srcSizeBytes) {
                retval = static_cast<size_t>(-1);
            }
            else {
                headerChar = srcBuff[srcIdx++];
                retval = 1;
            }
        }
        if (retval != 1) {
            if (meshFile) {
                fclose(meshFile);
            }
            throw std::runtime_error("Error reading from file");
        }

        headerBuffer[index++] = headerChar;
        if (headerChar == '\n') {
            nNewlines++;
        }
    } while (nNewlines < 3);

    char fileFormatHeader[2];
    unsigned int numberOfCols = 0;
    unsigned int numberOfRows = 0;
    float endiannessIndicator = 0;

    // @TODO (abock, 2019-08-30): I do not really understand how any of this works
#ifdef WIN32
    _sscanf(
        headerBuffer,
        "%2c\n",
        &fileFormatHeader,
        static_cast<unsigned int>(sizeof(fileFormatHeader))
    );
    // Read header past the 2 character start
    _sscanf(&headerBuffer[3], "%d %d\n", &numberOfCols, &numberOfRows);

    constexpr auto nDigits = [](unsigned int i) {
        return static_cast<int>(ceil(log(static_cast<double>(i))));;
    };
    const int indexForEndianness = 3 + nDigits(numberOfCols) + nDigits(numberOfRows) + 2;
    _sscanf(&headerBuffer[indexForEndianness], "%f\n", &endiannessIndicator);
#else
    if (_sscanf(headerBuffer, "%2c %d %d %f", fileFormatHeader,
                &numberOfCols, &numberOfRows, &endiannessIndicator) != 4)
    {
        if (isReadingFile) {
            fclose(meshFile);
        }
        throw std::runtime_error("Invalid header syntax");
    }
#endif

    if (fileFormatHeader[0] != 'P' || fileFormatHeader[1] != 'F') {
        //The 'Pf' header is invalid because PFM grayscale type is not supported.
        throw std::runtime_error("Incorrect file type");
    }
    const int numCorrectionValues = numberOfCols * numberOfRows;
    std::vector<float> corrGridX(numCorrectionValues);
    std::vector<float> corrGridY(numCorrectionValues);
    float errorPos;

    if (isReadingFile) {
        size_t ret = 0;
        for (int i = 0; i < numCorrectionValues; ++i) {
#ifdef WIN32
            fread_s(&corrGridX[i], numCorrectionValues, sizeof(int), 1, meshFile);
            fread_s(&corrGridY[i], numCorrectionValues, sizeof(int), 1, meshFile);
            // MPCDI uses the PFM format for correction grid. PFM format is designed for
            // 3 RGB values. However MPCDI substitutes Red for X correction, Green for Y
            // correction, and Blue for correction error. This will be NaN for error value
            ret = fread_s(&errorPos, numCorrectionValues, sizeof(int), 1, meshFile);
#else
            fread(&corrGridX[i], sizeof(float), 1, meshFile);
            fread(&corrGridY[i], sizeof(float), 1, meshFile);
            ret = fread(&errorPos, sizeof(float), 1, meshFile);
#endif
        }
        fclose(meshFile);
        if (ret != 4) {
            throw std::runtime_error("Error reading all correction values");
        }
    }
    else {
        bool readErr = false;
        for (int i = 0; i < numCorrectionValues; ++i) {
            readErr |= !readMeshBuffer(&corrGridX[i], srcIdx, srcBuff, srcSizeBytes, 4);
            readErr |= !readMeshBuffer(&corrGridY[i], srcIdx, srcBuff, srcSizeBytes, 4);
            readErr |= !readMeshBuffer(&errorPos, srcIdx, srcBuff, srcSizeBytes, 4);

            if (readErr) {
                char ErrorBuf[1024];
                sprintf(ErrorBuf, "Error reading MPCDI correction value at index %d", i);
                throw std::runtime_error(ErrorBuf);
            }
        }
    }

    std::vector<glm::vec2> smoothPos(numCorrectionValues);
    std::vector<glm::vec2> warpedPos(numCorrectionValues);
    for (int i = 0; i < numCorrectionValues; ++i) {
        const int gridIndex_column = i % numberOfCols;
        const int gridIndex_row = i / numberOfCols;
        // Compute XY positions for each point based on a normalized 0,0 to 1,1 grid,
        // add the correction offsets to each warp point
        smoothPos[i].x = static_cast<float>(gridIndex_column) /
                         static_cast<float>(numberOfCols - 1);
        // Reverse the y position because the values from pfm file are given in raster-scan
        // order, which is left to right but starts at upper-left rather than lower-left.
        smoothPos[i].y = 1.f - (static_cast<float>(gridIndex_row) /
                         static_cast<float>(numberOfRows - 1));
        warpedPos[i].x = smoothPos[i].x + corrGridX[i];
        warpedPos[i].y = smoothPos[i].y + corrGridY[i];
    }

    corrGridX.clear();
    corrGridY.clear();

#ifdef NORMALIZE_CORRECTION_MESH
    glm::vec2 max = *std::max_element(warpedPos, warpedPos + numCorrectionValues);
    glm::vec2 min = *std::min_element(warpedPos, warpedPos + numCorrectionValues);
    float scaleRangeX = max.x - min.x;
    float scaleRangeY = max.y - min.y;
    float scaleFactor = std::max(scaleRangeX, scaleRangeY);
    // Scale all positions to fit within 0,0 to 1,1
    for (int i = 0; i < numCorrectionValues; ++i) {
        warpedPos[i].x = (warpedPos[i].x - minX) / scaleFactor;
        warpedPos[i].y = (warpedPos[i].y - minY) / scaleFactor;
    }
#endif // NORMALIZE_CORRECTION_MESH

    buf.vertices.reserve(numCorrectionValues);
    for (int i = 0; i < numCorrectionValues; ++i) {
        CorrectionMeshVertex vertex;
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

    buf.indices.reserve(numberOfCols * numberOfRows * 6);
    for (unsigned int c = 0; c < (numberOfCols - 1); c++) {
        for (unsigned int r = 0; r < (numberOfRows - 1); r++) {
            const unsigned int i0 = r * numberOfCols + c;
            const unsigned int i1 = r * numberOfCols + (c + 1);
            const unsigned int i2 = (r + 1) * numberOfCols + (c + 1);
            const unsigned int i3 = (r + 1) * numberOfCols + c;

            // 3      2
            //  x____x
            //  |   /|
            //  |  / |
            //  | /  |
            //  |/   |
            //  x----x
            // 0      1

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

} // namespace sgct::core::correction
