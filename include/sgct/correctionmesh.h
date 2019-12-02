/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__CORRECTION_MESH__H__
#define __SGCT__CORRECTION_MESH__H__

#include <sgct/ogl_headers.h>
#include <string>
#include <vector>

namespace sgct::core {

class BaseViewport;

namespace correction { struct Buffer; }

/**
 * Helper class for reading and rendering a correction mesh. A correction mesh is used for
 * warping and edge-blending.
 */
class CorrectionMesh {
public:
    enum class Format {
        None = 0,
        DomeProjection,
        Scaleable,
        Sciss,
        SimCad,
        SkySkan,
        PaulBourke,
        Obj,
        Mpcdi
    };

    /**
     * This function finds a suitable parser for warping meshes and loads them.
     *
     * \param path the path to the mesh data
     * \param parent the pointer to parent viewport
     * \param hint a hint to pass to the parser selector
     * \param needsMaskGeometry If true, a separate geometry to applying blend masks is
     *        loaded
     * \throw std::runtime_error if mesh was not loaded successfully
     */
    void loadMesh(std::string path, BaseViewport& parent, Format hint = Format::None,
        bool needsMaskGeometry = false);

    /// Render the final mesh where for mapping the frame buffer to the screen.
    void renderQuadMesh() const;

    /// Render the final mesh where for mapping the frame buffer to the screen.
    void renderWarpMesh() const;

    /// Render the final mesh where for mapping the frame buffer to the screen.
    void renderMaskMesh() const;

private:
    struct CorrectionMeshGeometry {
        ~CorrectionMeshGeometry();

        unsigned int vao = 0;
        unsigned int vbo = 0;
        unsigned int ibo = 0;
        unsigned int nVertices = 0;
        unsigned int nIndices = 0;
        GLenum type = GL_TRIANGLE_STRIP;
    };

    void createMesh(CorrectionMeshGeometry& geom, const correction::Buffer& buffer);

    CorrectionMeshGeometry _quadGeometry;
    CorrectionMeshGeometry _warpGeometry;
    CorrectionMeshGeometry _maskGeometry;
};

/// Parse hint from string to enum.
CorrectionMesh::Format parseCorrectionMeshHint(const std::string& hintStr);

} // namespace sgct::core

#endif // __SGCT__CORRECTION_MESH__H__
