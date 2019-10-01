/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/FisheyeProjection.h>
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

//#define DebugCubemap

sgct_core::FisheyeProjection::FisheyeProjection()
{
    mInternalRenderFn = &FisheyeProjection::renderInternalFixedPipeline;
    mInternalRenderCubemapFn = &FisheyeProjection::renderCubemapInternalFixedPipeline;
    
    mFOV = 180.0f;
    mTilt = 0.0f;
    mDiameter = 14.8f;
    for (std::size_t i = 0; i < 4; i++)
        mCropFactors[i] = 0.0;

    mOffset.x = 0.0f;
    mOffset.y = 0.0f;
    mOffset.z = 0.0f;

    mBaseOffset.x = 0.0f;
    mBaseOffset.y = 0.0f;
    mBaseOffset.z = 0.0f;

    mTotalOffset = mBaseOffset + mOffset;

    mOffAxis = false;
    mIgnoreAspectRatio = false;
    mMethod = FourFaceCube;

    mCubemapLoc = -1;
    mDepthCubemapLoc = -1;
    mNormalCubemapLoc = -1;
    mPositionCubemapLoc = -1;
    mHalfFovLoc = -1;
    mOffsetLoc = -1;
    mSwapColorLoc = -1;
    mSwapDepthLoc = -1;
    mSwapNearLoc = -1;
    mSwapFarLoc = -1;
}

sgct_core::FisheyeProjection::~FisheyeProjection()
{
    
}

/*!
Update projection when aspect ratio changes for the viewport.
*/
void sgct_core::FisheyeProjection::update(float width, float height)
{
    updateGeomerty(width, height);
}

/*!
Render the non linear projection to currently bounded FBO
*/
void sgct_core::FisheyeProjection::render()
{
    (this->*mInternalRenderFn)();
}

/*!
Render the enabled faces of the cubemap
*/
void sgct_core::FisheyeProjection::renderCubemap(std::size_t * subViewPortIndex)
{
    if (sgct::Engine::instance()->getCurrentFrustumMode() == Frustum::StereoLeftEye)
        setOffset(-sgct::Engine::instance()->getDefaultUserPtr()->getEyeSeparation() / mDiameter, 0.0f);
    else if (sgct::Engine::instance()->getCurrentFrustumMode() == Frustum::StereoRightEye)
        setOffset(sgct::Engine::instance()->getDefaultUserPtr()->getEyeSeparation() / mDiameter, 0.0f);
    (this->*mInternalRenderCubemapFn)(subViewPortIndex);
}

/*!
Set the dome diameter used in the fisheye renderer (used for the viewplane distance calculations)

@param diameter diameter of the dome diameter in meters
*/
void sgct_core::FisheyeProjection::setDomeDiameter(float diameter)
{
    mDiameter = diameter;
    //generateCubeMapViewports();
}

/*!
Set the fisheye/dome tilt angle used in the fisheye renderer.
The tilt angle is from the horizontal.

@param angle the tilt angle in degrees
*/
void sgct_core::FisheyeProjection::setTilt(float angle)
{
    mTilt = angle;
}

/*!
Set the fisheye/dome field-of-view angle used in the fisheye renderer.

@param angle the FOV angle in degrees
*/
void sgct_core::FisheyeProjection::setFOV(float angle)
{
    mFOV = angle;
}

/*!
Set the method used for rendering the fisheye projection.

@param method the selected method
*/
void sgct_core::FisheyeProjection::setRenderingMethod(FisheyeMethod method)
{
    mMethod = method;
}

/*!
Set the fisheye crop values. Theese values are used when rendering content for a single projector dome.
The elumenati geodome has usually a 4:3 SXGA+ (1400x1050) projector and the fisheye is cropped 25% (350 pixels) at the top.
*/
void sgct_core::FisheyeProjection::setCropFactors(float left, float right, float bottom, float top)
{
    mCropFactors[CropLeft] = (left < 1.0f && left > 0.0f) ? left : 0.0f;
    mCropFactors[CropRight] = (right < 1.0f && right > 0.0f) ? right : 0.0f;
    mCropFactors[CropBottom] = (bottom < 1.0f && bottom > 0.0f) ? bottom : 0.0f;
    mCropFactors[CropTop] = (top < 1.0f && top > 0.0f) ? top : 0.0f;
}

