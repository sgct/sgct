/*************************************************************************
 Copyright (c) 2012-2015 Miroslav Andel
 All rights reserved.
 
 For conditions of distribution and use, see copyright notice in sgct.h
 *************************************************************************/

#ifndef __SGCT__CORRECTION_MESH__H__
#define __SGCT__CORRECTION_MESH__H__

#include <sgct/ogl_headers.h>
#include <string>

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

    /**
     * Parse data from domeprojection's camera based calibration system.
     * Domeprojection.com
     */
    bool readAndGenerateDomeProjectionMesh(const std::string& meshPath, Viewport& parent);
    bool readAndGenerateScalableMesh(const std::string& meshPath, Viewport& parent);
    bool readAndGenerateScissMesh(const std::string& meshPath, Viewport& parent);
    bool readAndGenerateSimCADMesh(const std::string& meshPath, Viewport& parent);
    bool readAndGenerateSkySkanMesh(const std::string& meshPath, Viewport& parent);
    bool readAndGeneratePaulBourkeMesh(const std::string& meshPath, Viewport& parent);
    bool readAndGenerateOBJMesh(const std::string& meshPath, Viewport& parent);
    bool readAndGenerateMpcdiMesh(const std::string& meshPath, Viewport& parent);
    void setupSimpleMesh(CorrectionMeshGeometry& geomPtr, Viewport& parent);
    void setupMaskMesh(Viewport& parent, bool flipX, bool flipY);

    struct CorrectionMeshVertex {
        float x, y;
        float s, t;
        float r, g, b, a;
    };
    void createMesh(CorrectionMeshGeometry& geomPtr, CorrectionMeshVertex* vertices,
        unsigned int* indices);
    void exportMesh(const std::string& exportMeshPath);
    void cleanUp();
        
    void render(const CorrectionMeshGeometry& mt) const;

    CorrectionMeshVertex* mTempVertices = nullptr;
    unsigned int* mTempIndices = nullptr;
       
    struct {
        CorrectionMeshGeometry quad;
        CorrectionMeshGeometry warp;
        CorrectionMeshGeometry mask;
    } mGeometries;
};
    
} // namespace sgct_core

#endif // __SGCT__CORRECTION_MESH__H__
