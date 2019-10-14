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
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace sgct::core {

class Viewport;

/**
 * Helper class for reading and rendering a correction mesh. A correction mesh is used for
 * warping and edge-blending.
 */
class CorrectionMesh {
public:
    struct CorrectionMeshGeometry {
        ~CorrectionMeshGeometry();

        unsigned int vao = 0;
        unsigned int vbo = 0;
        unsigned int ibo = 0;
        unsigned int nVertices = 0;
        unsigned int nIndices = 0;
        GLenum geometryType = GL_TRIANGLE_STRIP;
    };

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
     * This function finds a suitible parser for warping meshes and loads them into
     * memory.
     *
     * \param path the path to the mesh data
     * \param hint a hint to pass to the parser selector
     * \param parent the pointer to parent viewport
     *
     * \return true if mesh found and loaded successfully
     */
    bool readAndGenerateMesh(std::string path, Viewport& parent,
        Format hint = Format::None);

    /// Render the final mesh where for mapping the frame buffer to the screen.
    void renderQuadMesh() const;

    /// Render the final mesh where for mapping the frame buffer to the screen.
    void renderWarpMesh() const;

    /// Render the final mesh where for mapping the frame buffer to the screen.
    void renderMaskMesh() const;

    /// Parse hint from string to enum.
    static Format parseHint(const std::string& hintStr);

private:
    CorrectionMeshGeometry _quadGeometry;
    CorrectionMeshGeometry _warpGeometry;
    CorrectionMeshGeometry _maskGeometry;
};
    
} // namespace sgct::core

#endif // __SGCT__CORRECTION_MESH__H__
