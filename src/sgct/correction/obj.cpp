/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/correction/skyskan.h>

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

Buffer generateOBJMesh(const std::string& path) {
    Buffer buf;

    MessageHandler::printInfo(
        "CorrectionMesh: Reading Maya Wavefront OBJ mesh data from '%s'", path.c_str()
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
        MessageHandler::printError(
            "CorrectionMesh: Failed to open warping mesh file '%s'", path.c_str()
        );
        return Buffer();
    }

    unsigned int counter = 0;
    while (!feof(meshFile)) {
        char lineBuffer[MaxLineLength];
        if (fgets(lineBuffer, MaxLineLength, meshFile) != nullptr) {
            CorrectionMeshVertex tmpVert;

            int i0, i1, i2;
            if (_sscanf(lineBuffer, "v %f %f %*f", &tmpVert.x, &tmpVert.y) == 2) {
                tmpVert.r = 1.f;
                tmpVert.g = 1.f;
                tmpVert.b = 1.f;
                tmpVert.a = 1.f;

                buf.vertices.push_back(tmpVert);
            }
            else if (_sscanf(lineBuffer, "vt %f %f %*f", &tmpVert.s, &tmpVert.t) == 2) {
                if (counter < buf.vertices.size()) {
                    buf.vertices[counter].s = tmpVert.s;
                    buf.vertices[counter].t = tmpVert.t;
                }
                
                counter++;
            }
            else if (_sscanf(
                        lineBuffer,
                        "f %d/%*d/%*d %d/%*d/%*d %d/%*d/%*d", &i0, &i1, &i2
                    ) == 3)
            {
                // indexes starts at 1 in OBJ
                buf.indices.push_back(i0 - 1);
                buf.indices.push_back(i1 - 1);
                buf.indices.push_back(i2 - 1);
            }
        }
    }

    // sanity check
    if (counter != buf.vertices.size() || buf.vertices.empty()) {
        MessageHandler::printError(
            "CorrectionMesh: Vertex count doesn't match number of texture coordinates"
        );
        return Buffer();
    }

    buf.isComplete = true;
    buf.geometryType = GL_TRIANGLES;

    return buf;
}

} // namespace sgct::core::correction
