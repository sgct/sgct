/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/correctionmesh.h>

#include <sgct/error.h>
#include <sgct/fisheyeprojection.h>
#include <sgct/logger.h>
#include <sgct/settings.h>
#include <sgct/viewport.h>
#include <sgct/window.h>
#include <sgct/correction/domeprojection.h>
#include <sgct/correction/mpcdimesh.h>
#include <sgct/correction/obj.h>
#include <sgct/correction/paulbourke.h>
#include <sgct/correction/scalable.h>
#include <sgct/correction/sciss.h>
#include <sgct/correction/simcad.h>
#include <sgct/correction/skyskan.h>
#include <algorithm>
#include <fstream>
#include <iomanip>

#define Error(c, msg) sgct::Error(sgct::Error::Component::CorrectionMesh, c, msg)

namespace sgct {

namespace {
correction::Buffer setupMaskMesh(const glm::vec2& pos, const glm::vec2& size) {
    correction::Buffer buff;
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

    buff.geometryType = GL_TRIANGLE_STRIP;

    return buff;
}

sgct::correction::Buffer setupSimpleMesh(const glm::vec2& pos,
                                                const glm::vec2& size)
{
    sgct::correction::Buffer buff;
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

    buff.geometryType = GL_TRIANGLE_STRIP;

    return buff;
}

void exportMesh(GLenum type, const std::string& exportPath,
                const sgct::correction::Buffer& buf)
{
    if (type != GL_TRIANGLES && type != GL_TRIANGLE_STRIP) {
        throw Error(
            2000, "Failed to export " + exportPath + ". Geometry type is not supported"
        );
    }

    std::ofstream file(exportPath, std::ios::out);
    if (!file.is_open()) {
        throw Error(2001, "Failed to export " + exportPath + ". Failed to open");
    }

    file << std::fixed;
    file << std::setprecision(6);
    file << "# SGCT warping mesh\n# Number of vertices: " << buf.vertices.size() << "\n";

    // export vertices
    for (const sgct::correction::CorrectionMeshVertex& vertex : buf.vertices) {
        file << "v " << vertex.x << ' ' << vertex.y << " 0\n";
    }

    // export texture coords
    for (const sgct::correction::CorrectionMeshVertex& vertex : buf.vertices) {
        file << "vt " << vertex.s << ' ' << vertex.t << " 0\n";
    }

    // export generated normals
    for (size_t i = 0; i < buf.vertices.size(); ++i) {
        file << "vn 0 0 1\n";
    }

    file << "# Number of faces: " << buf.indices.size() / 3 << '\n';

    // export face indices
    if (type == GL_TRIANGLES) {
        for (unsigned int i = 0; i < buf.indices.size(); i += 3) {
            file << "f " << buf.indices[i] + 1 << '/' << buf.indices[i] + 1 << '/'
                 << buf.indices[i] + 1 << ' ';
            file << buf.indices[i + 1] + 1 << '/' << buf.indices[i + 1] + 1 << '/'
                 << buf.indices[i + 1] + 1 << ' ';
            file << buf.indices[i + 2] + 1 << '/' << buf.indices[i + 2] + 1 << '/'
                 << buf.indices[i + 2] + 1 << '\n';
        }
    }
    else {
        // triangle strip

        // first base triangle
        file << "f " << buf.indices[0] + 1 << '/' << buf.indices[0] + 1 << '/'
             << buf.indices[0] + 1 << ' ';
        file << buf.indices[1] + 1 << '/' << buf.indices[1] + 1 << '/'
             << buf.indices[1] + 1 << ' ';
        file << buf.indices[2] + 1 << '/' << buf.indices[2] + 1 << '/'
             << buf.indices[2] + 1 << '\n';

        for (unsigned int i = 2; i < buf.indices.size(); i++) {
            file << "f " << buf.indices[i] + 1 << '/' << buf.indices[i] + 1 << '/'
                 << buf.indices[i] + 1 << ' ';
            file << buf.indices[i - 1] + 1 << '/' << buf.indices[i - 1] + 1 << '/'
                 << buf.indices[i - 1] + 1 << ' ';
            file << buf.indices[i - 2] + 1 << '/' << buf.indices[i - 2] + 1 << '/'
                 << buf.indices[i - 2] + 1 << '\n';
        }
    }

    Logger::Info("Mesh '%s' exported successfully", exportPath.c_str());
}

} // namespace

CorrectionMesh::Format parseCorrectionMeshHint(const std::string& hintStr) {
    if (hintStr == "domeprojection") { return CorrectionMesh::Format::DomeProjection; }
    if (hintStr == "scalable")       { return CorrectionMesh::Format::Scaleable; }
    if (hintStr == "sciss")          { return CorrectionMesh::Format::Sciss; }
    if (hintStr == "simcad")         { return CorrectionMesh::Format::SimCad; }
    if (hintStr == "skyskan")        { return CorrectionMesh::Format::SkySkan; }
    if (hintStr == "mpcdi")          { return CorrectionMesh::Format::Mpcdi;}
    if (!hintStr.empty()) {
        Logger::Warning("Unknown CorrectionMesh hint '%s'", hintStr.c_str());
    }
    return CorrectionMesh::Format::None;
}

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

void CorrectionMesh::loadMesh(std::string path, BaseViewport& parent, Format hint,
                              bool needsMaskGeometry)
{
    using namespace correction;
    const glm::vec2& parentPos = parent.position();
    const glm::vec2& parentSize = parent.size();

    // generate unwarped mask
    {
        Buffer buf = setupSimpleMesh(parentPos, parentSize);
        createMesh(_quadGeometry, buf);
    }
    
    // generate unwarped mesh for mask
    if (needsMaskGeometry) {
    //if (parent.hasBlendMaskTexture() || parent.hasBlackLevelMaskTexture()) {
        Logger::Debug("CorrectionMesh: Creating mask mesh");

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

    // find a suitable format
    if (path.find(".sgc") != std::string::npos) {
        buf = generateScissMesh(path, parent);
    }
    else if (path.find(".ol") != std::string::npos) {
        buf = generateScalableMesh(path, parentPos, parentSize);
    }
    else if (path.find(".skyskan") != std::string::npos) {
        buf = generateSkySkanMesh(path, parent);
    }
    else if ((path.find(".txt") != std::string::npos) &&
            (hint == Format::None || hint == Format::SkySkan))
    {
        buf = generateSkySkanMesh(path, parent);
    }
    else if ((path.find(".csv") != std::string::npos) &&
            (hint == Format::None || hint == Format::DomeProjection))
    {
        buf = generateDomeProjectionMesh(path, parentPos, parentSize);
    }
    else if ((path.find(".data") != std::string::npos) &&
            (hint == Format::None || hint == Format::PaulBourke))
    {
        const float aspectRatio = parent.window().aspectRatio();
        buf = generatePaulBourkeMesh(path, parentPos, parentSize, aspectRatio);

        // force regeneration of dome render quad
        if (Viewport* vp = dynamic_cast<Viewport*>(&parent); vp) {
            auto fishPrj = dynamic_cast<FisheyeProjection*>(vp->nonLinearProjection());
            if (fishPrj) {
                fishPrj->setIgnoreAspectRatio(true);
                fishPrj->update(glm::ivec2(1.f, 1.f));
            }
        }
    }
    else if ((path.find(".obj") != std::string::npos) &&
            (hint == Format::None || hint == Format::Obj))
    {
        buf = generateOBJMesh(path);
    }
    else if (path.find(".mpcdi") != std::string::npos) {
        const Viewport* vp = dynamic_cast<const Viewport*>(&parent);
        if (vp == nullptr) {
            throw Error(2020, "Configuration error. Trying load MPCDI to wrong viewport");
        }
        buf = generateMpcdiMesh(vp->mpcdiWarpMesh());
    }
    else if ((path.find(".simcad") != std::string::npos) &&
            (hint == Format::None || hint == Format::SimCad))
    {
        buf = generateSimCADMesh(path, parentPos, parentSize);
    }
    else {
        throw Error(2002, "Could not determine format for warping mesh");
    }

    createMesh(_warpGeometry, buf);

    Logger::Debug(
        "CorrectionMesh read successfully. Vertices=%u, Indices=%u",
        static_cast<int>(buf.vertices.size()), static_cast<int>(buf.indices.size())
    );

    if (Settings::instance().exportWarpingMeshes()) {
        const size_t found = path.find_last_of(".");
        std::string filename = path.substr(0, found) + "_export.obj";
        exportMesh(_warpGeometry.type, std::move(filename), buf);
    }
}

void CorrectionMesh::renderQuadMesh() const {
    glBindVertexArray(_quadGeometry.vao);
    glDrawElements(_quadGeometry.type, _quadGeometry.nIndices, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void CorrectionMesh::renderWarpMesh() const {
    glBindVertexArray(_warpGeometry.vao);
    glDrawElements(_warpGeometry.type, _warpGeometry.nIndices, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void CorrectionMesh::renderMaskMesh() const {
    glBindVertexArray(_maskGeometry.vao);
    glDrawElements(_maskGeometry.type, _maskGeometry.nIndices, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void CorrectionMesh::createMesh(CorrectionMeshGeometry& geom,
                                const correction::Buffer& buffer)
{
    glGenVertexArrays(1, &geom.vao);
    glBindVertexArray(geom.vao);

    glGenBuffers(1, &geom.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, geom.vbo);
    constexpr const int s = sizeof(correction::CorrectionMeshVertex);
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
