/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/correctionmesh.h>

#include <sgct/error.h>
#include <sgct/format.h>
#include <sgct/log.h>
#include <sgct/math.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <sgct/viewport.h>
#include <sgct/window.h>
#include <sgct/correction/domeprojection.h>
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

#define Err(c, msg) sgct::Error(sgct::Error::Component::CorrectionMesh, c, msg)

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

} // namespace

CorrectionMesh::CorrectionMeshGeometry::CorrectionMeshGeometry(
                                                         const correction::Buffer& buffer)
{
    ZoneScoped;
    TracyGpuZone("createMesh");

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
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

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        buffer.indices.size() * sizeof(unsigned int),
        buffer.indices.data(),
        GL_STATIC_DRAW
    );
    glBindVertexArray(0);

    nVertices = static_cast<int>(buffer.vertices.size());
    nIndices = static_cast<int>(buffer.indices.size());
    type = buffer.geometryType;
}

CorrectionMesh::CorrectionMeshGeometry::CorrectionMeshGeometry(
                                                    CorrectionMeshGeometry&& rhs) noexcept
    : vao(rhs.vao)
    , vbo(rhs.vbo)
    , ibo(rhs.ibo)
    , nVertices(rhs.nVertices)
    , nIndices(rhs.nIndices)
    , type(rhs.type)
{
    // We need to prevent a double-free of the OpenGL resource in the destructor
    rhs.vao = 0;
    rhs.vbo = 0;
    rhs.ibo = 0;
}

CorrectionMesh::CorrectionMeshGeometry::~CorrectionMeshGeometry() {
    // Yes, glDeleteVertexArrays and glDeleteBuffers work when passing 0, but this check
    // is a standin for whether they were created in the first place. This would only fail
    // if there is no OpenGL context, which would cause these functions to fail, too.
    if (vao) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    if (vbo) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    if (ibo) {
        glDeleteBuffers(1, &ibo);
        ibo = 0;
    }
}

void CorrectionMesh::loadMesh(const std::filesystem::path& path, BaseViewport& parent,
                              bool needsMaskGeometry, bool textureRenderMode)
{
    ZoneScoped;

    using namespace correction;
    const vec2& parentPos = parent.position();
    const vec2& parentSize = parent.size();

    // generate unwarped mask
    {
        ZoneScopedN("Create simple mask");
        const Buffer buf = setupSimpleMesh(parentPos, parentSize);
        _quadGeometry = CorrectionMeshGeometry(buf);
    }

    // generate unwarped mesh for mask
    if (needsMaskGeometry) {
        ZoneScopedN("Create unwarped mask");
        Log::Debug("CorrectionMesh: Creating mask mesh");

        const Buffer buf = setupMaskMesh(parentPos, parentSize);
        _maskGeometry = CorrectionMeshGeometry(buf);
    }

    // fallback if no mesh is provided
    if (path.empty()) {
        const Buffer buf = setupSimpleMesh(parentPos, parentSize);
        _warpGeometry = CorrectionMeshGeometry(buf);
        return;
    }

    Buffer buf;

    // find a suitable format
    if (path.extension() == ".sgc") {
        buf = generateScissMesh(path, parent);
    }
    else if (path.extension() == ".ol") {
        buf = generateScalableMesh(path, parent);
    }
    else if (path.extension() == ".skyskan") {
        buf = generateSkySkanMesh(path, parent);
    }
    else if (path.extension() == ".txt") {
        buf = generateSkySkanMesh(path, parent);
    }
    else if (path.extension() == ".csv") {
        buf = generateDomeProjectionMesh(path, parentPos, parentSize);
    }
    else if (path.extension() == ".data") {
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
    else if (path.extension() == ".obj") {
        buf = generateOBJMesh(path);
    }
    else if (path.extension() == ".pfm") {
        buf = generatePerEyeMeshFromPFMImage(
            path,
            parentPos,
            parentSize,
            textureRenderMode
        );
    }
    else if (path.extension() == ".simcad") {
        buf = generateSimCADMesh(path, parentPos, parentSize);
    }
    else {
        throw Err(2002, "Could not determine format for warping mesh");
    }

    _warpGeometry = CorrectionMeshGeometry(buf);

    Log::Debug(std::format(
        "CorrectionMesh read successfully. Vertices={}, Indices={}",
        buf.vertices.size(), buf.indices.size()
    ));
}

void CorrectionMesh::CorrectionMeshGeometry::render() const {
    glBindVertexArray(vao);
    glDrawElements(type, nIndices, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void CorrectionMesh::renderQuadMesh() const {
    TracyGpuZone("Render Quad mesh")
    assert(_quadGeometry);
    _quadGeometry->render();
}

void CorrectionMesh::renderWarpMesh() const {
    if (_warpGeometry) {
        TracyGpuZone("Render Warp mesh")
        _warpGeometry->render();
    }
}

void CorrectionMesh::renderMaskMesh() const {
    if (_maskGeometry) {
        TracyGpuZone("Render Mask mesh")
        _maskGeometry->render();
    }
}

} // namespace sgct
