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
    
class Viewport;

/**
 * Helper class for reading and rendering a correction mesh. A correction mesh is used for
 * warping and edge-blending.
 */
class CorrectionMesh {
public:
    enum class Hint {
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
     * \param meshPath the path to the mesh data
     * \param meshHint a hint to pass to the parser selector
     * \param parent the pointer to parent viewport
     *
     * \return true if mesh found and loaded successfully
     */
    bool readAndGenerateMesh(std::string path, Viewport& parent, Hint hint = Hint::None);

    /// Render the final mesh where for mapping the frame buffer to the screen.
    void renderQuadMesh() const;

    /// Render the final mesh where for mapping the frame buffer to the screen.
    void renderWarpMesh() const;

    /// Render the final mesh where for mapping the frame buffer to the screen.
    void renderMaskMesh() const;

    /// Parse hint from string to enum.
    static Hint parseHint(const std::string& hintStr);
        
private:
    enum class Format {
        None,
        DomeProjection,
        Scaleable,
        Sciss,
        SimCad,
        SkySkan,
        PaulBourke,
        Obj,
        Mpcdi
    };

    struct CorrectionMeshGeometry {
        ~CorrectionMeshGeometry();

        GLenum geometryType = GL_TRIANGLE_STRIP;
        unsigned int nVertices = 0;
        unsigned int nIndices = 0;
        unsigned int vertexData = 0;
        unsigned int indexData = 0;
        unsigned int arrayData = 0;
    };

    struct CorrectionMeshVertex {
        float x, y;
        float s, t;
        float r, g, b, a;
    };

    struct Buffer {
        std::vector<CorrectionMeshVertex> vertices;
        std::vector<unsigned int> indices;
    };

    Buffer setupSimpleMesh(CorrectionMeshGeometry& geom, const Viewport& parent);
    Buffer setupMaskMesh(const Viewport& parent, bool flipX, bool flipY);
    bool generateDomeProjectionMesh(const std::string& meshPath, const Viewport& parent);
    bool generateScalableMesh(const std::string& meshPath, const Viewport& parent);
    bool generateScissMesh(const std::string& meshPath, Viewport& parent);
    bool generateSimCADMesh(const std::string& meshPath, const Viewport& parent);
    bool generateSkySkanMesh(const std::string& meshPath, Viewport& parent);
    bool generateOBJMesh(const std::string& meshPath);
    bool generateMpcdiMesh(const std::string& meshPath, const Viewport& parent);

    bool generatePaulBourkeMesh(const std::string& meshPath, const Viewport& parent);

    void createMesh(CorrectionMeshGeometry& geom,
        const std::vector<CorrectionMeshVertex>& vertices,
        const std::vector<unsigned int>& indices);

    void exportMesh(const std::string& exportMeshPath,
        const std::vector<CorrectionMeshVertex>& vertices,
        const std::vector<unsigned int>& indices);

    void render(const CorrectionMeshGeometry& mt) const;

    CorrectionMeshGeometry _quadGeometry;
    CorrectionMeshGeometry _warpGeometry;
    CorrectionMeshGeometry _maskGeometry;
};
    
} // namespace sgct::core

#endif // __SGCT__CORRECTION_MESH__H__
