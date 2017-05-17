/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _SPOUTOUTPUT_PROJECTION_H
#define _SPOUTOUTPUT_PROJECTION_H

#include "NonLinearProjection.h"
#include <glm/glm.hpp>

namespace sgct_core
{
	/*!
	This class manages and renders non linear fisheye projections
	*/
	class SpoutOutputProjection : public NonLinearProjection
	{
	public:
		SpoutOutputProjection();
		~SpoutOutputProjection();

		void update(float width, float height);
		void render();
		void renderCubemap(std::size_t * subViewPortIndex);

	private:
		void initTextures();
		void initViewports();
		void initShaders();

		void drawCubeFace(const std::size_t & face);
		void blitCubeFace(const int & face);
		void attachTextures(const int & face);
		void renderInternal();
		void renderInternalFixedPipeline();
		void renderCubemapInternal(std::size_t * subViewPortIndex);
		void renderCubemapInternalFixedPipeline(std::size_t * subViewPortIndex);

		void(SpoutOutputProjection::*mInternalRenderFn)(void);
		void(SpoutOutputProjection::*mInternalRenderCubemapFn)(std::size_t *);

		//shader locations
		int mSwapColorLoc, mSwapDepthLoc, mSwapNearLoc, mSwapFarLoc;
		/*
		// for 4 faces
		const std::string spoutCubeMapFaceName[6] = {
			"Right",
			"Bottom",
			"Top",
			"Left",
			"zLeft",
			"zRight",
		};
		*/
		// for 6 faces
		const std::string spoutCubeMapFaceName[6] = {
			"Right",
			"zLeft",
			"Bottom",
			"Top",
			"Left",
			"zRight",
		};
		void *handle[6];
		GLuint spoutTexture[6];
	};

}

#endif
