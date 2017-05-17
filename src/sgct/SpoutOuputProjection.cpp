/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/SpoutOutputProjection.h>
#include <sgct/SGCTSettings.h>
#include <sgct/Engine.h>
#include <sgct/MessageHandler.h>
#include <sgct/shaders/SGCTInternalFisheyeShaders.h>
#include <sgct/shaders/SGCTInternalFisheyeShaders_modern.h>
#include <sgct/helpers/SGCTStringFunctions.h>
#include <sstream>
#include <algorithm>

#include <spout/SpoutLibrary.h>

//#define DebugCubemap

sgct_core::SpoutOutputProjection::SpoutOutputProjection()
{
	mInternalRenderFn = &SpoutOutputProjection::renderInternalFixedPipeline;
	mInternalRenderCubemapFn = &SpoutOutputProjection::renderCubemapInternalFixedPipeline;

	for (size_t i = 0; i < 6; i++)
	{
		handle[i] = nullptr;
		spoutTexture[i] = GL_FALSE;
	}

	mSwapColorLoc = -1;
	mSwapDepthLoc = -1;
	mSwapNearLoc = -1;
	mSwapFarLoc = -1;
}

sgct_core::SpoutOutputProjection::~SpoutOutputProjection()
{
	for (size_t i = 0; i < 6; i++)
	{
		if (handle[i]) 
		{
#ifdef SGCT_HAS_SPOUT
			((SPOUTHANDLE)handle[i])->ReleaseSender();
			((SPOUTHANDLE)handle[i])->Release();
#endif
			handle[i] = nullptr;
		}
		if (spoutTexture[i] != GL_FALSE)
		{
			glDeleteTextures(1, &mTextures[i]);
			spoutTexture[i] = GL_FALSE;
		}
	}
}

/*!
Update projection when aspect ratio changes for the viewport.
*/
void sgct_core::SpoutOutputProjection::update(float, float)
{
}

/*!
Render the non linear projection to currently bounded FBO
*/
void sgct_core::SpoutOutputProjection::render()
{
	(this->*mInternalRenderFn)();
}

/*!
Render the enabled faces of the cubemap
*/
void sgct_core::SpoutOutputProjection::renderCubemap(std::size_t * subViewPortIndex)
{
	(this->*mInternalRenderCubemapFn)(subViewPortIndex);
}



void sgct_core::SpoutOutputProjection::initTextures() {
	sgct_core::NonLinearProjection::initTextures();
	
	if (sgct::Engine::instance()->getRunMode() <= sgct::Engine::OpenGL_Compablity_Profile)
	{
		glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT);
		glEnable(GL_TEXTURE_2D);
	}
	
	for (size_t i = 0; i < 6; ++i) {
#ifdef SGCT_HAS_SPOUT
		handle[i] = GetSpout();
		if (handle[i]) ((SPOUTHANDLE)handle[i])->CreateSender(spoutCubeMapFaceName[i].c_str(), mCubemapResolution, mCubemapResolution);
#endif

		glGenTextures(1, &spoutTexture[i]);
		glBindTexture(GL_TEXTURE_2D, spoutTexture[i]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, mTextureInternalFormat, mCubemapResolution, mCubemapResolution, 0, mTextureFormat, mTextureType, NULL);
	}

	if (sgct::Engine::instance()->getRunMode() <= sgct::Engine::OpenGL_Compablity_Profile)
		glPopAttrib();

	if (sgct::Engine::instance()->isOGLPipelineFixed())
	{
		mInternalRenderFn = &SpoutOutputProjection::renderInternalFixedPipeline;
		mInternalRenderCubemapFn = &SpoutOutputProjection::renderCubemapInternalFixedPipeline;
	}
	else
	{
		mInternalRenderFn = &SpoutOutputProjection::renderInternal;
		mInternalRenderCubemapFn = &SpoutOutputProjection::renderCubemapInternal;
	}
}


