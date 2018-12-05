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
#include <sgct/shaders/SGCTInternalFisheyeShaders_cubic.h>
#include <sgct/shaders/SGCTInternalFisheyeShaders_modern_cubic.h>
#include <sgct/helpers/SGCTStringFunctions.h>
#include <sstream>
#include <algorithm>

#ifdef SGCT_HAS_SPOUT
#include <spout/SpoutLibrary.h>
#endif


//#define DebugCubemap

const std::string sgct_core::SpoutOutputProjection::spoutCubeMapFaceName[spoutTotalFaces] = {
    "Right",
    "zLeft",
    "Bottom",
    "Top",
    "Left",
    "zRight",
};

sgct_core::SpoutOutputProjection::SpoutOutputProjection()
{
    mInternalRenderFn = &SpoutOutputProjection::renderInternalFixedPipeline;
    mInternalRenderCubemapFn = &SpoutOutputProjection::renderCubemapInternalFixedPipeline;

    for (size_t i = 0; i < spoutTotalFaces; i++)
    {
        spoutEnabled[i] = true;
        handle[i] = nullptr;
        spoutTexture[i] = -1;
    }

    mSpoutFBO_Ptr = NULL;
    spoutMappingHandle = nullptr;
    spoutMappingTexture = -1;
    spoutMappingType = Mapping::Cubemap;
    spoutRigOrientation = glm::vec3(0.0f);

    spoutMappingName = "SPOUT_OS_MAPPING";

    mCubemapLoc = -1;
    mHalfFovLoc = -1;
    mSwapColorLoc = -1;
    mSwapDepthLoc = -1;
    mSwapNearLoc = -1;
    mSwapFarLoc = -1;
}

sgct_core::SpoutOutputProjection::~SpoutOutputProjection()
{
    for (size_t i = 0; i < spoutTotalFaces; i++)
    {
        if (handle[i])
        {
#ifdef SGCT_HAS_SPOUT
            ((SPOUTHANDLE)handle[i])->ReleaseSender();
            ((SPOUTHANDLE)handle[i])->Release();
#endif
            handle[i] = nullptr;
        }
        if (spoutTexture[i] != -1)
        {
            glDeleteTextures(1, &mTextures[i]);
            spoutTexture[i] = -1;
        }
    }

    if (spoutMappingHandle)
    {
#ifdef SGCT_HAS_SPOUT
        ((SPOUTHANDLE)spoutMappingHandle)->ReleaseSender();
        ((SPOUTHANDLE)spoutMappingHandle)->Release();
#endif
        spoutMappingHandle = nullptr;
    }

    if (spoutMappingTexture != -1)
    {
        glDeleteTextures(1, &spoutMappingTexture);
        spoutMappingTexture = -1;
    }

    if (mSpoutFBO_Ptr)
    {
        mSpoutFBO_Ptr->destroy();
        delete mSpoutFBO_Ptr;
    }
}

