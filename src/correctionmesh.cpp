/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/correctionmesh.h>

#include <sgct/baseviewport.h>
#include <sgct/error.h>
#include <sgct/format.h>
#include <sgct/log.h>
#include <sgct/math.h>
#include <sgct/profiling.h>
#include <sgct/viewport.h>
#include <sgct/window.h>
#include <sgct/correction/buffer.h>
#include <sgct/correction/domeprojection.h>
#include <sgct/correction/obj.h>
#include <sgct/correction/paulbourke.h>
#include <sgct/correction/pfm.h>
#include <sgct/correction/scalable.h>
#include <sgct/correction/sciss.h>
#include <sgct/correction/simcad.h>
#include <sgct/correction/skyskan.h>
#include <sgct/projection/fisheye.h>
#include <sgct/vulkan/buffer.h>
#include <sgct/vulkan/context.h>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <vector>

#define Err(c, msg) sgct::Error(sgct::Error::Component::CorrectionMesh, c, msg)

namespace {
    using namespace sgct;

    correction::Buffer setupMaskMesh(const vec2& pos, const vec2& size) {
        correction::Buffer buff = {
            .vertices = {
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
            },
            .indices = { 0, 3, 1, 2 },
            .geometryType = correction::Buffer::GeometryType::TriangleStrip
        };
        return buff;
    }

    correction::Buffer setupSimpleMesh(const vec2& pos, const vec2& size) {
        correction::Buffer buff = {
            .vertices = {
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
            },
            .indices = { 0, 3, 1, 2 },
            .geometryType = correction::Buffer::GeometryType::TriangleStrip
        };
        return buff;
    }
} // namespace

namespace sgct {

CorrectionMesh::CorrectionMeshGeometry::CorrectionMeshGeometry(
                                                         const correction::Buffer& buffer)
{
    using Vertex = correction::Buffer::Vertex;

    ZoneScoped;

    // All meshes are drawn as triangle lists so that a single pipeline serves all of
    // them. Triangle strips are expanded here; the meshes contain no primitive restart
    // indices, and the degenerate triangles that stitch strip rows together have no area
    // and therefore produce no fragments
    std::vector<uint32_t> indices;
    if (buffer.geometryType == correction::Buffer::GeometryType::Triangles) {
        indices.assign(buffer.indices.begin(), buffer.indices.end());
    }
    else if (buffer.geometryType == correction::Buffer::GeometryType::TriangleStrip) {
        if (buffer.indices.size() >= 3) {
            indices.reserve((buffer.indices.size() - 2) * 3);
            for (size_t i = 0; i + 2 < buffer.indices.size(); i++) {
                indices.push_back(buffer.indices[i]);
                indices.push_back(buffer.indices[i + 1]);
                indices.push_back(buffer.indices[i + 2]);
            }
        }
    }
    else {
        throw Err(2003, std::format(
            "Unsupported correction mesh geometry type {}",
            static_cast<int>(buffer.geometryType)
        ));
    }

    if (buffer.vertices.empty() || indices.empty()) {
        throw Err(2004, "Correction mesh contains no geometry");
    }

    vulkan::Context& ctx = vulkan::Context::instance();

    const VkDeviceSize vertexSize = buffer.vertices.size() * sizeof(Vertex);
    vertexBuffer = std::make_unique<vulkan::Buffer>(
        ctx, vertexSize, vulkan::Buffer::Type::Vertex
    );
    vertexBuffer->upload(buffer.vertices.data(), vertexSize);

    const VkDeviceSize indexSize = indices.size() * sizeof(uint32_t);
    indexBuffer = std::make_unique<vulkan::Buffer>(
        ctx, indexSize, vulkan::Buffer::Type::Index
    );
    indexBuffer->upload(indices.data(), indexSize);

    nIndices = static_cast<uint32_t>(indices.size());
}

CorrectionMesh::CorrectionMeshGeometry::CorrectionMeshGeometry(
                                           CorrectionMeshGeometry&&) noexcept = default;

CorrectionMesh::CorrectionMeshGeometry&
CorrectionMesh::CorrectionMeshGeometry::operator=(
                                           CorrectionMeshGeometry&&) noexcept = default;

CorrectionMesh::CorrectionMeshGeometry::~CorrectionMeshGeometry() = default;

void CorrectionMesh::loadMesh(const std::filesystem::path& path, BaseViewport& parent,
                              bool needsMaskGeometry, bool textureRenderMode)
{
    ZoneScoped;

    using namespace correction;
    const vec2& parentPos = parent.position();
    const vec2& parentSize = parent.size();

    // Generate unwarped mask
    {
        ZoneScopedN("Create simple mask");
        const Buffer buf = setupSimpleMesh(parentPos, parentSize);
        _quadGeometry = CorrectionMeshGeometry(buf);
    }

    // Generate unwarped mesh for mask
    if (needsMaskGeometry) {
        ZoneScopedN("Create unwarped mask");
        Log::Debug("CorrectionMesh: Creating mask mesh");

        const Buffer buf = setupMaskMesh(parentPos, parentSize);
        _maskGeometry = CorrectionMeshGeometry(buf);
    }

    // Fallback if no mesh is provided
    if (path.empty()) {
        const Buffer buf = setupSimpleMesh(parentPos, parentSize);
        _warpGeometry = CorrectionMeshGeometry(buf);
        return;
    }

    Buffer buf;

    // Find a suitable format
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

        // Force regeneration of dome render quad
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

void CorrectionMesh::CorrectionMeshGeometry::render(VkCommandBuffer cmd) const {
    const VkBuffer vb = vertexBuffer->handle();
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &offset);
    vkCmdBindIndexBuffer(cmd, indexBuffer->handle(), 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, nIndices, 1, 0, 0, 0);
}

void CorrectionMesh::renderQuadMesh(VkCommandBuffer cmd) const {
    assert(_quadGeometry);
    _quadGeometry->render(cmd);
}

void CorrectionMesh::renderWarpMesh(VkCommandBuffer cmd) const {
    if (_warpGeometry) {
        _warpGeometry->render(cmd);
    }
}

void CorrectionMesh::renderMaskMesh(VkCommandBuffer cmd) const {
    if (_maskGeometry) {
        _maskGeometry->render(cmd);
    }
}

} // namespace sgct
