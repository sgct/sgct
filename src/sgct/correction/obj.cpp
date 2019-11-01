/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/correction/skyskan.h>

#include <sgct/error.h>
#include <sgct/messagehandler.h>

namespace {
    constexpr const int MaxLineLength = 1024;
} // namespace

namespace sgct::core::correction {

Buffer generateOBJMesh(const std::string& path) {
    Buffer buffer;

    MessageHandler::printInfo(
        "CorrectionMesh: Reading Maya Wavefront OBJ mesh data from '%s'", path.c_str()
    );

    FILE* meshFile = nullptr;
    meshFile = fopen(path.c_str(), "r");
    if (meshFile == nullptr) {
        throw Error(Error::Component::OBJ, 2007, "Failed to open " + path);
    }

    unsigned int counter = 0;
    while (!feof(meshFile)) {
        char buf[MaxLineLength];
        if (fgets(buf, MaxLineLength, meshFile) != nullptr) {
            CorrectionMeshVertex tmpVert;

            int i0, i1, i2;
            if (sscanf(buf, "v %f %f %*f", &tmpVert.x, &tmpVert.y) == 2) {
                tmpVert.r = 1.f;
                tmpVert.g = 1.f;
                tmpVert.b = 1.f;
                tmpVert.a = 1.f;

                buffer.vertices.push_back(tmpVert);
            }
            if (sscanf(buf, "vt %f %f %*f", &tmpVert.s, &tmpVert.t) == 2) {
                if (counter < buffer.vertices.size()) {
                    buffer.vertices[counter].s = tmpVert.s;
                    buffer.vertices[counter].t = tmpVert.t;
                }
                
                counter++;
            }
            if (sscanf(buf, "f %d/%*d/%*d %d/%*d/%*d %d/%*d/%*d", &i0, &i1, &i2) == 3) {
                // indexes starts at 1 in OBJ
                buffer.indices.push_back(i0 - 1);
                buffer.indices.push_back(i1 - 1);
                buffer.indices.push_back(i2 - 1);
            }
        }
    }

    // sanity check
    if (counter != buffer.vertices.size() || buffer.vertices.empty()) {
        throw Error(
            Error::Component::OBJ,
            2008,
            "Vertex count doesn't match number of texture coordinates"
        );
    }

    buffer.geometryType = GL_TRIANGLES;
    return buffer;
}

} // namespace sgct::core::correction