/*!
Update projection when aspect ratio changes for the viewport.
*/
void sgct_core::SpoutOutputProjection::update(float width, float height)
{
    updateGeomerty(width, height);
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


void sgct_core::SpoutOutputProjection::setSpoutChannels(bool channels[spoutTotalFaces]) {
    for (size_t i = 0; i < spoutTotalFaces; i++) {
        spoutEnabled[i] = channels[i];
    }
}


void sgct_core::SpoutOutputProjection::setSpoutMappingName(std::string name) {
    spoutMappingName = name;
}


void sgct_core::SpoutOutputProjection::setSpoutMapping(Mapping type) {
    spoutMappingType = type;
}

void sgct_core::SpoutOutputProjection::setSpoutRigOrientation(glm::vec3 orientation) {
    spoutRigOrientation = orientation;
}



void sgct_core::SpoutOutputProjection::initTextures() {
    sgct_core::NonLinearProjection::initTextures();

    sgct::MessageHandler::instance()->print("SpoutOutputProjection initTextures");

    if (sgct::Engine::instance()->getRunMode() <= sgct::Engine::OpenGL_Compablity_Profile)
    {
        glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT);
        glEnable(GL_TEXTURE_2D);
    }

    switch (spoutMappingType) {
    case Mapping::Cubemap:
        spoutMappingWidth = mCubemapResolution;
        spoutMappingHeight = mCubemapResolution;

        for (size_t i = 0; i < spoutTotalFaces; ++i) {
#ifdef SGCT_HAS_SPOUT
            sgct::MessageHandler::instance()->print("SpoutOutputProjection initTextures %d", i);
            if (!spoutEnabled[i]) continue;
            handle[i] = GetSpout();
            if (handle[i]) ((SPOUTHANDLE)handle[i])->CreateSender(spoutCubeMapFaceName[i].c_str(), spoutMappingWidth, spoutMappingHeight);
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
            glTexImage2D(GL_TEXTURE_2D, 0, mTextureInternalFormat, spoutMappingWidth, spoutMappingHeight, 0, mTextureFormat, mTextureType, NULL);
        }
        break;

    case Mapping::Equirectangular:
        spoutMappingWidth = mCubemapResolution * 4;
        spoutMappingHeight = mCubemapResolution * 2;

#ifdef SGCT_HAS_SPOUT
        sgct::MessageHandler::instance()->print("SpoutOutputProjection initTextures Equirectangular");
        spoutMappingHandle = GetSpout();
        if (spoutMappingHandle) ((SPOUTHANDLE)spoutMappingHandle)->CreateSender(spoutMappingName.c_str(), spoutMappingWidth, spoutMappingHeight);
#endif

        glGenTextures(1, &spoutMappingTexture);
        glBindTexture(GL_TEXTURE_2D, spoutMappingTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, mTextureInternalFormat, spoutMappingWidth, spoutMappingHeight, 0, mTextureFormat, mTextureType, NULL);
        break;

    case Mapping::Fisheye:
        spoutMappingWidth = mCubemapResolution * 2;
        spoutMappingHeight = mCubemapResolution * 2;

#ifdef SGCT_HAS_SPOUT
        sgct::MessageHandler::instance()->print("SpoutOutputProjection initTextures Fisheye");
        spoutMappingHandle = GetSpout();
        if (spoutMappingHandle) ((SPOUTHANDLE)spoutMappingHandle)->CreateSender(spoutMappingName.c_str(), spoutMappingWidth, spoutMappingHeight);
#endif

        glGenTextures(1, &spoutMappingTexture);
        glBindTexture(GL_TEXTURE_2D, spoutMappingTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, mTextureInternalFormat, spoutMappingWidth, spoutMappingHeight, 0, mTextureFormat, mTextureType, NULL);
        break;
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

    glm::mat4 pitchRot = glm::rotate(glm::mat4(1.0f), glm::radians(-spoutRigOrientation.x), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 yawRot = glm::rotate(pitchRot, glm::radians(spoutRigOrientation.y), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rollRot = glm::rotate(yawRot, glm::radians(-spoutRigOrientation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    //glm::mat4 rollRot = glm::rotate(tiltRot, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    //glm::mat4 rollRot = glm::mat4(1.0f);

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
    if (mShader.isLinked())
        mShader.deleteProgram();

    std::string fisheyeFragmentShader;
    std::string fisheyeVertexShader;

    if (sgct::Engine::instance()->isOGLPipelineFixed())
    {
        fisheyeVertexShader = sgct_core::shaders_fisheye::Fisheye_Vert_Shader;

        if (sgct::SGCTSettings::instance()->useDepthTexture())
        {
            switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType())
            {
            case sgct::SGCTSettings::Diffuse:
            default:
                fisheyeFragmentShader = sgct_core::shaders_fisheye::Fisheye_Frag_Shader_Depth;
                break;

            case sgct::SGCTSettings::Diffuse_Normal:
                fisheyeFragmentShader = sgct_core::shaders_fisheye::Fisheye_Frag_Shader_Depth_Normal;
                break;

            case sgct::SGCTSettings::Diffuse_Position:
                fisheyeFragmentShader = sgct_core::shaders_fisheye::Fisheye_Frag_Shader_Depth_Position;
                break;

            case sgct::SGCTSettings::Diffuse_Normal_Position:
                fisheyeFragmentShader = sgct_core::shaders_fisheye::Fisheye_Frag_Shader_Depth_Normal_Position;
                break;
            }
        }
        else //no depth
        {
            switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType())
            {
            case sgct::SGCTSettings::Diffuse:
            default:
                fisheyeFragmentShader =  sgct_core::shaders_fisheye::Fisheye_Frag_Shader;
                break;

            case sgct::SGCTSettings::Diffuse_Normal:
                fisheyeFragmentShader = sgct_core::shaders_fisheye::Fisheye_Frag_Shader_Normal;
                break;

            case sgct::SGCTSettings::Diffuse_Position:
                fisheyeFragmentShader = sgct_core::shaders_fisheye::Fisheye_Frag_Shader_Position;
                break;

            case sgct::SGCTSettings::Diffuse_Normal_Position:
                fisheyeFragmentShader = sgct_core::shaders_fisheye::Fisheye_Frag_Shader_Normal_Position;
                break;
            }
        }

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
        fisheyeVertexShader = sgct_core::shaders_modern_fisheye::Fisheye_Vert_Shader;

        if (sgct::SGCTSettings::instance()->useDepthTexture())
        {
            switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType())
            {
            case sgct::SGCTSettings::Diffuse:
            default:
                fisheyeFragmentShader = sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_Depth;
                break;

            case sgct::SGCTSettings::Diffuse_Normal:
                fisheyeFragmentShader = sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_Depth_Normal;
                break;

            case sgct::SGCTSettings::Diffuse_Position:
                fisheyeFragmentShader = sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_Depth_Position;
                break;

            case sgct::SGCTSettings::Diffuse_Normal_Position:
                fisheyeFragmentShader = sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_Depth_Normal_Position;
                break;
            }
        }
        else //no depth
        {
            switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType())
            {
            case sgct::SGCTSettings::Diffuse:
            default:
                fisheyeFragmentShader = sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader;
                break;

            case sgct::SGCTSettings::Diffuse_Normal:
                fisheyeFragmentShader = sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_Normal;
                break;

            case sgct::SGCTSettings::Diffuse_Position:
                fisheyeFragmentShader = sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_Position;
                break;

            case sgct::SGCTSettings::Diffuse_Normal_Position:
                fisheyeFragmentShader = sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_Normal_Position;
                break;
            }
        }

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

    //add functions to shader
    switch (spoutMappingType) {
    case Mapping::Fisheye:
        sgct_helpers::findAndReplace(fisheyeFragmentShader, "**sample_fun**", sgct::Engine::instance()->isOGLPipelineFixed() ? sgct_core::shaders_fisheye::sample_fun : sgct_core::shaders_modern_fisheye::sample_fun);
        break;
    case Mapping::Equirectangular:
        sgct_helpers::findAndReplace(fisheyeFragmentShader, "**sample_fun**", sgct::Engine::instance()->isOGLPipelineFixed() ? sgct_core::shaders_fisheye::sample_latlon_fun : sgct_core::shaders_modern_fisheye::sample_latlon_fun);
        break;
    default:
        sgct_helpers::findAndReplace(fisheyeFragmentShader, "**sample_fun**", sgct::Engine::instance()->isOGLPipelineFixed() ? sgct_core::shaders_fisheye::sample_fun : sgct_core::shaders_modern_fisheye::sample_fun);
        break;
    }

    glm::mat4 pitchRot = glm::rotate(glm::mat4(1.0f), glm::radians(spoutRigOrientation.x), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 yawRot = glm::rotate(pitchRot, glm::radians(spoutRigOrientation.y), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rollRot = glm::rotate(yawRot, glm::radians(spoutRigOrientation.z), glm::vec3(0.0f, 0.0f, 1.0f));

//    yawRot[0][0] * x + yawRot[0][1] * y + yawRot[0][2] * z;
//    yawRot[1][0] * x + yawRot[1][1] * y + yawRot[1][2] * z;
//    yawRot[2][0] * x + yawRot[2][1] * y + yawRot[2][2] * z;

    char str[400];
    sprintf(str, "vec3 rotVec = vec3(%0.5ff*x + %0.5ff*y + %0.5ff*z, %0.5ff*x + %0.5ff*y + %0.5ff*z, %0.5ff*x + %0.5ff*y + %0.5ff*z)", rollRot[0][0], rollRot[0][1], rollRot[0][2], rollRot[1][0], rollRot[1][1], rollRot[1][2], rollRot[2][0], rollRot[2][1], rollRot[2][2]);

    //replace add correct transform in the fragment shader
    //sgct_helpers::findAndReplace(fisheyeFragmentShader, "**rotVec**", "vec3 rotVec = vec3(angle45Factor*x - angle45Factor*y, angle45Factor*x + angle45Factor*y, z)");
    sgct_helpers::findAndReplace(fisheyeFragmentShader, "**rotVec**", str);

    //replace glsl version
    sgct_helpers::findAndReplace(fisheyeVertexShader, "**glsl_version**", sgct::Engine::instance()->getGLSLVersion());
    sgct_helpers::findAndReplace(fisheyeFragmentShader, "**glsl_version**", sgct::Engine::instance()->getGLSLVersion());

    //replace color
    std::stringstream ssColor;
    ssColor.precision(2);
    ssColor << std::fixed << "vec4(" << mClearColor.r << ", " << mClearColor.g << ", " << mClearColor.b << ", " << mClearColor.a << ")";
    sgct_helpers::findAndReplace(fisheyeFragmentShader, "**bgColor**", ssColor.str());

    if (!mShader.addShaderSrc(fisheyeVertexShader, GL_VERTEX_SHADER, sgct::ShaderProgram::SHADER_SRC_STRING))
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Failed to load fisheye vertex shader:\n%s\n", fisheyeVertexShader.c_str());
    if (!mShader.addShaderSrc(fisheyeFragmentShader, GL_FRAGMENT_SHADER, sgct::ShaderProgram::SHADER_SRC_STRING))
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Failed to load fisheye fragment shader\n%s\n", fisheyeFragmentShader.c_str());


    switch (spoutMappingType) {
    case Mapping::Fisheye:
        mShader.setName("FisheyeShader");
        break;
    case Mapping::Equirectangular:
        mShader.setName("EquirectangularShader");
        break;
    case Mapping::Cubemap:
        mShader.setName("None");
        break;
    }
    mShader.createAndLinkProgram();
    mShader.bind();

    mCubemapLoc = mShader.getUniformLocation("cubemap");
    glUniform1i(mCubemapLoc, 0);

    mHalfFovLoc = mShader.getUniformLocation("halfFov");
    glUniform1f(mHalfFovLoc, glm::half_pi<float>());

    sgct::ShaderProgram::unbind();

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


void sgct_core::SpoutOutputProjection::initFBO() {
    NonLinearProjection::initFBO();

    mSpoutFBO_Ptr = new sgct_core::OffScreenBuffer();
    mSpoutFBO_Ptr->setInternalColorFormat(mTextureInternalFormat);
    mSpoutFBO_Ptr->createFBO(spoutMappingWidth, spoutMappingHeight, 1);

    if (mSpoutFBO_Ptr->checkForErrors())
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Spout FBO created.\n");
    else
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Spout FBO created with errors!\n");

    sgct_core::OffScreenBuffer::unBind();
}



void sgct_core::SpoutOutputProjection::updateGeomerty(const float & width, const float & height)
{
    float x = 1.0f;
    float y = 1.0f;

    mVerts[0] = 0.0f;
    mVerts[1] = 0.0f;
    mVerts[2] = -x;
    mVerts[3] = -y;
    mVerts[4] = -1.0f;

    mVerts[5] = 0.0f;
    mVerts[6] = 1.0f;
    mVerts[7] = -x;
    mVerts[8] = y;
    mVerts[9] = -1.0f;

    mVerts[10] = 1.0f;
    mVerts[11] = 0.0f;
    mVerts[12] = x;
    mVerts[13] = -y;
    mVerts[14] = -1.0f;

    mVerts[15] = 1.0f;
    mVerts[16] = 1.0f;
    mVerts[17] = x;
    mVerts[18] = y;
    mVerts[19] = -1.0f;

    //update VBO
    if (!sgct::Engine::instance()->isOGLPipelineFixed())
        glBindVertexArray(mVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);

    GLvoid* PositionBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    memcpy(PositionBuffer, mVerts, 20 * sizeof(float));
    glUnmapBuffer(GL_ARRAY_BUFFER);

    if (!sgct::Engine::instance()->isOGLPipelineFixed())
        glBindVertexArray(0);
    else
        glBindBuffer(GL_ARRAY_BUFFER, 0);
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
    glEnable(GL_SCISSOR_TEST);
    sgct::Engine::mInstance->enterCurrentViewport();
    glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    if (spoutMappingType != Mapping::Cubemap) {
        GLint saveBuffer = 0;
        GLint saveTexture = 0;
        GLint saveFrameBuffer = 0;
        glGetIntegerv(GL_DRAW_BUFFER0, &saveBuffer);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &saveTexture);
        glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &saveFrameBuffer);


        GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
        mSpoutFBO_Ptr->bind(false, 1, buffers); //bind no multi-sampled
        mSpoutFBO_Ptr->attachColorTexture(spoutMappingTexture);

        bindShaderProgram();

        glViewport(0, 0, spoutMappingWidth, spoutMappingHeight);
        glScissor(0, 0, spoutMappingWidth, spoutMappingHeight);
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        //if for some reson the active texture has been reset
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures[CubeMapColor]);

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

        glUniform1i(mCubemapLoc, 0);
        if (spoutMappingType == Mapping::Fisheye) {
            glUniform1f(mHalfFovLoc, glm::radians<float>(180.0f / 2.0f));
        }
        else if (spoutMappingType == Mapping::Equirectangular) {
            glUniform1f(mHalfFovLoc, glm::radians<float>(360.0f / 2.0f));
        }

        glBindVertexArray(mVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(GL_FALSE);

        sgct::ShaderProgram::unbind();

        glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        glDisable(GL_DEPTH_TEST);

        if (alpha)
            glDisable(GL_BLEND);

        //restore depth func
        glDepthFunc(GL_LESS);

        mSpoutFBO_Ptr->unBind();

        glBindTexture(GL_TEXTURE_2D, spoutMappingTexture);
        ((SPOUTHANDLE)spoutMappingHandle)->SendTexture(spoutMappingTexture, GL_TEXTURE_2D, spoutMappingWidth, spoutMappingHeight);
        glBindTexture(GL_TEXTURE_2D, 0);

        buffers[0] = saveBuffer;
        glDrawBuffers(1, buffers);
        glBindTexture(GL_TEXTURE_2D, saveTexture);
        glBindFramebuffer(GL_FRAMEBUFFER, saveFrameBuffer);
    }
    else {
        GLint saveTexture = 0;
        GLint saveFrameBuffer = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &saveTexture);
        glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &saveFrameBuffer);

#ifdef SGCT_HAS_SPOUT
        for (std::size_t i = 0; i < spoutTotalFaces; i++)
        {
            if (!spoutEnabled[i]) continue;
            glBindTexture(GL_TEXTURE_2D, spoutTexture[i]);
            ((SPOUTHANDLE)handle[i])->SendTexture(spoutTexture[i], GL_TEXTURE_2D, spoutMappingWidth, spoutMappingHeight);
        }
#endif

        glBindTexture(GL_TEXTURE_2D, saveTexture);
        glBindFramebuffer(GL_FRAMEBUFFER, saveFrameBuffer);
    }
}

void sgct_core::SpoutOutputProjection::renderInternalFixedPipeline()
{
    glEnable(GL_SCISSOR_TEST);
    sgct::Engine::mInstance->enterCurrentViewport();
    glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    if (spoutMappingType != Mapping::Cubemap) {
        GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
        mSpoutFBO_Ptr->bind(false, 1, buffers); //bind no multi-sampled
        mSpoutFBO_Ptr->attachColorTexture(spoutMappingTexture);

        bindShaderProgram();

        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        //if for some reson the active texture has been reset
        glActiveTexture(GL_TEXTURE0);

        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW); //restore

        glEnable(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures[CubeMapColor]);

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

        glUniform1i(mCubemapLoc, 0);
        if (spoutMappingType == Mapping::Fisheye) {
            glUniform1f(mHalfFovLoc, glm::radians<float>(180.0f / 2.0f));
        } else if (spoutMappingType == Mapping::Equirectangular) {
            glUniform1f(mHalfFovLoc, glm::radians<float>(360.0f / 2.0f));
        }

        glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
        //make sure that VBO:s are unbinded, to not mess up the vertex array
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glClientActiveTexture(GL_TEXTURE0);

        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(0));

        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(8));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindBuffer(GL_ARRAY_BUFFER, GL_FALSE);

        sgct::ShaderProgram::unbind();
        mSpoutFBO_Ptr->unBind();

        glBindTexture(GL_TEXTURE_2D, spoutMappingTexture);
        ((SPOUTHANDLE)spoutMappingHandle)->SendTexture(spoutMappingTexture, GL_TEXTURE_2D, spoutMappingWidth, spoutMappingHeight);
        glBindTexture(GL_TEXTURE_2D, 0);

        glPopClientAttrib();
        glPopAttrib();
    }
    else {
#ifdef SGCT_HAS_SPOUT
        for (std::size_t i = 0; i < spoutTotalFaces; i++)
        {
            if (!spoutEnabled[i]) continue;
            glBindTexture(GL_TEXTURE_2D, spoutTexture[i]);
            ((SPOUTHANDLE)handle[i])->SendTexture(spoutTexture[i], GL_TEXTURE_2D, spoutMappingWidth, spoutMappingHeight);
        }
#endif

        glBindTexture(GL_TEXTURE_2D, 0);
    }
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

        if (!spoutEnabled[i]) continue;

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

                glViewport(0, 0, spoutMappingWidth, spoutMappingHeight);
                glScissor(0, 0, spoutMappingWidth, spoutMappingHeight);
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

            if (spoutMappingType == Mapping::Cubemap) {
                mCubeMapFBO_Ptr->unBind();

                if (handle[i]) {
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glCopyImageSubData(mTextures[CubeMapColor], GL_TEXTURE_CUBE_MAP, 0, 0, 0, faceIndex, spoutTexture[i], GL_TEXTURE_2D, 0, 0, 0, 0, spoutMappingWidth, spoutMappingHeight, 1);
                }
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

        if (!spoutEnabled[i]) continue;

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

                glViewport(0, 0, spoutMappingWidth, spoutMappingHeight);
                glScissor(0, 0, spoutMappingWidth, spoutMappingHeight);

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

            if (spoutMappingType == Mapping::Cubemap) {
                mCubeMapFBO_Ptr->unBind();

                if (handle[i]) {
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glCopyImageSubData(mTextures[CubeMapColor], GL_TEXTURE_CUBE_MAP, 0, 0, 0, faceIndex, spoutTexture[i], GL_TEXTURE_2D, 0, 0, 0, 0, spoutMappingWidth, spoutMappingHeight, 1);
                }
            }
        }//end if viewport is enabled
    }//end for
}
