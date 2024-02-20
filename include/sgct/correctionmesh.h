/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CORRECTION_MESH__H__
#define __SGCT__CORRECTION_MESH__H__

#include <sgct/sgctexports.h>
#include <string>
#include <vector>

namespace sgct {

class BaseViewport;

namespace correction { struct Buffer; }

/**
 * Helper class for reading and rendering a correction mesh. A correction mesh is used for
 * warping and edge-blending.
 */
class SGCT_EXPORT CorrectionMesh {
public:
    /**
     * This function finds a suitable parser for warping meshes and loads them.
     *
     * \param path The path to the mesh data
     * \param parent The pointer to parent viewport
     * \param needsMaskGeometry If `true`, a separate geometry to applying blend masks is
     *        loaded
     *
     * \throw std::runtime_error if mesh was not loaded successfully
     */
    void loadMesh(std::string path, BaseViewport& parent,
        bool needsMaskGeometry = false, bool textureRenderMode = false);

    /**
     * Render the final mesh where for mapping the frame buffer to the screen.
     */
    void renderQuadMesh() const;

    /**
     * Render the final mesh where for mapping the frame buffer to the screen.
     */
    void renderWarpMesh() const;

    /**
     * Render the final mesh where for mapping the frame buffer to the screen.
     */
    void renderMaskMesh() const;

private:
    struct CorrectionMeshGeometry {
        ~CorrectionMeshGeometry();

        unsigned int vao = 0;
        unsigned int vbo = 0;
        unsigned int ibo = 0;
        unsigned int nVertices = 0;
        unsigned int nIndices = 0;
        unsigned int type = 0x0005; // = GL_TRIANGLE_STRIP;
    };

    void createMesh(CorrectionMeshGeometry& geom, const correction::Buffer& buffer);

    CorrectionMeshGeometry _quadGeometry;
    CorrectionMeshGeometry _warpGeometry;
    CorrectionMeshGeometry _maskGeometry;
};

} // namespace sgct

#endif // __SGCT__CORRECTION_MESH__H__
