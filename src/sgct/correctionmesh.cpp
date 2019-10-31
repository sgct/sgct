/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/correctionmesh.h>

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/settings.h>
#include <sgct/viewport.h>
#include <sgct/user.h>
#include <sgct/correction/domeprojection.h>
#include <sgct/correction/mpcdimesh.h>
#include <sgct/correction/obj.h>
#include <sgct/correction/paulbourke.h>
#include <sgct/correction/scalable.h>
#include <sgct/correction/sciss.h>
#include <sgct/correction/simcad.h>
#include <sgct/correction/simple.h>
#include <sgct/correction/skyskan.h>
#include <algorithm>
#include <fstream>
#include <iomanip>

namespace {
    void exportMesh(GLenum type, const std::string& exportPath,
                    const sgct::core::correction::Buffer& buf)
    {
        if (type != GL_TRIANGLES && type != GL_TRIANGLE_STRIP) {
            sgct::MessageHandler::printError(
                "CorrectionMesh: Failed to export '%s'. Geometry type is not supported",
                exportPath.c_str()
            );
            return;
        }

        std::ofstream file(exportPath, std::ios::out);
        if (!file.is_open()) {
            sgct::MessageHandler::printError(
                "CorrectionMesh: Failed to export '%s'", exportPath.c_str()
            );
            return;
        }

        file << std::fixed;
        file << std::setprecision(6);
        file << "# SGCT warping mesh\n";
        file << "# Number of vertices: " << buf.vertices.size() << "\n";

        // export vertices
        for (const sgct::core::correction::CorrectionMeshVertex& vertex : buf.vertices) {
            file << "v " << vertex.x << ' ' << vertex.y << " 0\n";
        }

        // export texture coords
        for (const sgct::core::correction::CorrectionMeshVertex& vertex : buf.vertices) {
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

        file.close();

        sgct::MessageHandler::printInfo(
            "Mesh '%s' exported successfully", exportPath.c_str()
        );
    }

} // namespace

namespace sgct::core {

CorrectionMesh::Format parseCorrectionMeshHint(const std::string& hintStr) {
    if (hintStr == "domeprojection") {
        return CorrectionMesh::Format::DomeProjection;
    }
    if (hintStr == "scalable") {
        return CorrectionMesh::Format::Scaleable;
    }
    if (hintStr == "sciss") {
        return CorrectionMesh::Format::Sciss;
    }
    if (hintStr == "simcad") {
        return CorrectionMesh::Format::SimCad;
    }
    if (hintStr == "skyskan") {
        return CorrectionMesh::Format::SkySkan;
    }
    if (hintStr == "mpcdi") {
        return CorrectionMesh::Format::Mpcdi;
    }
    if (!hintStr.empty()) {
        MessageHandler::printWarning("Unknown CorrectionMesh hint '%s'", hintStr.c_str());
    }
    return CorrectionMesh::Format::None;
}

CorrectionMesh::CorrectionMeshGeometry::~CorrectionMeshGeometry() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);
}

bool CorrectionMesh::readAndGenerateMesh(std::string path, Viewport& parent, Format hint)
{
    using namespace correction;

    // generate unwarped mask
    {
        Buffer buf = setupSimpleMesh(parent.getPosition(), parent.getSize());
        createMesh(_quadGeometry, buf);
    }
    
    // generate unwarped mesh for mask
    if (parent.hasBlendMaskTexture() || parent.hasBlackLevelMaskTexture()) {
        MessageHandler::printDebug("CorrectionMesh: Creating mask mesh");

        Buffer buf = setupMaskMesh(parent.getPosition(), parent.getSize());
        createMesh(_maskGeometry, buf);
    }

    // fallback if no mesh is provided
    if (path.empty()) {
        Buffer buf = setupSimpleMesh(parent.getPosition(), parent.getSize());
        createMesh(_warpGeometry, buf);
        return false;
    }
    
    try {
        Buffer buf;

        // find a suitable format
        if (path.find(".sgc") != std::string::npos) {
            buf = generateScissMesh(path, parent);
        }
        else if (path.find(".ol") != std::string::npos) {
            buf = generateScalableMesh(path, parent.getPosition(), parent.getSize());
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
            buf = generateDomeProjectionMesh(
                path,
                parent.getPosition(),
                parent.getSize()
            );
        }
        else if ((path.find(".data") != std::string::npos) &&
            (hint == Format::None || hint == Format::PaulBourke))
        {
            buf = generatePaulBourkeMesh(path, parent.getPosition(), parent.getSize());

            // force regeneration of dome render quad
            FisheyeProjection* fishPrj = dynamic_cast<FisheyeProjection*>(
                parent.getNonLinearProjection()
            );
            if (fishPrj) {
                fishPrj->setIgnoreAspectRatio(true);
                fishPrj->update(glm::ivec2(1.f, 1.f));
            }
        }
        else if ((path.find(".obj") != std::string::npos) &&
            (hint == Format::None || hint == Format::Obj))
        {
            buf = generateOBJMesh(path);
        }
        else if (path.find(".mpcdi") != std::string::npos) {
            buf = generateMpcdiMesh(parent);
        }
        else if ((path.find(".simcad") != std::string::npos) &&
            (hint == Format::None || hint == Format::SimCad))
        {
            buf = generateSimCADMesh(path, parent);
        }
        else {
            throw std::runtime_error("Could not find format");
        }

        createMesh(_warpGeometry, buf);

        MessageHandler::printDebug(
            "CorrectionMesh read successfully. Vertices=%u, Indices=%u",
            static_cast<int>(buf.vertices.size()), static_cast<int>(buf.indices.size())
        );

        if (Settings::instance()->getExportWarpingMeshes()) {
            const size_t found = path.find_last_of(".");
            std::string filename = path.substr(0, found) + "_export.obj";
            exportMesh(_warpGeometry.type, std::move(filename), buf);
        }

        return true;
    }
    catch (const std::runtime_error & e) {
        MessageHandler::printError("%s", e.what());
        Buffer buf = setupSimpleMesh(parent.getPosition(), parent.getSize());
        createMesh(_warpGeometry, buf);
        return false;
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
    glBufferData(
        GL_ARRAY_BUFFER,
        buffer.vertices.size() * sizeof(correction::CorrectionMeshVertex),
        buffer.vertices.data(),
        GL_STATIC_DRAW
    );

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(correction::CorrectionMeshVertex),
        reinterpret_cast<void*>(0)
    );

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(correction::CorrectionMeshVertex),
        reinterpret_cast<void*>(8)
    );

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(correction::CorrectionMeshVertex),
        reinterpret_cast<void*>(16)
    );

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

} // namespace sgct::core