void sgct_core::SpoutOutputProjection::initViewports()
{
	enum cubeFaces { Pos_X = 0, Neg_X, Pos_Y, Neg_Y, Pos_Z, Neg_Z };

	//radius is needed to calculate the distance to all view planes
	float radius = 1.0f;

	//setup base viewport that will be rotated to create the other cubemap views
	//+Z face
	const glm::vec4 lowerLeftBase(-radius, -radius, radius, 1.0f);
	const glm::vec4 upperLeftBase(-radius, radius, radius, 1.0f);
	const glm::vec4 upperRightBase(radius, radius, radius, 1.0f);

	glm::mat4 rollRot = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	//add viewports
	for (unsigned int i = 0; i<6; i++)
	{
		std::stringstream ss;
		ss << "SpoutOutput " << i;
		mSubViewports[i].setName(ss.str());

		glm::vec4 lowerLeft = lowerLeftBase;
		glm::vec4 upperLeft = upperLeftBase;
		glm::vec4 upperRight = upperRightBase;

		glm::mat4 rotMat(1.0f);

		/*
		Rotate and clamp the halv height viewports
		*/
		switch (i)
		{
		case Pos_X: //+X face
		{
			rotMat = glm::rotate(rollRot, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			upperRight.x = radius;
			mSubViewports[i].setSize(1.0f, 1.0f);
		}
		break;

		case Neg_X: //-X face
		{
			rotMat = glm::rotate(rollRot, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			lowerLeft.x = -radius;
			upperLeft.x = -radius;
			mSubViewports[i].setPos(0.0f, 0.0f);
			mSubViewports[i].setSize(1.0f, 1.0f);
		}
		break;

		case Pos_Y: //+Y face
		{
			rotMat = glm::rotate(rollRot, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			lowerLeft.y = -radius;
			mSubViewports[i].setPos(0.0f, 0.0f);
			mSubViewports[i].setSize(1.0f, 1.0f);
		}
		break;

		case Neg_Y: //-Y face
		{
			rotMat = glm::rotate(rollRot, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			upperLeft.y = radius;
			upperRight.y = radius;
			mSubViewports[i].setSize(1.0f, 1.0f);
		}
		break;

		case Pos_Z: //+Z face
			rotMat = rollRot;
			break;

		case Neg_Z: //-Z face
			rotMat = glm::rotate(rollRot, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			break;
		}

		mSubViewports[i].getProjectionPlane()->setCoordinate(sgct_core::SGCTProjectionPlane::LowerLeft, glm::vec3(rotMat * lowerLeft));
		mSubViewports[i].getProjectionPlane()->setCoordinate(sgct_core::SGCTProjectionPlane::UpperLeft, glm::vec3(rotMat * upperLeft));
		mSubViewports[i].getProjectionPlane()->setCoordinate(sgct_core::SGCTProjectionPlane::UpperRight, glm::vec3(rotMat * upperRight));
	}
}



void sgct_core::SpoutOutputProjection::initShaders()
{	
	//reload shader program if it exists
	if (mDepthCorrectionShader.isLinked())
		mDepthCorrectionShader.deleteProgram();

	if (sgct::Engine::instance()->isOGLPipelineFixed())
	{
		//depth correction shader only
		if (sgct::SGCTSettings::instance()->useDepthTexture())
		{
			std::string depth_corr_frag_shader = sgct_core::shaders_fisheye::Base_Vert_Shader;
			std::string depth_corr_vert_shader = sgct_core::shaders_fisheye::Fisheye_Depth_Correction_Frag_Shader;

			//replace glsl version
			sgct_helpers::findAndReplace(depth_corr_frag_shader, "**glsl_version**", sgct::Engine::instance()->getGLSLVersion());
			sgct_helpers::findAndReplace(depth_corr_vert_shader, "**glsl_version**", sgct::Engine::instance()->getGLSLVersion());

			if (!mDepthCorrectionShader.addShaderSrc(depth_corr_frag_shader, GL_VERTEX_SHADER, sgct::ShaderProgram::SHADER_SRC_STRING))
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Failed to load fisheye depth correction vertex shader\n");
			if (!mDepthCorrectionShader.addShaderSrc(depth_corr_vert_shader, GL_FRAGMENT_SHADER, sgct::ShaderProgram::SHADER_SRC_STRING))
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Failed to load fisheye depth correction fragment shader\n");
		}
	}
	else //modern pipeline
	{
		//depth correction shader only
		if (sgct::SGCTSettings::instance()->useDepthTexture())
		{
			std::string depth_corr_frag_shader = sgct_core::shaders_modern_fisheye::Base_Vert_Shader;
			std::string depth_corr_vert_shader = sgct_core::shaders_modern_fisheye::Fisheye_Depth_Correction_Frag_Shader;

			//replace glsl version
			sgct_helpers::findAndReplace(depth_corr_frag_shader, "**glsl_version**", sgct::Engine::instance()->getGLSLVersion());
			sgct_helpers::findAndReplace(depth_corr_vert_shader, "**glsl_version**", sgct::Engine::instance()->getGLSLVersion());

			if (!mDepthCorrectionShader.addShaderSrc(depth_corr_frag_shader, GL_VERTEX_SHADER, sgct::ShaderProgram::SHADER_SRC_STRING))
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Failed to load fisheye depth correction vertex shader\n");
			if (!mDepthCorrectionShader.addShaderSrc(depth_corr_vert_shader, GL_FRAGMENT_SHADER, sgct::ShaderProgram::SHADER_SRC_STRING))
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Failed to load fisheye depth correction fragment shader\n");
		}
	}


	if (sgct::SGCTSettings::instance()->useDepthTexture())
	{
		mDepthCorrectionShader.setName("FisheyeDepthCorrectionShader");
		mDepthCorrectionShader.createAndLinkProgram();
		mDepthCorrectionShader.bind();

		mSwapColorLoc = mDepthCorrectionShader.getUniformLocation("cTex");
		glUniform1i(mSwapColorLoc, 0);
		mSwapDepthLoc = mDepthCorrectionShader.getUniformLocation("dTex");
		glUniform1i(mSwapDepthLoc, 1);
		mSwapNearLoc = mDepthCorrectionShader.getUniformLocation("near");
		mSwapFarLoc = mDepthCorrectionShader.getUniformLocation("far");

		sgct::ShaderProgram::unbind();
	}
}


void sgct_core::SpoutOutputProjection::drawCubeFace(const std::size_t & face)
{
	glLineWidth(1.0);
	sgct::Engine::instance()->getWireframe() ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//reset depth function (to opengl default)
	glDepthFunc(GL_LESS);

	//run scissor test to prevent clearing of entire buffer
	glEnable(GL_SCISSOR_TEST);
	setupViewport(face);

#if defined DebugCubemap
	float color[4];
	switch (face)
	{
	case 0:
		color[0] = 0.5f;
		color[1] = 0.0f;
		color[2] = 0.0f;
		color[3] = 1.0f;
		break;

	case 1:
		color[0] = 0.5f;
		color[1] = 0.5f;
		color[2] = 0.0f;
		color[3] = 1.0f;
		break;

	case 2:
		color[0] = 0.0f;
		color[1] = 0.5f;
		color[2] = 0.0f;
		color[3] = 1.0f;
		break;

	case 3:
		color[0] = 0.0f;
		color[1] = 0.5f;
		color[2] = 0.5f;
		color[3] = 1.0f;
		break;

	case 4:
		color[0] = 0.0f;
		color[1] = 0.0f;
		color[2] = 0.5f;
		color[3] = 1.0f;
		break;

	case 5:
		color[0] = 0.5f;
		color[1] = 0.0f;
		color[2] = 0.5f;
		color[3] = 1.0f;
		break;
	}
	glClearColor(color[0], color[1], color[2], color[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
	if (sgct::Engine::mInstance->mClearBufferFnPtr != SGCT_NULL_PTR)
		sgct::Engine::mInstance->mClearBufferFnPtr();
	else
	{
		const float * colorPtr = sgct::Engine::instance()->getClearColor();
		glClearColor(colorPtr[0], colorPtr[1], colorPtr[2], colorPtr[3]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
#endif

	glDisable(GL_SCISSOR_TEST);

	if (sgct::Engine::instance()->isOGLPipelineFixed())
	{
		glMatrixMode(GL_PROJECTION);
		SGCTProjection * proj = mSubViewports[face].getProjection(sgct::Engine::instance()->getCurrentFrustumMode());
		glLoadMatrixf(glm::value_ptr(proj->getProjectionMatrix()));
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(glm::value_ptr(proj->getViewMatrix() * sgct::Engine::instance()->getModelMatrix()));
	}

	//render
	sgct::Engine::mInstance->mDrawFnPtr();

	//restore polygon mode
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}



void sgct_core::SpoutOutputProjection::blitCubeFace(const int & face)
{
	//copy AA-buffer to "regular"/non-AA buffer
	mCubeMapFBO_Ptr->bindBlit(); //bind separate read and draw buffers to prepare blit operation
	attachTextures(face);
	mCubeMapFBO_Ptr->blit();
}



void sgct_core::SpoutOutputProjection::attachTextures(const int & face)
{
	if (sgct::SGCTSettings::instance()->useDepthTexture())
	{
		mCubeMapFBO_Ptr->attachDepthTexture(mTextures[DepthSwap]);
		mCubeMapFBO_Ptr->attachColorTexture(mTextures[ColorSwap]);
	}
	else
		mCubeMapFBO_Ptr->attachCubeMapTexture(mTextures[CubeMapColor], face);

	if (sgct::SGCTSettings::instance()->useNormalTexture())
		mCubeMapFBO_Ptr->attachCubeMapTexture(mTextures[CubeMapNormals], face, GL_COLOR_ATTACHMENT1);

	if (sgct::SGCTSettings::instance()->usePositionTexture())
		mCubeMapFBO_Ptr->attachCubeMapTexture(mTextures[CubeMapPositions], face, GL_COLOR_ATTACHMENT2);
}


void sgct_core::SpoutOutputProjection::renderInternal()
{
	glActiveTexture(GL_TEXTURE0);

#ifdef SGCT_HAS_SPOUT
	for (std::size_t i = 0; i < 6; i++)
	{
		glBindTexture(GL_TEXTURE_2D, spoutTexture[i]);
		((SPOUTHANDLE)handle[i])->SendTexture(spoutTexture[i], GL_TEXTURE_2D, mCubemapResolution, mCubemapResolution);
	}
#endif
	glBindTexture(GL_TEXTURE_2D, 0);
}



void sgct_core::SpoutOutputProjection::renderInternalFixedPipeline()
{
	glActiveTexture(GL_TEXTURE0);

#ifdef SGCT_HAS_SPOUT
	for (std::size_t i = 0; i < 6; i++)
	{
		glBindTexture(GL_TEXTURE_2D, spoutTexture[i]);
		((SPOUTHANDLE)handle[i])->SendTexture(spoutTexture[i], GL_TEXTURE_2D, mCubemapResolution, mCubemapResolution);
	}
#endif
	glBindTexture(GL_TEXTURE_2D, 0);
}



void sgct_core::SpoutOutputProjection::renderCubemapInternal(std::size_t * subViewPortIndex)
{
	BaseViewport * vp;
	unsigned int faceIndex;
	
	for (std::size_t i = 0; i < 6; i++)
	{
		vp = &mSubViewports[i];
		*subViewPortIndex = i;
		faceIndex = static_cast<unsigned int>(i);

		if (vp->isEnabled())
		{
			//bind & attach buffer
			mCubeMapFBO_Ptr->bind(); //osg seems to unbind FBO when rendering with osg FBO cameras
			if (!mCubeMapFBO_Ptr->isMultiSampled())
				attachTextures(faceIndex);

			sgct::Engine::mInstance->getCurrentWindowPtr()->setCurrentViewport(vp);
			drawCubeFace(i);

			//blit MSAA fbo to texture
			if (mCubeMapFBO_Ptr->isMultiSampled())
				blitCubeFace(faceIndex);

			//re-calculate depth values from a cube to spherical model
			if (sgct::SGCTSettings::instance()->useDepthTexture())
			{
				GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
				mCubeMapFBO_Ptr->bind(false, 1, buffers); //bind no multi-sampled

				mCubeMapFBO_Ptr->attachCubeMapTexture(mTextures[CubeMapColor], faceIndex);
				mCubeMapFBO_Ptr->attachCubeMapDepthTexture(mTextures[CubeMapDepth], faceIndex);

				glViewport(0, 0, mCubemapResolution, mCubemapResolution);
				glScissor(0, 0, mCubemapResolution, mCubemapResolution);
				glEnable(GL_SCISSOR_TEST);

				sgct::Engine::mInstance->mClearBufferFnPtr();

				glDisable(GL_CULL_FACE);
				bool alpha = sgct::Engine::mInstance->getCurrentWindowPtr()->getAlpha();
				if (alpha)
				{
					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				}
				else
					glDisable(GL_BLEND);

				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_ALWAYS);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, mTextures[ColorSwap]);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, mTextures[DepthSwap]);

				//bind shader
				bindDepthCorrectionShaderProgram();
				glUniform1i(mSwapColorLoc, 0);
				glUniform1i(mSwapDepthLoc, 1);
				glUniform1f(mSwapNearLoc, sgct::Engine::mInstance->mNearClippingPlaneDist);
				glUniform1f(mSwapFarLoc, sgct::Engine::mInstance->mFarClippingPlaneDist);

				sgct::Engine::mInstance->getCurrentWindowPtr()->bindVAO();
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				sgct::Engine::mInstance->getCurrentWindowPtr()->unbindVAO();

				//unbind shader
				sgct::ShaderProgram::unbind();

				glDisable(GL_DEPTH_TEST);

				if (alpha)
					glDisable(GL_BLEND);

				//restore depth func
				glDepthFunc(GL_LESS);
				glDisable(GL_SCISSOR_TEST);
			}//end if depthmap
			
			mCubeMapFBO_Ptr->unBind();

			if (handle[i]) {
				glBindTexture(GL_TEXTURE_2D, 0);
				glCopyImageSubData(mTextures[CubeMapColor], GL_TEXTURE_CUBE_MAP, 0, 0, 0, faceIndex, spoutTexture[i], GL_TEXTURE_2D, 0, 0, 0, 0, mCubemapResolution, mCubemapResolution, 1);
			}
		}//end if viewport is enabled
	}//end for
}



void sgct_core::SpoutOutputProjection::renderCubemapInternalFixedPipeline(std::size_t * subViewPortIndex)
{
	BaseViewport * vp;
	unsigned int faceIndex;

	for (std::size_t i = 0; i < 6; i++)
	{
		vp = &mSubViewports[i];
		*subViewPortIndex = i;
		faceIndex = static_cast<unsigned int>(i);

		if (vp->isEnabled())
		{
			//bind & attach buffer
			mCubeMapFBO_Ptr->bind(); //osg seems to unbind FBO when rendering with osg FBO cameras
			if (!mCubeMapFBO_Ptr->isMultiSampled())
				attachTextures(faceIndex);

			sgct::Engine::mInstance->getCurrentWindowPtr()->setCurrentViewport(vp);
			drawCubeFace(i);

			//blit MSAA fbo to texture
			if (mCubeMapFBO_Ptr->isMultiSampled())
				blitCubeFace(faceIndex);

			//re-calculate depth values from a cube to spherical model
			if (sgct::SGCTSettings::instance()->useDepthTexture())
			{
				GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
				mCubeMapFBO_Ptr->bind(false, 1, buffers); //bind no multi-sampled

				mCubeMapFBO_Ptr->attachCubeMapTexture(mTextures[CubeMapColor], faceIndex);
				mCubeMapFBO_Ptr->attachCubeMapDepthTexture(mTextures[CubeMapDepth], faceIndex);

				glViewport(0, 0, mCubemapResolution, mCubemapResolution);
				glScissor(0, 0, mCubemapResolution, mCubemapResolution);

				glPushAttrib(GL_ALL_ATTRIB_BITS);
				glEnable(GL_SCISSOR_TEST);
				
				sgct::Engine::mInstance->mClearBufferFnPtr();

				glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
				glMatrixMode(GL_TEXTURE);
				glLoadIdentity();

				glMatrixMode(GL_MODELVIEW); //restore

				//bind shader
				bindDepthCorrectionShaderProgram();
				glUniform1i(mSwapColorLoc, 0);
				glUniform1i(mSwapDepthLoc, 1);
				glUniform1f(mSwapNearLoc, sgct::Engine::mInstance->mNearClippingPlaneDist);
				glUniform1f(mSwapFarLoc, sgct::Engine::mInstance->mFarClippingPlaneDist);

				glDisable(GL_CULL_FACE);
				bool alpha = sgct::Engine::mInstance->getCurrentWindowPtr()->getAlpha();
				if (alpha)
				{
					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				}
				else
					glDisable(GL_BLEND);

				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_ALWAYS);
				
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, mTextures[ColorSwap]);

				glActiveTexture(GL_TEXTURE1);
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, mTextures[DepthSwap]);

				glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
				sgct::Engine::mInstance->getCurrentWindowPtr()->bindVBO();
				glClientActiveTexture(GL_TEXTURE0);

				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(0));

				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(8));
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				sgct::Engine::mInstance->getCurrentWindowPtr()->unbindVBO();
				glPopClientAttrib();

				//unbind shader
				sgct::ShaderProgram::unbind();
				
				glPopAttrib();
			}//end if depthmap

			mCubeMapFBO_Ptr->unBind();

			if (handle[i]) {
				glBindTexture(GL_TEXTURE_2D, 0);
				glCopyImageSubData(mTextures[CubeMapColor], GL_TEXTURE_CUBE_MAP, 0, 0, 0, faceIndex, spoutTexture[i], GL_TEXTURE_2D, 0, 0, 0, 0, mCubemapResolution, mCubemapResolution, 1);
			}
		}//end if viewport is enabled
	}//end for
}