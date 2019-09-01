/*************************************************************************
 Copyright (c) 2012-2015 Miroslav Andel
 All rights reserved.
 
 For conditions of distribution and use, see copyright notice in sgct.h
 *************************************************************************/

#ifndef __SGCT__CORRECTION_MESH__H__
#define __SGCT__CORRECTION_MESH__H__

#include <sgct/ogl_headers.h>
#include <string>
#include <vector>

namespace sgct_core {
    
class Viewport;

/**
 * Helper class for reading and rendering a correction mesh. A correction mesh is used for
 * warping and edge-blending.
 */
class CorrectionMesh {
public:
    enum class MeshHint {
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
    bool readAndGenerateMesh(std::string meshPath, Viewport& parent,
        MeshHint hint = MeshHint::None);

    /// Render the final mesh where for mapping the frame buffer to the screen.
    void renderQuadMesh() const;

    /// Render the final mesh where for mapping the frame buffer to the screen.
    void renderWarpMesh() const;

    /// Render the final mesh where for mapping the frame buffer to the screen.
    void renderMaskMesh() const;

    /// Parse hint from string to enum.
    static MeshHint parseHint(const std::string& hintStr);
        
private:
    enum class MeshFormat {
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

        GLenum mGeometryType = GL_TRIANGLE_STRIP;
        unsigned int mNumberOfVertices = 0;
        unsigned int mNumberOfIndices = 0;
        unsigned int mVertexMeshData = 0;
        unsigned int mIndexMeshData = 0;
        unsigned int mArrayMeshData = 0;
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

    Buffer setupSimpleMesh(CorrectionMeshGeometry& geomPtr, const Viewport& parent);
    Buffer setupMaskMesh(const Viewport& parent, bool flipX, bool flipY);
    bool generateDomeProjectionMesh(const std::string& meshPath, const Viewport& parent);
    bool generateScalableMesh(const std::string& meshPath, const Viewport& parent);
    bool generateScissMesh(const std::string& meshPath, Viewport& parent);
    bool generateSimCADMesh(const std::string& meshPath, const Viewport& parent);
    bool generateSkySkanMesh(const std::string& meshPath, Viewport& parent);
    bool generateOBJMesh(const std::string& meshPath, const Viewport& parent);
    bool generateMpcdiMesh(const std::string& meshPath, const Viewport& parent);

    bool generatePaulBourkeMesh(const std::string& meshPath, const Viewport& parent);


    void createMesh(CorrectionMeshGeometry& geomPtr,
        const std::vector<CorrectionMeshVertex>& vertices,
        const std::vector<unsigned int>& indices);

    void exportMesh(const std::string& exportMeshPath,
        const std::vector<CorrectionMeshVertex>& vertices,
        const std::vector<unsigned int>& indices);

    void render(const CorrectionMeshGeometry& mt) const;

    CorrectionMeshGeometry mQuadGeometry;
    CorrectionMeshGeometry mWarpGeometry;
    CorrectionMeshGeometry mMaskGeometry;
};
    
} // namespace sgct_core

#endif // __SGCT__CORRECTION_MESH__H__
