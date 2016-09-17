/*************************************************************************
 Copyright (c) 2012-2015 Miroslav Andel
 All rights reserved.
 
 For conditions of distribution and use, see copyright notice in sgct.h
 *************************************************************************/

#ifndef _CORRECTION_MESH_H_
#define _CORRECTION_MESH_H_

#include "sgct/ogl_headers.h"

namespace sgct_core
{
    
    struct CorrectionMeshVertex
    {
        float x, y;	//Vertex 8
        float s, t;	//Texcoord0 8
        float r, g, b, a; //color 16
        
        //ATI performs better using sizes of power of two
    };
    
    class CorrectionMeshGeometry
    {
    public:
        CorrectionMeshGeometry();
        ~CorrectionMeshGeometry();
        
        GLenum mGeometryType;
        unsigned int mNumberOfVertices;
        unsigned int mNumberOfIndices;
        unsigned int mMeshData[3];
    };
    
    class Viewport;
    
    /*!
     Helper class for reading and rendering a correction mesh.
     A correction mesh is used for warping and edge-blending.
     */
    class CorrectionMesh
    {
    public:
        enum MeshType { QUAD_MESH = 0, WARP_MESH, MASK_MESH };
		enum MeshHint { NO_HINT = 0, DOMEPROJECTION_HINT, SCALEABLE_HINT, SCISS_HINT, SKYSKAN_HINT, PAULBOURKE_HINT};
        
        CorrectionMesh();
        ~CorrectionMesh();
        bool readAndGenerateMesh(std::string meshPath, Viewport * parent, MeshHint hint = NO_HINT);
        void render(const MeshType & mt);
		static MeshHint parseHint(const std::string & hintStr);
        
    private:
		enum MeshFormat { NO_FMT = 0, DOMEPROJECTION_FMT, SCALEABLE_FMT, SCISS_FMT, SKYSKAN_FMT, PAULBOURKE_FMT};

		bool readAndGenerateDomeProjectionMesh(const std::string & meshPath, Viewport * parent);
		bool readAndGenerateScalableMesh(const std::string & meshPath, Viewport * parent);
		bool readAndGenerateScissMesh(const std::string & meshPath, Viewport * parent);
		bool readAndGenerateSkySkanMesh(const std::string & meshPath, Viewport * parent);
		bool readAndGeneratePaulBourkeMesh(const std::string & meshPath, Viewport * parent);
		void setupSimpleMesh(CorrectionMeshGeometry * geomPtr, Viewport * parent);
		void setupMaskMesh(Viewport * parent, bool flip_x, bool flip_y);
        void createMesh(CorrectionMeshGeometry * geomPtr);
        void cleanUp();
		inline void clamp(float & val, const float max, const float min);
        
        enum buffer { Vertex = 0, Index, Array };
        
        CorrectionMeshVertex * mTempVertices;
        unsigned int * mTempIndices;
        
        CorrectionMeshGeometry mGeometries[3];
    };
    
} //sgct_core

#endif