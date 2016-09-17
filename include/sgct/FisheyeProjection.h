/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _FISHEYE_PROJECTION_H
#define _FISHEYE_PROJECTION_H

#include "NonLinearProjection.h"
#include <glm/glm.hpp>

namespace sgct_core
{
	/*!
	This class manages and renders non linear fisheye projections
	*/
	class FisheyeProjection : public NonLinearProjection
	{
	public:
		enum FisheyeMethod { FourFaceCube = 0, FiveFaceCube, SixFaceCube };
		enum FisheyeCropSide { CropLeft = 0, CropRight, CropBottom, CropTop };

		FisheyeProjection();
		~FisheyeProjection();

		void update(float width, float height);
		void render();
		void renderCubemap(std::size_t * subViewPortIndex);

		void setDomeDiameter(float size);
		void setTilt(float angle);
		void setFOV(float angle);
		void setRenderingMethod(FisheyeMethod method);
		void setCropFactors(float left, float right, float bottom, float top);
		void setOffset(const glm::vec3 & offset);
		void setOffset(float x, float y, float z = 0.0f);
		void setBaseOffset(const glm::vec3 & offset);
		void setIgnoreAspectRatio(bool state);

		glm::vec3 getOffset() const;

	private:
		void initViewports();
		void initShaders();
		void updateGeomerty(const float & width, const float & height);

		void drawCubeFace(const std::size_t & face);
		void blitCubeFace(const int & face);
		void attachTextures(const int & face);
		void renderInternal();
		void renderInternalFixedPipeline();
		void renderCubemapInternal(std::size_t * subViewPortIndex);
		void renderCubemapInternalFixedPipeline(std::size_t * subViewPortIndex);

		void(FisheyeProjection::*mInternalRenderFn)(void);
		void(FisheyeProjection::*mInternalRenderCubemapFn)(std::size_t *);

		float mFOV;
		float mTilt;
		float mDiameter;
		float mCropFactors[4];

		bool mOffAxis;
		bool mIgnoreAspectRatio;
		
		glm::vec3 mOffset;
		glm::vec3 mBaseOffset;
		glm::vec3 mTotalOffset;

		FisheyeMethod mMethod;

		//shader locations
		int mCubemapLoc, mDepthCubemapLoc, mNormalCubemapLoc, mPositionCubemapLoc, mHalfFovLoc, mOffsetLoc, mSwapColorLoc, mSwapDepthLoc, mSwapNearLoc, mSwapFarLoc;

	};

}

#endif
