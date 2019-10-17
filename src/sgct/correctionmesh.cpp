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
#include <sgct/correction/mpcdi.h>
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

namespace sgct::core {

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
    
    // transform to lowercase
    std::string pathLower(path);
    std::transform(
        path.begin(),
        path.end(),
        pathLower.begin(),
        [](char c) { return static_cast<char>(::tolower(c)); }
    );

    bool loadStatus = false;

    // find a suitable format
    if (pathLower.find(".sgc") != std::string::npos) {
        Buffer buf = generateScissMesh(path, parent);
        loadStatus = buf.isComplete;
        if (buf.isComplete) {
            createMesh(_warpGeometry, buf);

            MessageHandler::printDebug(
                "CorrectionMesh: Correction mesh read successfully. Vertices=%u, Indices=%u",
                static_cast<int>(buf.vertices.size()), static_cast<int>(buf.indices.size())
            );
            
            if (Settings::instance()->getExportWarpingMeshes()) {
                const size_t found = path.find_last_of(".");
                std::string filename = path.substr(0, found) + "_export.obj";
                exportMesh(_warpGeometry, std::move(filename), buf);
            }
        }
    }
    else if (pathLower.find(".ol") != std::string::npos) {
        Buffer buf = generateScalableMesh(path, parent.getPosition(), parent.getSize());
        loadStatus = buf.isComplete;
        if (buf.isComplete) {
            createMesh(_warpGeometry, buf);

            MessageHandler::printInfo(
                "CorrectionMesh: Mesh read successfully. Vertices=%u, Faces=%u\n",
                buf.vertices.size(), buf.indices.size()
            );

            if (Settings::instance()->getExportWarpingMeshes()) {
                const size_t found = path.find_last_of(".");
                std::string filename = path.substr(0, found) + "_export.obj";
                exportMesh(_warpGeometry, std::move(filename), buf);
            }
        }
    }
    else if (pathLower.find(".skyskan") != std::string::npos) {
        Buffer buf = generateSkySkanMesh(path, parent);
        loadStatus = buf.isComplete;
        if (buf.isComplete) {
            createMesh(_warpGeometry, buf);

            MessageHandler::printDebug(
                "CorrectionMesh: Mesh read successfully. Vertices=%u, Indices=%u",
                _warpGeometry.nVertices, _warpGeometry.nIndices
            );

            if (Settings::instance()->getExportWarpingMeshes()) {
                const size_t found = path.find_last_of(".");
                std::string filename = path.substr(0, found) + "_export.obj";
                exportMesh(_warpGeometry, std::move(filename), buf);
            }
        }
    }
    else if ((pathLower.find(".txt") != std::string::npos) &&
            (hint == Format::None || hint == Format::SkySkan))
    {
        Buffer buf = generateSkySkanMesh(path, parent);
        loadStatus = buf.isComplete;
        if (buf.isComplete) {
            createMesh(_warpGeometry, buf);

            MessageHandler::printDebug(
                "CorrectionMesh: Mesh read successfully. Vertices=%u, Indices=%u",
                _warpGeometry.nVertices, _warpGeometry.nIndices
            );

            if (Settings::instance()->getExportWarpingMeshes()) {
                const size_t found = path.find_last_of(".");
                std::string filename = path.substr(0, found) + "_export.obj";
                exportMesh(_warpGeometry, std::move(filename), buf);
            }
        }
    }
    else if ((pathLower.find(".csv") != std::string::npos) &&
            (hint == Format::None || hint == Format::DomeProjection))
    {
        Buffer buf = generateDomeProjectionMesh(
            path,
            parent.getPosition(),
            parent.getSize()
        );
        loadStatus = buf.isComplete;

        if (buf.isComplete) {
            createMesh(_warpGeometry, buf);

            MessageHandler::printDebug(
                "CorrectionMesh: Mesh read successfully. Vertices=%u, Indices=%u",
                _warpGeometry.nVertices, _warpGeometry.nIndices
            );
            
            if (Settings::instance()->getExportWarpingMeshes()) {
                const size_t found = path.find_last_of(".");
                std::string filename = path.substr(0, found) + "_export.obj";
                exportMesh(_warpGeometry, std::move(filename), buf);
            }
        }
    }
    else if ((pathLower.find(".data") != std::string::npos) &&
            (hint == Format::None || hint == Format::PaulBourke))
    {
        Buffer buf = generatePaulBourkeMesh(path, parent.getPosition(), parent.getSize());
        loadStatus = buf.isComplete;
        if (buf.isComplete) {
            createMesh(_warpGeometry, buf);

            // force regeneration of dome render quad
            FisheyeProjection* fishPrj = dynamic_cast<FisheyeProjection*>(
                parent.getNonLinearProjection()
            );
            if (fishPrj) {
                fishPrj->setIgnoreAspectRatio(true);
                fishPrj->update(glm::ivec2(1.f, 1.f));
            }

            MessageHandler::printDebug(
                "CorrectionMesh: Mesh read successfully. Vertices=%u, Indices=%u",
                _warpGeometry.nVertices, _warpGeometry.nIndices
            );

            if (Settings::instance()->getExportWarpingMeshes()) {
                const size_t found = path.find_last_of(".");
                std::string filename = path.substr(0, found) + "_export.obj";
                exportMesh(_warpGeometry, std::move(filename), buf);
            }
        }
    }
    else if ((pathLower.find(".obj") != std::string::npos) &&
            (hint == Format::None || hint == Format::Obj))
    {
        Buffer buf = generateOBJMesh(path);
        loadStatus = buf.isComplete;
        if (buf.isComplete) {
            createMesh(_warpGeometry, buf);

            MessageHandler::printDebug(
                "CorrectionMesh: Mesh read successfully. Vertices=%u, Indices=%u",
                _warpGeometry.nVertices, _warpGeometry.nIndices
            );

            if (Settings::instance()->getExportWarpingMeshes()) {
                const size_t found = path.find_last_of(".");
                std::string filename = path.substr(0, found) + "_export.obj";
                exportMesh(_warpGeometry, std::move(filename), buf);
            }
        }
    }
    else if (pathLower.find(".mpcdi") != std::string::npos) {
        Buffer buf = generateMpcdiMesh("", parent);
        loadStatus = buf.isComplete;
        if (buf.isComplete) {
            createMesh(_warpGeometry, buf);

            MessageHandler::printDebug(
                "CorrectionMesh: Mpcdi Correction mesh read successfully. "
                "Vertices=%u, Indices=%u", _warpGeometry.nVertices, _warpGeometry.nIndices
            );

            if (Settings::instance()->getExportWarpingMeshes()) {
                const size_t found = path.find_last_of(".");
                std::string filename = path.substr(0, found) + "_export.obj";
                exportMesh(_warpGeometry, std::move(filename), buf);
            }
        }

    }
    else if ((pathLower.find(".simcad") != std::string::npos) &&
             (hint == Format::None || hint == Format::SimCad))
    {
        Buffer buf = generateSimCADMesh(path, parent);
        loadStatus = buf.isComplete;
        if (buf.isComplete) {
            createMesh(_warpGeometry, buf);

            MessageHandler::printDebug(
                "CorrectionMesh: Mesh read successfully. Vertices=%u, Indices=%u",
                _warpGeometry.nVertices, _warpGeometry.nIndices
            );

            if (Settings::instance()->getExportWarpingMeshes()) {
                const size_t found = path.find_last_of(".");
                std::string filename = path.substr(0, found) + "_export.obj";
                exportMesh(_warpGeometry, std::move(filename), buf);
            }
        }
    }

