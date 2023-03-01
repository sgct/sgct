/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/correction/sciss.h>

#include <sgct/engine.h>
#include <sgct/error.h>
#include <sgct/fmt.h>
#include <sgct/log.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <sgct/viewport.h>
#include <sgct/user.h>
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <fstream>

#define Error(code, msg) sgct::Error(sgct::Error::Component::SCISS, code, msg)

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

        // Position of view (unused in Uniview)
        float x = 0.f;
        float y = 0.f;
        float z = 0.f;

        float fovUp = 20.f;
        float fovDown = 20.f;
        float fovLeft = 20.f;
        float fovRight = 20.f;
    };
} // namespace

namespace sgct::correction {

Buffer generateScissMesh(const std::filesystem::path& path, BaseViewport& parent) {
    ZoneScoped;

    Buffer buf;

    Log::Info(fmt::format("Reading SCISS mesh data from {}", path));

    
    std::ifstream file(path, std::ifstream::binary);
    if (!file.good()) {
        throw Error(2070, fmt::format("Failed to open {}", path));
    }

    char fileID[3];
    file.read(fileID, 3 * sizeof(char));

    // check fileID
    if (!file.good() || fileID[0] != 'S' || fileID[1] != 'G' || fileID[2] != 'C') {
        throw Error(2071, fmt::format("Incorrect file id in file {}", path));
    }

    // read file version
    uint8_t fileVersion;
    file.read(reinterpret_cast<char*>(&fileVersion), sizeof(uint8_t));
    if (!file.good()) {
        throw Error(2072, fmt::format("Error parsing file version from file {}", path));
    }

    Log::Debug(fmt::format("SCISS file version {}", fileVersion));

    // read mapping type
    unsigned int type;
    file.read(reinterpret_cast<char*>(&type), sizeof(unsigned int));
    if (!file.good()) {
        throw Error(2073, fmt::format("Error parsing type from file {}", path));
    }

    Log::Debug(fmt::format(
        "Mapping type: {} ({})", type == 0 ? "planar" : "cube", type)
    );

    // read viewdata
    SCISSViewData viewData;
    file.read(reinterpret_cast<char*>(&viewData), sizeof(SCISSViewData));
    if (!file.good()) {
        throw Error(2074, fmt::format("Error parsing view data from file {}", path));
    }

    const double x = static_cast<double>(viewData.qx);
    const double y = static_cast<double>(viewData.qy);
    const double z = static_cast<double>(viewData.qz);
    const double w = static_cast<double>(viewData.qw);

    // Switching the Euler angles to switch from a right-handed coordinate system to
    // a left-handed one
    glm::dvec3 angles = glm::degrees(glm::eulerAngles(glm::dquat(w, y, x, z)));
    double yaw = -angles.x;
    double pitch = angles.y;
    double roll = -angles.z;

    Log::Debug(fmt::format(
        "Rotation quat = [{} {} {} {}]. yaw = {}, pitch = {}, roll = {}",
        viewData.qx, viewData.qy, viewData.qz, viewData.qw, yaw, pitch, roll)
    );

    Log::Debug(fmt::format("Position: {} {} {}", viewData.x, viewData.y, viewData.z));

    Log::Debug(fmt::format(
        "FOV: (up {}) (down {}) (left {}) (right {})",
        viewData.fovUp, viewData.fovDown, viewData.fovLeft, viewData.fovRight
    ));

    // read number of vertices
    unsigned int size[2];
    file.read(reinterpret_cast<char*>(size), 2 * sizeof(unsigned int));
    if (!file.good()) {
        throw Error(2075, fmt::format("Error parsing file {}", path));
    }

    unsigned int nVertices = 0;
    if (fileVersion == 2) {
        nVertices = size[1];
        Log::Debug(fmt::format("Number of vertices: {}", nVertices));
    }
    else {
        nVertices = size[0] * size[1];
        Log::Debug(fmt::format(
            "Number of vertices: {} ({}x{})", nVertices, size[0], size[1]
        ));
    }
    // read vertices
    std::vector<SCISSTexturedVertex> texturedVertexList(nVertices);
    file.read(
        reinterpret_cast<char*>(texturedVertexList.data()),
        nVertices * sizeof(SCISSTexturedVertex)
    );
    if (!file.good()) {
        throw Error(2076, fmt::format("Error parsing vertices from file {}", path));
    }

    // read number of indices
    unsigned int nIndices = 0;
    file.read(reinterpret_cast<char*>(&nIndices), sizeof(unsigned int));
    if (!file.good()) {
        throw Error(2077, fmt::format("Error parsing indices from file {}", path));
    }
    Log::Debug(fmt::format("Number of indices: {}", nIndices));

    // read faces
    if (nIndices > 0) {
        buf.indices.resize(nIndices);
        file.read(
            reinterpret_cast<char*>(buf.indices.data()),
            nIndices * sizeof(unsigned int)
        );
        if (!file.good()) {
            throw Error(2078, fmt::format("Error parsing faces from file {}", path));
        }
    }

    parent.user().setPos(vec3{ viewData.x, viewData.y, viewData.z });
    parent.setViewPlaneCoordsUsingFOVs(
        viewData.fovUp,
        viewData.fovDown,
        viewData.fovLeft,
        viewData.fovRight,
        quat{ viewData.qx, viewData.qy, viewData.qz, viewData.qw }
    );

    Engine::instance().updateFrustums();

    buf.vertices.resize(nVertices);
    for (unsigned int i = 0; i < nVertices; i++) {
        SCISSTexturedVertex& scissVertex = texturedVertexList[i];
        scissVertex.x = glm::clamp(scissVertex.x, 0.f, 1.f);
        scissVertex.y = glm::clamp(scissVertex.y, 0.f, 1.f);
        scissVertex.tx = glm::clamp(scissVertex.tx, 0.f, 1.f);
        scissVertex.ty = glm::clamp(scissVertex.ty, 0.f, 1.f);

        const vec2& s = parent.size();
        const vec2& p = parent.position();

        // convert to [-1, 1]
        Buffer::Vertex& vertex = buf.vertices[i];
        vertex.x = 2.f * (scissVertex.x * s.x + p.x) - 1.f;
        vertex.y = 2.f * ((1.f - scissVertex.y) * s.y + p.y) - 1.f;

        vertex.s = scissVertex.tx * parent.size().x + parent.position().x;
        vertex.t = scissVertex.ty * parent.size().y + parent.position().y;

        vertex.r = 1.f;
        vertex.g = 1.f;
        vertex.b = 1.f;
        vertex.a = 1.f;
    }

    if (fileVersion == '2' && size[0] == 4) {
        buf.geometryType = GL_TRIANGLES;
    }
    else {
        buf.geometryType = GL_TRIANGLE_STRIP;
    }

    return buf;
}

} // namespace sgct::correction
