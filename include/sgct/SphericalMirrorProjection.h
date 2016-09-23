/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _SPHERICAL_MIRROR_PROJECTION_H
#define _SPHERICAL_MIRROR_PROJECTION_H

#include "NonLinearProjection.h"
#include "CorrectionMesh.h"
#include <glm/glm.hpp>

namespace sgct_core
{
	/*!
	This class manages and renders non linear fisheye projections
	*/
	class SphericalMirrorProjection : public NonLinearProjection
	{
	public:
		enum MeshFace { BOTTOM_MESH = 0, LEFT_MESH, RIGHT_MESH, TOP_MESH, LAST_MESH };

		SphericalMirrorProjection();
		~SphericalMirrorProjection();

		void update(float width, float height);
		void render();
		void renderCubemap(std::size_t * subViewPortIndex);

		void setTilt(float angle);
		void setMeshPath(MeshFace mf, const char * str);

	private:
		void initTextures();
		void initVBO();
		void initViewports();
		void initShaders();
		
		void drawCubeFace(const std::size_t & face);
		void blitCubeFace(TextureIndex &ti);
		void attachTextures(TextureIndex &ti);
		void renderInternal();
		void renderInternalFixedPipeline();
		void renderCubemapInternal(std::size_t * subViewPortIndex);
		void renderCubemapInternalFixedPipeline(std::size_t * subViewPortIndex);

		void(SphericalMirrorProjection::*mInternalRenderFn)(void);
		void(SphericalMirrorProjection::*mInternalRenderCubemapFn)(std::size_t *);

		float mTilt;
		float mDiameter;
		
		//mesh data
		CorrectionMesh mMeshes[4];
		std::string mMeshPaths[4];

		//shader locations
		int mTexLoc;
		int mMatrixLoc;
	};

}

#endif