/*!
Set fisheye offset to render offaxis. Length of vector must be smaller then 1.
Base of fisheye is the XY-plane. This function is normally used in fisheye stereo rendering.
*/
void sgct_core::FisheyeProjection::setOffset(const glm::vec3 & offset)
{
    mOffset = offset;
    mTotalOffset = mBaseOffset + mOffset;

    mOffAxis = (glm::length(mTotalOffset) > 0.0f ? true : false);
}

/*!
Set fisheye offset to render offaxis. Length of vector must be smaller then 1.
Base of fisheye is the XY-plane. This function is normally used in fisheye stereo rendering.
*/
void sgct_core::FisheyeProjection::setOffset(float x, float y, float z)
{
    mOffset.x = x;
    mOffset.y = y;
    mOffset.z = z;

    mTotalOffset = mBaseOffset + mOffset;
    mOffAxis = (glm::length(mTotalOffset) > 0.0f ? true : false);
}

/*!
Set fisheye base offset to render offaxis. Length of vector must be smaller then 1.
Base of fisheye is the XY-plane. The base offset will be added to the offset specified by setFisheyeOffset.
These values are set from the XML config.
*/
void sgct_core::FisheyeProjection::setBaseOffset(const glm::vec3 & offset)
{
    mBaseOffset = offset;
    mTotalOffset = mBaseOffset + mOffset;

    mOffAxis = (glm::length(mTotalOffset) > 0.0f ? true : false);
}

/*!
Ignore the framebuffer aspect ratio to allow non-circular fisheye. This is usefull for spherical mirror projections.
*/
void sgct_core::FisheyeProjection::setIgnoreAspectRatio(bool state)
{
    mIgnoreAspectRatio = state;
}

/*!
Get the lens offset for off-axis projection.

@returns offset
*/
glm::vec3 sgct_core::FisheyeProjection::getOffset() const
{
    return mTotalOffset;
}