    if (!loadStatus) {
        MessageHandler::printError(
            "CorrectionMesh error: Loading mesh '%s' failed", path.c_str()
        );

        Buffer buf = setupSimpleMesh(parent.getPosition(), parent.getSize());
        createMesh(_warpGeometry, buf);
    }

    return loadStatus;
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

CorrectionMesh::Format CorrectionMesh::parseHint(const std::string& hintStr) {
    if (hintStr.empty()) {
        return Format::None;
    }

    // transform to lowercase
    std::string str(hintStr);
    std::transform(
        str.begin(),
        str.end(),
        str.begin(),
        [](char c) { return static_cast<char>(::tolower(c)); }
    );

    CorrectionMesh::Format hint = Format::None;
    if (str == "domeprojection") {
        hint = Format::DomeProjection;
    }
    else if (str == "scalable") {
        hint = Format::Scaleable;
    }
    else if (str == "sciss") {
        hint = Format::Sciss;
    }
    else if (str == "simcad") {
        hint = Format::SimCad;
    }
    else if (str == "skyskan") {
        hint = Format::SkySkan;
    }
    else if (str == "mpcdi") {
        hint = Format::Mpcdi;
    }
    else {
        MessageHandler::printWarning(
            "CorrectionMesh: hint '%s' is invalid", hintStr.c_str()
        );
    }

    return hint;
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

void CorrectionMesh::exportMesh(const CorrectionMeshGeometry& geometry,
                                const std::string& exportPath,
                                const correction::Buffer& buf)
{
    if (geometry.type != GL_TRIANGLES && geometry.type != GL_TRIANGLE_STRIP) {
        MessageHandler::printError(
            "CorrectionMesh error: Failed to export '%s'. Geometry type is not supported",
            exportPath.c_str()
        );
        return;
    }
    
    std::ofstream file(exportPath, std::ios::out);
    if (!file.is_open()) {
        MessageHandler::printError(
            "CorrectionMesh error: Failed to export '%s'", exportPath.c_str()
        );
        return;
    }

    file << std::fixed;
    file << std::setprecision(6);
    file << "# SGCT warping mesh\n";
    file << "# Number of vertices: " << geometry.nVertices << "\n";

    // export vertices
    for (unsigned int i = 0; i < geometry.nVertices; i++) {
        file << "v " << buf.vertices[i].x << ' ' << buf.vertices[i].y << " 0\n";
    }

    // export texture coords
    for (unsigned int i = 0; i < geometry.nVertices; i++) {
        file << "vt " << buf.vertices[i].s << ' ' << buf.vertices[i].t << " 0\n";
    }

    // export generated normals
    for (unsigned int i = 0; i < geometry.nVertices; i++) {
        file << "vn 0 0 1\n";
    }

    file << "# Number of faces: " << geometry.nIndices / 3 << '\n';

    // export face indices
    if (geometry.type == GL_TRIANGLES) {
        for (unsigned int i = 0; i < geometry.nIndices; i += 3) {
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

        for (unsigned int i = 2; i < geometry.nIndices; i++) {
            file << "f " << buf.indices[i] + 1 << '/' << buf.indices[i] + 1 << '/'
                 << buf.indices[i] + 1 << ' ';
            file << buf.indices[i - 1] + 1 << '/' << buf.indices[i - 1] + 1 << '/'
                 << buf.indices[i - 1] + 1 << ' ';
            file << buf.indices[i - 2] + 1 << '/' << buf.indices[i - 2] + 1 << '/'
                 << buf.indices[i - 2] + 1 << '\n';
        }
    }

    file.close();

    MessageHandler::printInfo(
        "CorrectionMesh: Mesh '%s' exported successfully", exportPath.c_str()
    );
}

} // namespace sgct::core
