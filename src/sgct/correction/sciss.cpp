/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/correction/simple.h>

#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/viewport.h>
#include <sgct/user.h>

namespace {
    struct SCISSTexturedVertex {
        float x = 0.f;
        float y = 0.f;
        float z = 0.f;
        float tx = 0.f;
        float ty = 0.f;
        float tz = 0.f;
    };

    struct SCISSViewData {
        // Rotation quaternion
        float qx = 0.f;
        float qy = 0.f;
        float qz = 0.f;
        float qw = 1.f;

        // Position of view (currently unused in Uniview)
        float x = 0.f;
        float y = 0.f;
        float z = 0.f;

        float fovUp = 20.f;
        float fovDown = 20.f;
        float fovLeft = 20.f;
        float fovRight = 20.f;
};
} // namespace

namespace sgct::core::correction {

Buffer generateScissMesh(const std::string& path, core::Viewport& parent) {
    Buffer buf;

    MessageHandler::printInfo(
        "CorrectionMesh: Reading sciss mesh data from '%s'", path.c_str()
    );

    FILE* meshFile = nullptr;
    bool loadSuccess = false;
#if (_MSC_VER >= 1400)
    loadSuccess = fopen_s(&meshFile, path.c_str(), "rb") == 0;
#else
    meshFile = fopen(path.c_str(), "rb");
    loadSuccess = meshFile != nullptr;
#endif
    if (!loadSuccess) {
        throw std::runtime_error("Failed to open warping mesh file");
    }

    size_t ret;
    unsigned int numberOfVertices = 0;
    unsigned int numberOfIndices = 0;

    char fileID[3];
#if (_MSC_VER >= 1400)
    ret = fread_s(fileID, sizeof(char) * 3, sizeof(char), 3, meshFile);
#else
    ret = fread(fileID, sizeof(char), 3, meshFile);
#endif

    // check fileID
    if (fileID[0] != 'S' || fileID[1] != 'G' || fileID[2] != 'C' || ret != 3) {
        fclose(meshFile);
        throw std::runtime_error("Incorrect file id");
    }

    // read file version
    uint8_t fileVersion;
#if (_MSC_VER >= 1400)
    ret = fread_s(&fileVersion, sizeof(uint8_t), sizeof(uint8_t), 1, meshFile);
#else
    ret = fread(&fileVersion, sizeof(uint8_t), 1, meshFile);
#endif
    if (ret != 1) {
        fclose(meshFile);
        throw std::runtime_error("Error parsing file");
    }

    MessageHandler::printDebug("CorrectionMesh: file version %u", fileVersion);

    // read mapping type
    unsigned int type;
#if (_MSC_VER >= 1400)
    ret = fread_s(&type, sizeof(unsigned int), sizeof(unsigned int), 1, meshFile);
#else
    ret = fread(&type, sizeof(unsigned int), 1, meshFile);
#endif
    if (ret != 1) {
        fclose(meshFile);
        throw std::runtime_error("Error parsing file");
    }

    MessageHandler::printDebug(
        "CorrectionMesh: Mapping type = %s (%u)", type == 0 ? "planar" : "cube", type
    );

    // read viewdata
    SCISSViewData viewData;
#if (_MSC_VER >= 1400)
    ret = fread_s(&viewData, sizeof(SCISSViewData), sizeof(SCISSViewData), 1, meshFile);
#else
    ret = fread(&viewData, sizeof(SCISSViewData), 1, meshFile);
#endif
    double yaw, pitch, roll;
    if (ret != 1) {
        fclose(meshFile);
        throw std::runtime_error("Error parsing file");
    }

    const double x = static_cast<double>(viewData.qx);
    const double y = static_cast<double>(viewData.qy);
    const double z = static_cast<double>(viewData.qz);
    const double w = static_cast<double>(viewData.qw);
        
    // Switching the Euler angles to switch from a right-handed coordinate system to
    // a left-handed one
    glm::dvec3 angles = glm::degrees(glm::eulerAngles(glm::dquat(w, y, x, z)));
    yaw = -angles.x;
    pitch = angles.y;
    roll = -angles.z;
        
    MessageHandler::printDebug(
        "CorrectionMesh: Rotation quat = [%f %f %f %f]. "
        "yaw = %lf, pitch = %lf, roll = %lf",
        viewData.qx, viewData.qy, viewData.qz, viewData.qw, yaw, pitch, roll);

    MessageHandler::printDebug(
        "CorrectionMesh: Position = [%f %f %f]", viewData.x, viewData.y, viewData.z
    );

    MessageHandler::printDebug("CorrectionMesh: FOV up = %f", viewData.fovUp);
    MessageHandler::printDebug("CorrectionMesh: FOV down = %f", viewData.fovDown);
    MessageHandler::printDebug("CorrectionMesh: FOV left = %f", viewData.fovLeft);
    MessageHandler::printDebug("CorrectionMesh: FOV right = %f", viewData.fovRight);

    // read number of vertices
    unsigned int size[2];
#if (_MSC_VER >= 1400)
    ret = fread_s(size, sizeof(unsigned int) * 2, sizeof(unsigned int), 2, meshFile);
#else
    ret = fread(size, sizeof(unsigned int), 2, meshFile);
#endif
    if (ret != 2) {
        fclose(meshFile);
        throw std::runtime_error("Error parsing file");
    }

    if (fileVersion == 2) {
        numberOfVertices = size[1];
        MessageHandler::printDebug(
            "CorrectionMesh: Number of vertices = %u", numberOfVertices
        );
    }
    else {
        numberOfVertices = size[0] * size[1];
        MessageHandler::printDebug(
            "CorrectionMesh: Number of vertices = %u (%ux%u)",
            numberOfVertices, size[0], size[1]
        );
    }
    // read vertices
    std::vector<SCISSTexturedVertex> texturedVertexList(numberOfVertices);
#if (_MSC_VER >= 1400)
    ret = fread_s(
        texturedVertexList.data(),
        sizeof(SCISSTexturedVertex) * numberOfVertices,
        sizeof(SCISSTexturedVertex),
        numberOfVertices,
        meshFile
    );
#else
    ret = fread(
        texturedVertexList.data(),
        sizeof(SCISSTexturedVertex),
        numberOfVertices,
        meshFile
    );
#endif
    if (ret != numberOfVertices) {
        fclose(meshFile);
        throw std::runtime_error("Error parsing file");
    }

    // read number of indices
#if (_MSC_VER >= 1400)
    ret = fread_s(
        &numberOfIndices,
        sizeof(unsigned int),
        sizeof(unsigned int),
        1,
        meshFile
    );
#else
    ret = fread(&numberOfIndices, sizeof(unsigned int), 1, meshFile);
#endif

    if (ret != 1) {
        fclose(meshFile);
        throw std::runtime_error("Error parsing file");
    }
    MessageHandler::printDebug("CorrectionMesh: Number of indices = %u", numberOfIndices);

    // read faces
    if (numberOfIndices > 0) {
        buf.indices.resize(numberOfIndices);
#if (_MSC_VER >= 1400) //visual studio 2005 or later
        ret = fread_s(
            buf.indices.data(),
            sizeof(unsigned int) * numberOfIndices,
            sizeof(unsigned int),
            numberOfIndices,
            meshFile
        );
#else
        ret = fread(
            buf.indices.data(),
            sizeof(unsigned int),
            numberOfIndices,
            meshFile
        );
#endif
        if (ret != numberOfIndices) {
            fclose(meshFile);
            throw std::runtime_error("Error parsing file");
        }
    }

    fclose(meshFile);

    parent.getUser().setPos(glm::vec3(viewData.x, viewData.y, viewData.z));

    parent.setViewPlaneCoordsUsingFOVs(
        viewData.fovUp,
        viewData.fovDown,
        viewData.fovLeft,
        viewData.fovRight,
        glm::quat(viewData.qw, viewData.qx, viewData.qy, viewData.qz)
    );

    Engine::instance().updateFrustums();

    buf.vertices.resize(numberOfVertices);
    for (unsigned int i = 0; i < numberOfVertices; i++) {
        CorrectionMeshVertex& vertex = buf.vertices[i];
        SCISSTexturedVertex& scissVertex = texturedVertexList[i];

        vertex.r = 1.f;
        vertex.g = 1.f;
        vertex.b = 1.f;
        vertex.a = 1.f;

        scissVertex.x = glm::clamp(scissVertex.x, 0.f, 1.f);
        scissVertex.y = glm::clamp(scissVertex.y, 0.f, 1.f);
        scissVertex.tx = glm::clamp(scissVertex.tx, 0.f, 1.f);
        scissVertex.ty = glm::clamp(scissVertex.ty, 0.f, 1.f);

        const glm::vec2& s = parent.getSize();
        const glm::vec2& p = parent.getPosition();

        // convert to [-1, 1]
        vertex.x = 2.f * (scissVertex.x * s.x + p.x) - 1.f;
        vertex.y = 2.f * ((1.f - scissVertex.y) * s.y + p.y) - 1.f;

        vertex.s = scissVertex.tx * parent.getSize().x + parent.getPosition().x;
        vertex.t = scissVertex.ty * parent.getSize().y + parent.getPosition().y;
    }

    if (fileVersion == '2' && size[0] == 4) {
        buf.geometryType = GL_TRIANGLES;
    }
    else if (fileVersion == '2' && size[0] == 5) {
        buf.geometryType = GL_TRIANGLE_STRIP;
    }
    else {
        // assuming v1
        buf.geometryType = GL_TRIANGLE_STRIP;
    }

    return buf;
}

} // namespace sgct::core::correction
