/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _NON_LINEAR_PROJECTION_H
#define _NON_LINEAR_PROJECTION_H

#include "BaseViewport.h"
#include "OffScreenBuffer.h"
#include "ShaderProgram.h"
#include <string>
#include <glm/glm.hpp>

namespace sgct_core
{
	/*!
	Base class for non linear projections
	*/
	class NonLinearProjection
	{
	public:
		enum InterpolationMode { Linear, Cubic };

		NonLinearProjection();
		virtual ~NonLinearProjection();

		void init(int internalTextureFormat, unsigned int textureFormat, unsigned int textureType, int samples=1);

		virtual void render() = 0;
		virtual void renderCubemap(std::size_t * subViewPortIndex) = 0;
		virtual void update(float width, float height) = 0;

		void updateFrustums(const Frustum::FrustumMode &frustumMode, const float & near_clipping_plane, const float & far_clipping_plane);
		void setCubemapResolution(int res);
		void setCubemapResolution(std::string quality);
		void setInterpolationMode(InterpolationMode im);
		void setUseDepthTransformation(bool state);
		void setStereo(bool state);
		void setClearColor(float red, float green, float blue, float alpha = 1.0f);
		void setClearColor(glm::vec4 color);
		void setAlpha(float alpha);
		void setPreferedMonoFrustumMode(Frustum::FrustumMode fm);

		const int & getCubemapResolution() const;
		const InterpolationMode & getInterpolationMode() const;

		inline void bindShaderProgram() const { mShader.bind(); }
		inline void bindDepthCorrectionShaderProgram() const { mDepthCorrectionShader.bind(); }

		inline BaseViewport * getSubViewportPtr(std::size_t index) { return &mSubViewports[index]; }
		inline OffScreenBuffer * getOffScreenBuffer() { return mCubeMapFBO_Ptr; }
		inline const int * getViewportCoords() { return mVpCoords; }

	protected:
		enum TextureIndex { CubeMapColor, CubeMapDepth, CubeMapNormals, CubeMapPositions, ColorSwap, DepthSwap, LastIndex };

		void initTextures();
		void initFBO();
		void initVBO();
		virtual void initViewports() = 0;
		virtual void initShaders() = 0;

		void setupViewport(const std::size_t & face);
		void generateMap(TextureIndex ti, int internalFormat, unsigned int format, unsigned int type);
		void generateCubeMap(TextureIndex ti, int internalFormat, unsigned int format, unsigned int type);

		int getCubemapRes(std::string & quality);
		int mCubemapResolution;

		bool mUseDepthTransformation;
		bool mStereo;

		BaseViewport mSubViewports[6]; //cubemap
		InterpolationMode mInterpolationMode;
		Frustum::FrustumMode mPreferedMonoFrustumMode;

		//opengl data
		unsigned int mTextures[LastIndex];
		int mTextureInternalFormat;
		unsigned int mTextureFormat;
		unsigned int mTextureType;
		int mSamples;
		unsigned int mVBO;
		unsigned int mVAO;
		float mVerts[20];
		int mVpCoords[4];

		sgct::ShaderProgram mShader, mDepthCorrectionShader;
		OffScreenBuffer * mCubeMapFBO_Ptr;
		glm::vec4 mClearColor;
	};

}

#endif
