/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/correctionmesh.h>

#include <sgct/error.h>
#include <sgct/fmt.h>
#include <sgct/log.h>
#include <sgct/math.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <sgct/settings.h>
#include <sgct/viewport.h>
#include <sgct/window.h>
#include <sgct/correction/domeprojection.h>
#include <sgct/correction/mpcdimesh.h>
#include <sgct/correction/obj.h>
#include <sgct/correction/paulbourke.h>
#include <sgct/correction/pfm.h>
#include <sgct/correction/scalable.h>
#include <sgct/correction/sciss.h>
#include <sgct/correction/simcad.h>
#include <sgct/correction/skyskan.h>
#include <sgct/projection/fisheye.h>
#include <algorithm>
#include <fstream>
#include <iomanip>

#define Error(c, msg) sgct::Error(sgct::Error::Component::CorrectionMesh, c, msg)

namespace sgct {

namespace {

correction::Buffer setupMaskMesh(const vec2& pos, const vec2& size) {
    correction::Buffer buff;
    buff.geometryType = GL_TRIANGLE_STRIP;
    buff.indices = { 0, 3, 1, 2 };
    buff.vertices = {
        {
            2.f * pos.x - 1.f, 2.f * pos.y - 1.f,
            0.f, 0.f,
            1.f, 1.f, 1.f, 1.f
        },
        {
            2.f * (pos.x + size.x) - 1.f, 2.f * pos.y - 1.f,
            1.f, 0.f,
            1.f, 1.f, 1.f, 1.f
        },
        {
            2.f * (pos.x + size.x) - 1.f, 2.f * (pos.y + size.y) - 1.f,
            1.f, 1.f,
            1.f, 1.f, 1.f, 1.f
        },
        {
            2.f * pos.x - 1.f, 2.f * (pos.y + size.y) - 1.f,
            0.f, 1.f,
            1.f, 1.f, 1.f, 1.f
        }
    };
    return buff;
}

correction::Buffer setupSimpleMesh(const vec2& pos, const vec2& size) {
    correction::Buffer buff;
    buff.geometryType = GL_TRIANGLE_STRIP;
    buff.indices = { 0, 3, 1, 2 };
    buff.vertices = {
        {
            2.f * pos.x - 1.f, 2.f * pos.y - 1.f,
            pos.x, pos.y,
            1.f, 1.f, 1.f, 1.f
        },
        {
            2.f * (pos.x + size.x) - 1.f, 2.f * pos.y - 1.f,
            pos.x + size.x, pos.y,
            1.f, 1.f, 1.f, 1.f
        },
        {
            2.f * (pos.x + size.x) - 1.f, 2.f * (pos.y + size.y) - 1.f,
            1.f * size.x + pos.x, 1.f * size.y + pos.y,
            1.f, 1.f, 1.f, 1.f
        },
        {
            2.f * pos.x - 1.f, 2.f * (pos.y + size.y) - 1.f,
            pos.x, pos.y + size.y,
            1.f, 1.f, 1.f, 1.f
        }
    };
    return buff;
}

void exportMesh(GLenum type, const std::string& path, const correction::Buffer& buf) {
    if (type != GL_TRIANGLES && type != GL_TRIANGLE_STRIP) {
        throw Error(
            2000,
            fmt::format("Failed to export '{}'. Geometry type not supported", path)
        );
    }

    std::ofstream file(path, std::ios::out);
    if (!file.is_open()) {
        throw Error(
            2001,
            fmt::format("Failed to export '{}'. Failed to open", path)
        );
    }

    file << std::fixed;
    file << std::setprecision(6);
    file << "# SGCT warping mesh\n# Number of vertices: " << buf.vertices.size() << "\n";

    // export vertices
    for (const sgct::correction::Buffer::Vertex& vertex : buf.vertices) {
        file << fmt::format("v {} {} 0\n", vertex.x, vertex.y);
    }

    // export texture coords
    for (const sgct::correction::Buffer::Vertex& vertex : buf.vertices) {
        file << fmt::format("vt {} {} 0\n", vertex.s, vertex.t);
    }

    // export generated normals
    file.write("vn 0 0 1\n", buf.vertices.size());

    file << fmt::format("# Number of faces: {}\n", buf.indices.size() / 3);

    // export face indices
    if (type == GL_TRIANGLES) {
        for (size_t i = 0; i < buf.indices.size(); i += 3) {
            file << fmt::format(
                "f {0}/{0}/{0} {1}/{1}/{1} {2}/{2}/{2}\n",
                buf.indices[i] + 1, buf.indices[i + 1] + 1, buf.indices[i + 2] + 1
            );
        }
    }
    else {
        // first base triangle
        file << fmt::format(
            "f {0}/{0}/{0} {1}/{1}/{1} {2}/{2}/{2}\n",
            buf.indices[0] + 1, buf.indices[1] + 1, buf.indices[2] + 1
        );

        for (size_t i = 2; i < buf.indices.size(); i++) {
            file << fmt::format(
                "f {0}/{0}/{0} {1}/{1}/{1} {2}/{2}/{2}\n",
                buf.indices[i], buf.indices[i - 1] + 1, buf.indices[i - 2] + 1
            );
        }
    }

    Log::Info(fmt::format("Mesh '{}' exported successfully", path));
}

} // namespace

CorrectionMesh::CorrectionMeshGeometry::~CorrectionMeshGeometry() {
    // Yes, glDeleteVertexArrays and glDeleteBuffers work when passing 0, but this check
    // is a standin for whether they were created in the first place. This would only fail
    // if there is no OpenGL context, which would cause these functions to fail, too.
    if (vao) {
        glDeleteVertexArrays(1, &vao);
    }
    if (vbo) {
        glDeleteBuffers(1, &vbo);
    }
    if (ibo) {
        glDeleteBuffers(1, &ibo);
    }
}

void CorrectionMesh::loadMesh(std::string path, BaseViewport& parent,
                              bool needsMaskGeometry)
{
    ZoneScoped;

    using namespace correction;
    const vec2& parentPos = parent.position();
    const vec2& parentSize = parent.size();

    // generate unwarped mask
    {
        ZoneScopedN("Create simple mask");
        Buffer buf = setupSimpleMesh(parentPos, parentSize);
        createMesh(_quadGeometry, buf);
    }

    // generate unwarped mesh for mask
    if (needsMaskGeometry) {
        ZoneScopedN("Create unwarped mask");
        Log::Debug("CorrectionMesh: Creating mask mesh");

        Buffer buf = setupMaskMesh(parentPos, parentSize);
        createMesh(_maskGeometry, buf);
    }

    // fallback if no mesh is provided
    if (path.empty()) {
        Buffer buf = setupSimpleMesh(parentPos, parentSize);
        createMesh(_warpGeometry, buf);
        return;
    }

    Buffer buf;

    std::string ext = path.substr(path.rfind('.') + 1);
    // find a suitable format
    if (ext == "sgc") {
        buf = generateScissMesh(path, parent);
    }
    else if (ext == "ol") {
        buf = generateScalableMesh(path, parent);
    }
    else if (ext == "skyskan") {
        buf = generateSkySkanMesh(path, parent);
    }
    else if (ext == "txt") {
        buf = generateSkySkanMesh(path, parent);
    }
    else if (ext == "csv") {
        buf = generateDomeProjectionMesh(path, parentPos, parentSize);
    }
    else if (ext == "data") {
        const float aspectRatio = parent.window().aspectRatio();
        buf = generatePaulBourkeMesh(path, parentPos, parentSize, aspectRatio);

        // force regeneration of dome render quad
        if (Viewport* vp = dynamic_cast<Viewport*>(&parent); vp) {
            auto fishPrj = dynamic_cast<FisheyeProjection*>(vp->nonLinearProjection());
            if (fishPrj) {
                fishPrj->setIgnoreAspectRatio(true);
                fishPrj->update(vec2{ 1.f, 1.f });
            }
        }
    }
    else if (ext == "obj") {
        buf = generateOBJMesh(path);
    }
    else if (ext == "pfm") {
        buf = generatePerEyeMeshFromPFMImage(path, parentPos, parentSize);
    }
    else if (ext == "mpcdi") {
        const Viewport* vp = dynamic_cast<const Viewport*>(&parent);
        if (vp == nullptr) {
            throw Error(2020, "Configuration error. Trying load MPCDI to wrong viewport");
        }
        buf = generateMpcdiMesh(vp->mpcdiWarpMesh());
    }
    else if (ext == "simcad") {
        buf = generateSimCADMesh(path, parentPos, parentSize);
    }
    else {
        throw Error(2002, "Could not determine format for warping mesh");
    }

    createMesh(_warpGeometry, buf);

    Log::Debug(fmt::format(
        "CorrectionMesh read successfully. Vertices={}, Indices={}",
        buf.vertices.size(), buf.indices.size()
    ));

    if (Settings::instance().exportWarpingMeshes()) {
        const size_t found = path.find_last_of('.');
        std::string filename = path.substr(0, found) + "_export.obj";
        exportMesh(_warpGeometry.type, std::move(filename), buf);
    }
}

void CorrectionMesh::renderQuadMesh() const {
    TracyGpuZone("Render Quad mesh")

    glBindVertexArray(_quadGeometry.vao);
    glDrawElements(_quadGeometry.type, _quadGeometry.nIndices, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void CorrectionMesh::renderWarpMesh() const {
    TracyGpuZone("Render Warp mesh")

    glBindVertexArray(_warpGeometry.vao);
    glDrawElements(_warpGeometry.type, _warpGeometry.nIndices, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void CorrectionMesh::renderMaskMesh() const {
    TracyGpuZone("Render Mask mesh")

    glBindVertexArray(_maskGeometry.vao);
    glDrawElements(_maskGeometry.type, _maskGeometry.nIndices, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void CorrectionMesh::createMesh(CorrectionMeshGeometry& geom,
                                const correction::Buffer& buffer)
{
    ZoneScoped;
    TracyGpuZone("createMesh");

    glGenVertexArrays(1, &geom.vao);
    glBindVertexArray(geom.vao);

    glGenBuffers(1, &geom.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, geom.vbo);
    constexpr int s = sizeof(correction::Buffer::Vertex);
    glBufferData(
        GL_ARRAY_BUFFER,
        buffer.vertices.size() * s,
        buffer.vertices.data(),
        GL_STATIC_DRAW
    );

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, s, nullptr);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, s, reinterpret_cast<void*>(8));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, s, reinterpret_cast<void*>(16));

    glGenBuffers(1, &geom.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geom.ibo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        buffer.indices.size() * sizeof(unsigned int),
        buffer.indices.data(),
        GL_STATIC_DRAW
    );
    glBindVertexArray(0);

    geom.nVertices = static_cast<int>(buffer.vertices.size());
    geom.nIndices = static_cast<int>(buffer.indices.size());
    geom.type = buffer.geometryType;
}

} // namespace sgct
