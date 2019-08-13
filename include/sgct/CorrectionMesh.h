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

/*!
    Helper class for reading and rendering a correction mesh.
    A correction mesh is used for warping and edge-blending.
 */
class CorrectionMesh {
public:
    enum MeshType {
        QUAD_MESH = 0,
        WARP_MESH,
        MASK_MESH,
        LAST_MESH
    };
    enum MeshHint {
        NO_HINT = 0,
        DOMEPROJECTION_HINT,
        SCALEABLE_HINT,
        SCISS_HINT,
        SIMCAD_HINT,
        SKYSKAN_HINT,
        PAULBOURKE_HINT,
        OBJ_HINT,
        MPCDI_HINT
    };
        
    CorrectionMesh();

    bool readAndGenerateMesh(std::string meshPath, Viewport& parent,
        MeshHint hint = NO_HINT);
    void render(const MeshType & mt);
    static MeshHint parseHint(const std::string& hintStr);
        
private:
    enum MeshFormat {
        NO_FMT = 0,
        DOMEPROJECTION_FMT,
        SCALEABLE_FMT,
        SCISS_FMT,
        SIMCAD_FMT,
        SKYSKAN_FMT,
        PAULBOURKE_FMT,
        OBJ_FMT,
        MPCDI_FMT
    };

    class CorrectionMeshGeometry {
    public:
        ~CorrectionMeshGeometry();

        GLenum mGeometryType = GL_TRIANGLE_STRIP;
        unsigned int mNumberOfVertices = 0;
        unsigned int mNumberOfIndices = 0;
        unsigned int mMeshData[3] = { 0, 0, 0 };
    };

    bool readAndGenerateDomeProjectionMesh(const std::string& meshPath, Viewport& parent);
    bool readAndGenerateScalableMesh(const std::string& meshPath, Viewport& parent);
    bool readAndGenerateScissMesh(const std::string& meshPath, Viewport& parent);
    bool readAndGenerateSimCADMesh(const std::string& meshPath, Viewport& parent);
    bool readAndGenerateSkySkanMesh(const std::string& meshPath, Viewport& parent);
    bool readAndGeneratePaulBourkeMesh(const std::string& meshPath, Viewport& parent);
    bool readAndGenerateOBJMesh(const std::string& meshPath, Viewport& parent);
    bool readAndGenerateMpcdiMesh(const std::string& meshPath, Viewport& parent);
    bool readMeshBuffer(float* dest, unsigned int& idx, char* src,
        size_t srcSize_bytes, int readSize_bytes);
    void setupSimpleMesh(CorrectionMeshGeometry& geomPtr, Viewport& parent);
    void setupMaskMesh(Viewport& parent, bool flipX, bool flipY);
    void createMesh(CorrectionMeshGeometry& geomPtr);
    void exportMesh(const std::string& exportMeshPath);
    void cleanUp();
        
    enum buffer {
        Vertex = 0,
        Index,
        Array
    };

    struct CorrectionMeshVertex {
        float x, y;    //Vertex 8
        float s, t;    //Texcoord0 8
        float r, g, b, a; //color 16

        //ATI performs better using sizes of power of two
    };

    CorrectionMeshVertex* mTempVertices = nullptr;
    unsigned int* mTempIndices = nullptr;
        
    CorrectionMeshGeometry mGeometries[3];
};
    
} // namespace sgct_core

#endif // __SGCT__CORRECTION_MESH__H__