void sgct_core::FisheyeProjection::initViewports()
{
    enum cubeFaces { Pos_X = 0, Neg_X, Pos_Y, Neg_Y, Pos_Z, Neg_Z };

    //radius is needed to calculate the distance to all view planes
    float radius = mDiameter / 2.0f;

    //setup base viewport that will be rotated to create the other cubemap views
    //+Z face
    const glm::vec4 lowerLeftBase(-radius, -radius, radius, 1.0f);
    const glm::vec4 upperLeftBase(-radius, radius, radius, 1.0f);
    const glm::vec4 upperRightBase(radius, radius, radius, 1.0f);

    //250.5287794 degree FOV covers exactly five sides of a cube, larger FOV needs six sides
    const float fiveFaceLimit = 2.0f * glm::degrees(acosf(-1.0f / sqrtf(3.0f)));
    //109.4712206 degree FOV is needed to cover the entire top face
    const float topFaceLimit = 2.0f * glm::degrees(acosf(1.0f / sqrtf(3.0f)));

    float cropLevel = 0.5f; //how much of the side faces that are used
    float projectionOffset = 0.0f; //the projection offset
    float normalizedProjectionOffset = 0.0f;

    //four faces doesn't cover more than 180 degrees
    if (mFOV > 180.0f && mFOV <= fiveFaceLimit)
        mMethod = FiveFaceCube;

    if (mMethod == FiveFaceCube && mFOV >= topFaceLimit && mFOV <= fiveFaceLimit)
    {
        //helper variable
        float cosAngle = cosf(glm::radians(mFOV / 2.0f));
        if (mFOV < 180.0f)
        {
            normalizedProjectionOffset = 1.0f-mFOV/180.0f; //[-1, 0]
        }
        else
        {
            normalizedProjectionOffset = sqrtf((2.0f * cosAngle * cosAngle) / (1.0f - cosAngle * cosAngle)); //[0, 1]
        }

        projectionOffset = normalizedProjectionOffset * radius;
        cropLevel = (1.0f - normalizedProjectionOffset) / 2.0f;

        //cropLevel = 0.0f;
        //projectionOffset = radius;
    }
    else if (mFOV > fiveFaceLimit)
    {
        mMethod = SixFaceCube;
        cropLevel = 0.0f;
        projectionOffset = radius;
    }

    if (mMethod == FiveFaceCube || mMethod == SixFaceCube)
    {
        //tilt
        glm::mat4 tiltMat = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f - mTilt), glm::vec3(1.0f, 0.0f, 0.0f));
        //glm::mat4 tiltMat(1.0f);

        //roll 45 deg
        glm::mat4 rollRot = glm::rotate(tiltMat, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        //glm::mat4 rollRot(1.0f);
        //glm::mat4 rollRot = tiltMat;

        //add viewports
        for (unsigned int i = 0; i<6; i++)
        {
            std::stringstream ss;
            ss << "Fisheye " << i;
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
                upperRight.x = projectionOffset;
                mSubViewports[i].setSize(1.0f - cropLevel, 1.0f);
            }
            break;

            case Neg_X: //-X face
            {
                rotMat = glm::rotate(rollRot, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                lowerLeft.x = -projectionOffset;
                upperLeft.x = -projectionOffset;
                mSubViewports[i].setPos(cropLevel, 0.0f);
                mSubViewports[i].setSize(1.0f - cropLevel, 1.0f);
            }
            break;

            case Pos_Y: //+Y face
            {
                rotMat = glm::rotate(rollRot, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                lowerLeft.y = -projectionOffset;
                mSubViewports[i].setPos(0.0f, cropLevel);
                mSubViewports[i].setSize(1.0f, 1.0f - cropLevel);
            }
            break;

            case Neg_Y: //-Y face
            {
                rotMat = glm::rotate(rollRot, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                upperLeft.y = projectionOffset;
                upperRight.y = projectionOffset;
                mSubViewports[i].setSize(1.0f, 1.0f - cropLevel);
            }
            break;

            case Pos_Z: //+Z face
                rotMat = rollRot;
                break;

            case Neg_Z: //-Z face
                if(mMethod == FiveFaceCube)
                    mSubViewports[i].setEnabled(false);
                rotMat = glm::rotate(rollRot, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                break;
            }

            mSubViewports[i].getProjectionPlane()->setCoordinate(sgct_core::SGCTProjectionPlane::LowerLeft, glm::vec3(rotMat * lowerLeft));
            mSubViewports[i].getProjectionPlane()->setCoordinate(sgct_core::SGCTProjectionPlane::UpperLeft, glm::vec3(rotMat * upperLeft));
            mSubViewports[i].getProjectionPlane()->setCoordinate(sgct_core::SGCTProjectionPlane::UpperRight, glm::vec3(rotMat * upperRight));
        }
    }
    else
    {
        //tilt
        glm::mat4 tiltMat = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f - mTilt), glm::vec3(1.0f, 0.0f, 0.0f));
        //glm::mat4 tiltMat(1.0f);

        //pan 45 deg
        glm::mat4 panRot = glm::rotate(tiltMat, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        //glm::mat4 panRot(1.0f);
        //glm::mat4 panRot = tiltMat;

        //add viewports
        for (unsigned int i = 0; i<6; i++)
        {
            std::stringstream ss;
            ss << "Fisheye " << i;
            mSubViewports[i].setName( ss.str());

            glm::vec4 lowerLeft = lowerLeftBase;
            glm::vec4 upperLeft = upperLeftBase;
            glm::vec4 upperRight = upperRightBase;

            glm::mat4 rotMat(1.0f);

            switch (i)
            {
            case Pos_X: //+X face
                rotMat = glm::rotate(panRot, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                break;

            case Neg_X: //-X face
                mSubViewports[i].setEnabled(false);
                rotMat = glm::rotate(panRot, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                break;

            case Pos_Y: //+Y face
                rotMat = glm::rotate(panRot, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                break;

            case Neg_Y: //-Y face
                rotMat = glm::rotate(panRot, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                break;

            case Pos_Z: //+Z face
                rotMat = panRot;
                break;

            case Neg_Z: //-Z face
                mSubViewports[i].setEnabled(false);
                rotMat = glm::rotate(panRot, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                break;
            }//end switch

            //add viewplane vertices
            mSubViewports[i].getProjectionPlane()->setCoordinate(sgct_core::SGCTProjectionPlane::LowerLeft, glm::vec3(rotMat * lowerLeft));
            mSubViewports[i].getProjectionPlane()->setCoordinate(sgct_core::SGCTProjectionPlane::UpperLeft, glm::vec3(rotMat * upperLeft));
            mSubViewports[i].getProjectionPlane()->setCoordinate(sgct_core::SGCTProjectionPlane::UpperRight, glm::vec3(rotMat * upperRight));
        }//end for
    }//end if
}

void sgct_core::FisheyeProjection::initShaders()
{
    if (sgct::Engine::instance()->isOGLPipelineFixed())
    {
        mInternalRenderFn = &FisheyeProjection::renderInternalFixedPipeline;
        mInternalRenderCubemapFn = &FisheyeProjection::renderCubemapInternalFixedPipeline;
    }
    else
    {
        mInternalRenderFn = &FisheyeProjection::renderInternal;
        mInternalRenderCubemapFn = &FisheyeProjection::renderCubemapInternal;
    }
    
    if (mStereo || mPreferedMonoFrustumMode != Frustum::MonoEye) // if any frustum mode other than Mono (or stereo)
        mOffAxis = true;

    //reload shader program if it exists
    if (mShader.isLinked())
        mShader.deleteProgram();

    std::string fisheyeFragmentShader;
    std::string fisheyeVertexShader;

    bool isCubic = (mInterpolationMode == Cubic);

    if (sgct::Engine::instance()->isOGLPipelineFixed())
    {
        fisheyeVertexShader = isCubic ? sgct_core::shaders_fisheye_cubic::Fisheye_Vert_Shader : sgct_core::shaders_fisheye::Fisheye_Vert_Shader;

        if (mOffAxis)
        {
            if (sgct::SGCTSettings::instance()->useDepthTexture())
            {
                switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType())
                {
                case sgct::SGCTSettings::Diffuse:
                default:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_fisheye_cubic::Fisheye_Frag_Shader_OffAxis_Depth :
                        sgct_core::shaders_fisheye::Fisheye_Frag_Shader_OffAxis_Depth;
                    break;

                case sgct::SGCTSettings::Diffuse_Normal:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_fisheye_cubic::Fisheye_Frag_Shader_OffAxis_Depth_Normal :
                        sgct_core::shaders_fisheye::Fisheye_Frag_Shader_OffAxis_Depth_Normal;
                    break;

                case sgct::SGCTSettings::Diffuse_Position:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_fisheye_cubic::Fisheye_Frag_Shader_OffAxis_Depth_Position :
                        sgct_core::shaders_fisheye::Fisheye_Frag_Shader_OffAxis_Depth_Position;
                    break;

                case sgct::SGCTSettings::Diffuse_Normal_Position:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_fisheye_cubic::Fisheye_Frag_Shader_OffAxis_Depth_Normal_Position :
                        sgct_core::shaders_fisheye::Fisheye_Frag_Shader_OffAxis_Depth_Normal_Position;
                    break;
                }
            }
            else //no depth
            {
                switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType())
                {
                case sgct::SGCTSettings::Diffuse:
                default:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_fisheye_cubic::Fisheye_Frag_Shader_OffAxis :
                        sgct_core::shaders_fisheye::Fisheye_Frag_Shader_OffAxis;
                    break;

                case sgct::SGCTSettings::Diffuse_Normal:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_fisheye_cubic::Fisheye_Frag_Shader_OffAxis_Normal :
                        sgct_core::shaders_fisheye::Fisheye_Frag_Shader_OffAxis_Normal;
                    break;

                case sgct::SGCTSettings::Diffuse_Position:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_fisheye_cubic::Fisheye_Frag_Shader_OffAxis_Position :
                        sgct_core::shaders_fisheye::Fisheye_Frag_Shader_OffAxis_Position;
                    break;

                case sgct::SGCTSettings::Diffuse_Normal_Position:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_fisheye_cubic::Fisheye_Frag_Shader_OffAxis_Normal_Position :
                        sgct_core::shaders_fisheye::Fisheye_Frag_Shader_OffAxis_Normal_Position;
                    break;
                }
            }
        }
        else //not off-axis
        {
            if (sgct::SGCTSettings::instance()->useDepthTexture())
            {
                switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType())
                {
                case sgct::SGCTSettings::Diffuse:
                default:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_fisheye_cubic::Fisheye_Frag_Shader_Depth :
                        sgct_core::shaders_fisheye::Fisheye_Frag_Shader_Depth;
                    break;

                case sgct::SGCTSettings::Diffuse_Normal:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_fisheye_cubic::Fisheye_Frag_Shader_Depth_Normal :
                        sgct_core::shaders_fisheye::Fisheye_Frag_Shader_Depth_Normal;
                    break;

                case sgct::SGCTSettings::Diffuse_Position:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_fisheye_cubic::Fisheye_Frag_Shader_Depth_Position :
                        sgct_core::shaders_fisheye::Fisheye_Frag_Shader_Depth_Position;
                    break;

                case sgct::SGCTSettings::Diffuse_Normal_Position:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_fisheye_cubic::Fisheye_Frag_Shader_Depth_Normal_Position :
                        sgct_core::shaders_fisheye::Fisheye_Frag_Shader_Depth_Normal_Position;
                    break;
                }
            }
            else //no depth
            {
                switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType())
                {
                case sgct::SGCTSettings::Diffuse:
                default:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_fisheye_cubic::Fisheye_Frag_Shader :
                        sgct_core::shaders_fisheye::Fisheye_Frag_Shader;
                    break;

                case sgct::SGCTSettings::Diffuse_Normal:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_fisheye_cubic::Fisheye_Frag_Shader_Normal :
                        sgct_core::shaders_fisheye::Fisheye_Frag_Shader_Normal;
                    break;

                case sgct::SGCTSettings::Diffuse_Position:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_fisheye_cubic::Fisheye_Frag_Shader_Position :
                        sgct_core::shaders_fisheye::Fisheye_Frag_Shader_Position;
                    break;

                case sgct::SGCTSettings::Diffuse_Normal_Position:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_fisheye_cubic::Fisheye_Frag_Shader_Normal_Position :
                        sgct_core::shaders_fisheye::Fisheye_Frag_Shader_Normal_Position;
                    break;
                }
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
        fisheyeVertexShader = isCubic ? sgct_core::shaders_modern_fisheye_cubic::Fisheye_Vert_Shader : sgct_core::shaders_modern_fisheye::Fisheye_Vert_Shader;

        if (mOffAxis)
        {
            if (sgct::SGCTSettings::instance()->useDepthTexture())
            {
                switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType())
                {
                case sgct::SGCTSettings::Diffuse:
                default:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_modern_fisheye_cubic::Fisheye_Frag_Shader_OffAxis_Depth :
                        sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_OffAxis_Depth;
                    break;

                case sgct::SGCTSettings::Diffuse_Normal:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_modern_fisheye_cubic::Fisheye_Frag_Shader_OffAxis_Depth_Normal :
                        sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_OffAxis_Depth_Normal;
                    break;

                case sgct::SGCTSettings::Diffuse_Position:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_modern_fisheye_cubic::Fisheye_Frag_Shader_OffAxis_Depth_Position :
                        sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_OffAxis_Depth_Position;
                    break;

                case sgct::SGCTSettings::Diffuse_Normal_Position:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_modern_fisheye_cubic::Fisheye_Frag_Shader_OffAxis_Depth_Normal_Position :
                        sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_OffAxis_Depth_Normal_Position;
                    break;
                }
            }
            else //no depth
            {
                switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType())
                {
                case sgct::SGCTSettings::Diffuse:
                default:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_modern_fisheye_cubic::Fisheye_Frag_Shader_OffAxis :
                        sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_OffAxis;
                    break;

                case sgct::SGCTSettings::Diffuse_Normal:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_modern_fisheye_cubic::Fisheye_Frag_Shader_OffAxis_Normal :
                        sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_OffAxis_Normal;
                    break;

                case sgct::SGCTSettings::Diffuse_Position:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_modern_fisheye_cubic::Fisheye_Frag_Shader_OffAxis_Position :
                        sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_OffAxis_Position;
                    break;

                case sgct::SGCTSettings::Diffuse_Normal_Position:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_modern_fisheye_cubic::Fisheye_Frag_Shader_OffAxis_Normal_Position :
                        sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_OffAxis_Normal_Position;
                    break;
                }
            }
        }
        else//not off axis
        {
            if (sgct::SGCTSettings::instance()->useDepthTexture())
            {
                switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType())
                {
                case sgct::SGCTSettings::Diffuse:
                default:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_modern_fisheye_cubic::Fisheye_Frag_Shader_Depth :
                        sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_Depth;
                    break;

                case sgct::SGCTSettings::Diffuse_Normal:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_modern_fisheye_cubic::Fisheye_Frag_Shader_Depth_Normal :
                        sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_Depth_Normal;
                    break;

                case sgct::SGCTSettings::Diffuse_Position:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_modern_fisheye_cubic::Fisheye_Frag_Shader_Depth_Position :
                        sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_Depth_Position;
                    break;

                case sgct::SGCTSettings::Diffuse_Normal_Position:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_modern_fisheye_cubic::Fisheye_Frag_Shader_Depth_Normal_Position :
                        sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_Depth_Normal_Position;
                    break;
                }
            }
            else //no depth
            {
                switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType())
                {
                case sgct::SGCTSettings::Diffuse:
                default:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_modern_fisheye_cubic::Fisheye_Frag_Shader :
                        sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader;
                    break;

                case sgct::SGCTSettings::Diffuse_Normal:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_modern_fisheye_cubic::Fisheye_Frag_Shader_Normal :
                        sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_Normal;
                    break;

                case sgct::SGCTSettings::Diffuse_Position:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_modern_fisheye_cubic::Fisheye_Frag_Shader_Position :
                        sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_Position;
                    break;

                case sgct::SGCTSettings::Diffuse_Normal_Position:
                    fisheyeFragmentShader = isCubic ?
                        sgct_core::shaders_modern_fisheye_cubic::Fisheye_Frag_Shader_Normal_Position :
                        sgct_core::shaders_modern_fisheye::Fisheye_Frag_Shader_Normal_Position;
                    break;
                }
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
    if (mOffAxis)
        sgct_helpers::findAndReplace(fisheyeFragmentShader, "**sample_fun**", sgct::Engine::instance()->isOGLPipelineFixed() ? sgct_core::shaders_fisheye::sample_offset_fun : sgct_core::shaders_modern_fisheye::sample_offset_fun);
    else
        sgct_helpers::findAndReplace(fisheyeFragmentShader, "**sample_fun**", sgct::Engine::instance()->isOGLPipelineFixed() ? sgct_core::shaders_fisheye::sample_fun : sgct_core::shaders_modern_fisheye::sample_fun);

    if (isCubic)
    {
        std::stringstream ssRes;
        ssRes << mCubemapResolution << ".0";
        
        //add functions to shader
        sgct_helpers::findAndReplace(fisheyeFragmentShader, "**cubic_fun**", sgct::Engine::instance()->isOGLPipelineFixed() ? sgct_core::shaders_fisheye_cubic::catmull_rom_fun : sgct_core::shaders_modern_fisheye_cubic::catmull_rom_fun);
        sgct_helpers::findAndReplace(fisheyeFragmentShader, "**interpolatef**", sgct::Engine::instance()->isOGLPipelineFixed() ? sgct_core::shaders_fisheye_cubic::interpolate4_f : sgct_core::shaders_modern_fisheye_cubic::interpolate4_f);
        sgct_helpers::findAndReplace(fisheyeFragmentShader, "**interpolate3f**", sgct_core::shaders_modern_fisheye_cubic::interpolate4_3f);
        sgct_helpers::findAndReplace(fisheyeFragmentShader, "**interpolate4f**", sgct::Engine::instance()->isOGLPipelineFixed() ? sgct_core::shaders_fisheye_cubic::interpolate4_4f : sgct_core::shaders_modern_fisheye_cubic::interpolate4_4f);

        //set size
        sgct_helpers::findAndReplace(fisheyeFragmentShader, "**size**", ssRes.str());

        //set step
        sgct_helpers::findAndReplace(fisheyeFragmentShader, "**step**", "1.0");
    }

    //replace add correct transform in the fragment shader
    if (mMethod == FourFaceCube)
        sgct_helpers::findAndReplace(fisheyeFragmentShader, "**rotVec**", "vec3 rotVec = vec3( angle45Factor*x + angle45Factor*z, y, -angle45Factor*x + angle45Factor*z)");
    else //five or six face
        sgct_helpers::findAndReplace(fisheyeFragmentShader, "**rotVec**", "vec3 rotVec = vec3(angle45Factor*x - angle45Factor*y, angle45Factor*x + angle45Factor*y, z)");

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

    mShader.setName("FisheyeShader");
    mShader.createAndLinkProgram();
    mShader.bind();

    mCubemapLoc = mShader.getUniformLocation("cubemap");
    glUniform1i(mCubemapLoc, 0);

    if (sgct::SGCTSettings::instance()->useDepthTexture())
    {
        mDepthCubemapLoc = mShader.getUniformLocation("depthmap");
        glUniform1i(mDepthCubemapLoc, 1);
    }

    if (sgct::SGCTSettings::instance()->useNormalTexture())
    {
        mNormalCubemapLoc = mShader.getUniformLocation("normalmap");
        glUniform1i(mNormalCubemapLoc, 2);
    }
    
    if (sgct::SGCTSettings::instance()->usePositionTexture())
    {
        mPositionCubemapLoc = mShader.getUniformLocation("positionmap");
        glUniform1i(mPositionCubemapLoc, 3);
    }

    mHalfFovLoc = mShader.getUniformLocation("halfFov");
    glUniform1f(mHalfFovLoc, glm::half_pi<float>());

    if (mOffAxis)
    {
        mOffsetLoc = mShader.getUniformLocation("offset");
        glUniform3f(mOffsetLoc,
            mTotalOffset.x,
            mTotalOffset.y,
            mTotalOffset.z);
    }

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

void sgct_core::FisheyeProjection::updateGeomerty(const float & width, const float & height)
{
    //create proxy geometry
    float leftcrop = mCropFactors[CropLeft];
    float rightcrop = mCropFactors[CropRight];
    float bottomcrop = mCropFactors[CropBottom];
    float topcrop = mCropFactors[CropTop];

    float cropAspect = ((1.0f - 2.0f * bottomcrop) + (1.0f - 2.0f*topcrop)) / ((1.0f - 2.0f*leftcrop) + (1.0f - 2.0f*rightcrop));

    float x = 1.0f;
    float y = 1.0f;

    float frameBufferAspect = mIgnoreAspectRatio ? 1.0f : width/height;

    if (sgct::SGCTSettings::instance()->getTryMaintainAspectRatio())
    {
        float aspect = frameBufferAspect * cropAspect;
        (aspect >= 1.0f) ? x = 1.0f / aspect : y = aspect;
    }

    mVerts[0] = leftcrop;
    mVerts[1] = bottomcrop;
    mVerts[2] = -x;
    mVerts[3] = -y;
    mVerts[4] = -1.0f;

    mVerts[5] = leftcrop;
    mVerts[6] = 1.0f - topcrop;
    mVerts[7] = -x;
    mVerts[8] = y;
    mVerts[9] = -1.0f;

    mVerts[10] = 1.0f - rightcrop;
    mVerts[11] = bottomcrop;
    mVerts[12] = x;
    mVerts[13] = -y;
    mVerts[14] = -1.0f;

    mVerts[15] = 1.0f - rightcrop;
    mVerts[16] = 1.0f - topcrop;
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

void sgct_core::FisheyeProjection::drawCubeFace(const std::size_t & face)
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

void sgct_core::FisheyeProjection::blitCubeFace(const int & face)
{
    //copy AA-buffer to "regular"/non-AA buffer
    mCubeMapFBO_Ptr->bindBlit(); //bind separate read and draw buffers to prepare blit operation
    attachTextures(face);
    mCubeMapFBO_Ptr->blit();
}

void sgct_core::FisheyeProjection::attachTextures(const int & face)
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

void sgct_core::FisheyeProjection::renderInternal()
{
    glEnable(GL_SCISSOR_TEST);
    sgct::Engine::mInstance->enterCurrentViewport();
    glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    bindShaderProgram();

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    //if for some reson the active texture has been reset
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures[CubeMapColor]);

    if (sgct::SGCTSettings::instance()->useDepthTexture())
    {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures[CubeMapDepth]);
        glUniform1i(mDepthCubemapLoc, 1);
    }

    if (sgct::SGCTSettings::instance()->useNormalTexture())
    {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures[CubeMapNormals]);
        glUniform1i(mNormalCubemapLoc, 2);
    }

    if (sgct::SGCTSettings::instance()->usePositionTexture())
    {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures[CubeMapPositions]);
        glUniform1i(mPositionCubemapLoc, 3);
    }

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
    glUniform1f(mHalfFovLoc, glm::radians<float>(mFOV / 2.0f));

    if (mOffAxis)
    {
        glUniform3f(mOffsetLoc,
            mTotalOffset.x,
            mTotalOffset.y,
            mTotalOffset.z);
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
}

void sgct_core::FisheyeProjection::renderInternalFixedPipeline()
{
    glEnable(GL_SCISSOR_TEST);
    sgct::Engine::mInstance->enterCurrentViewport();
    glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

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

    if (sgct::SGCTSettings::instance()->useDepthTexture())
    {
        glActiveTexture(GL_TEXTURE1);
        glEnable(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures[CubeMapDepth]);
        glUniform1i(mDepthCubemapLoc, 1);
    }

    if (sgct::SGCTSettings::instance()->useNormalTexture())
    {
        glActiveTexture(GL_TEXTURE2);
        glEnable(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures[CubeMapNormals]);
        glUniform1i(mNormalCubemapLoc, 2);
    }

    if (sgct::SGCTSettings::instance()->usePositionTexture())
    {
        glActiveTexture(GL_TEXTURE3);
        glEnable(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures[CubeMapPositions]);
        glUniform1i(mPositionCubemapLoc, 3);
    }

    glDisable(GL_CULL_FACE);
    bool alpha = sgct::Engine::mInstance->getCurrentWindowPtr()->getAlpha();
    if (alpha)
    {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
    }
    else{}
        glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    glUniform1i(mCubemapLoc, 0);
    glUniform1f(mHalfFovLoc, glm::radians<float>(mFOV / 2.0f));

    if (mOffAxis)
    {
        glUniform3f(mOffsetLoc,
            mTotalOffset.x,
            mTotalOffset.y,
            mTotalOffset.z);
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

    glPopClientAttrib();
    glPopAttrib();
}

void sgct_core::FisheyeProjection::renderCubemapInternal(std::size_t * subViewPortIndex)
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
        }//end if viewport is enabled
    }//end for
}

void sgct_core::FisheyeProjection::renderCubemapInternalFixedPipeline(std::size_t * subViewPortIndex)
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
                    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
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
        }//end if viewport is enabled
    }//end for
}