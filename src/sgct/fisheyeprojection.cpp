/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/fisheyeprojection.h>

#include <sgct/engine.h>
#include <sgct/messageHandler.h>
#include <sgct/settings.h>
#include <sgct/user.h>
#include <sgct/window.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/helpers/stringfunctions.h>
#include <sgct/shaders/internalfisheyeshaders.h>
#include <sgct/shaders/internalfisheyeshaders_modern.h>
#include <sgct/shaders/internalfisheyeshaders_cubic.h>
#include <sgct/shaders/internalfisheyeshaders_modern_cubic.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>

//#define DebugCubemap

namespace sgct::core {

void FisheyeProjection::update(glm::vec2 size) {
    float cropAspect =
        ((1.f - 2.f * mCropFactor.bottom) + (1.f - 2.f * mCropFactor.top)) /
        ((1.f - 2.f * mCropFactor.left) + (1.f - 2.f * mCropFactor.right));

    float x = 1.f;
    float y = 1.f;

    float frameBufferAspect = mIgnoreAspectRatio ? 1.f : (size.x / size.y);

    if (Settings::instance()->getTryMaintainAspectRatio()) {
        float aspect = frameBufferAspect * cropAspect;
        if (aspect >= 1.f) {
            x = 1.f / aspect;
        }
        else {
            y = aspect;
        }
    }

    mVerts[0] = mCropFactor.left;
    mVerts[1] = mCropFactor.bottom;
    mVerts[2] = -x;
    mVerts[3] = -y;
    mVerts[4] = -1.f;

    mVerts[5] = mCropFactor.left;
    mVerts[6] = 1.f - mCropFactor.top;
    mVerts[7] = -x;
    mVerts[8] = y;
    mVerts[9] = -1.f;

    mVerts[10] = 1.f - mCropFactor.right;
    mVerts[11] = mCropFactor.bottom;
    mVerts[12] = x;
    mVerts[13] = -y;
    mVerts[14] = -1.f;

    mVerts[15] = 1.f - mCropFactor.right;
    mVerts[16] = 1.f - mCropFactor.top;
    mVerts[17] = x;
    mVerts[18] = y;
    mVerts[19] = -1.f;

    // update VBO
    if (!Engine::instance()->isOGLPipelineFixed()) {
        glBindVertexArray(mVAO);
    }
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);

    GLvoid* PositionBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    memcpy(PositionBuffer, mVerts.data(), 20 * sizeof(float));
    glUnmapBuffer(GL_ARRAY_BUFFER);

    if (!Engine::instance()->isOGLPipelineFixed()) {
        glBindVertexArray(0);
    }
    else {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void FisheyeProjection::render() {
    if (Engine::instance()->isOGLPipelineFixed()) {
        renderInternalFixedPipeline();
    }
    else {
        renderInternal();
    }
}

void FisheyeProjection::renderCubemap(std::size_t* subViewPortIndex) {
    const Engine& eng = *Engine::instance();
    switch (Engine::instance()->getCurrentFrustumMode()) {
        default:
            break;
        case Frustum::Mode::StereoLeftEye:
            setOffset(glm::vec3(
                -eng.getDefaultUser().getEyeSeparation() / mDiameter,
                0.f,
                0.f
            ));
            break;
        case Frustum::Mode::StereoRightEye:
            setOffset(glm::vec3(
                eng.getDefaultUser().getEyeSeparation() / mDiameter,
                0.f,
                0.f
            ));
            break;
    }

    if (Engine::instance()->isOGLPipelineFixed()) {
        renderCubemapInternalFixedPipeline(subViewPortIndex);
    }
    else {
        renderCubemapInternal(subViewPortIndex);
    }
}

void FisheyeProjection::setDomeDiameter(float diameter) {
    mDiameter = diameter;
    //generateCubeMapViewports();
}

void FisheyeProjection::setTilt(float angle) {
    mTilt = angle;
}

void FisheyeProjection::setFOV(float angle) {
    mFOV = angle;
}

void FisheyeProjection::setRenderingMethod(FisheyeMethod method) {
    mMethod = method;
}

void FisheyeProjection::setCropFactors(float left, float right, float bottom, float top) {
    mCropFactor.left = glm::clamp(left, 0.f, 1.f);
    mCropFactor.right = glm::clamp(right, 0.f, 1.f);
    mCropFactor.bottom = glm::clamp(bottom, 0.f, 1.f);
    mCropFactor.top = glm::clamp(top, 0.f, 1.f);
}

void FisheyeProjection::setOffset(glm::vec3 offset) {
    mOffset = std::move(offset);
    mTotalOffset = mBaseOffset + mOffset;

    mOffAxis = glm::length(mTotalOffset) > 0.f;
}

void FisheyeProjection::setBaseOffset(glm::vec3 offset) {
    mBaseOffset = std::move(offset);
    mTotalOffset = mBaseOffset + mOffset;

    mOffAxis = glm::length(mTotalOffset) > 0.f;
}

void FisheyeProjection::setIgnoreAspectRatio(bool state) {
    mIgnoreAspectRatio = state;
}

glm::vec3 FisheyeProjection::getOffset() const {
    return mTotalOffset;
}

void FisheyeProjection::initViewports() {
    // radius is needed to calculate the distance to all view planes
    const float radius = mDiameter / 2.f;

    // setup base viewport that will be rotated to create the other cubemap views
    // +Z face
    const glm::vec4 lowerLeftBase(-radius, -radius, radius, 1.f);
    const glm::vec4 upperLeftBase(-radius, radius, radius, 1.f);
    const glm::vec4 upperRightBase(radius, radius, radius, 1.f);

    // 250.5287794 degree FOV covers exactly five sides of a cube, larger FOV needs six
    const float fiveFaceLimit = 2.f * glm::degrees(acos(-1.f / sqrt(3.f)));
    // 109.4712206 degree FOV is needed to cover the entire top face
    const float topFaceLimit = 2.f * glm::degrees(acos(1.f / sqrt(3.f)));


    // four faces doesn't cover more than 180 degrees
    if (mFOV > 180.f && mFOV <= fiveFaceLimit) {
        mMethod = FisheyeMethod::FiveFaceCube;
    }

    float cropLevel = 0.5f; // how much of the side faces that are used
    float projectionOffset = 0.f;
    if (mMethod == FisheyeMethod::FiveFaceCube &&
        mFOV >= topFaceLimit && mFOV <= fiveFaceLimit)
    {
        float cosAngle = cos(glm::radians(mFOV / 2.f));
        float normalizedProjectionOffset = 0.f;
        if (mFOV < 180.f) {
            normalizedProjectionOffset = 1.f - mFOV / 180.f; // [-1, 0]
        }
        else {
            normalizedProjectionOffset = sqrt((2.f * cosAngle * cosAngle) /
                                              (1.f - cosAngle * cosAngle)); // [0, 1]
        }

        projectionOffset = normalizedProjectionOffset * radius;
        cropLevel = (1.f - normalizedProjectionOffset) / 2.f;
    }
    else if (mFOV > fiveFaceLimit) {
        mMethod = FisheyeMethod::SixFaceCube;
        cropLevel = 0.f;
        projectionOffset = radius;
    }

    const glm::mat4 tiltMat = glm::rotate(
        glm::mat4(1.f),
        glm::radians(90.f - mTilt),
        glm::vec3(1.f, 0.f, 0.f)
    );

    if (mMethod == FisheyeMethod::FiveFaceCube || mMethod == FisheyeMethod::SixFaceCube) {
        const glm::mat4 rollRot = glm::rotate(
            tiltMat,
            glm::radians(45.f),
            glm::vec3(0.f, 0.f, 1.f)
        );

        // +X face
        {
            mSubViewports.right.setName("Fisheye +X");

            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(-90.f),
                glm::vec3(0.f, 1.f, 0.f)
            );

            glm::vec4 lowerLeft = lowerLeftBase;
            glm::vec4 upperLeft = upperLeftBase;
            glm::vec4 upperRight = upperRightBase;
            upperRight.x = projectionOffset;

            mSubViewports.right.setSize(glm::vec2(1.f - cropLevel, 1.f));

            mSubViewports.right.getProjectionPlane().setCoordinateLowerLeft(
                glm::vec3(rotMat * lowerLeft)
            );
            mSubViewports.right.getProjectionPlane().setCoordinateUpperLeft(
                glm::vec3(rotMat * upperLeft)
            );
            mSubViewports.right.getProjectionPlane().setCoordinateUpperRight(
                glm::vec3(rotMat * upperRight)
            );
        }

        // -X face
        {
            mSubViewports.left.setName("Fisheye -X");

            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(90.f),
                glm::vec3(0.f, 1.f, 0.f)
            );

            glm::vec4 lowerLeft = lowerLeftBase;
            lowerLeft.x = -projectionOffset;
            glm::vec4 upperLeft = upperLeftBase;
            upperLeft.x = -projectionOffset;
            glm::vec4 upperRight = upperRightBase;

            mSubViewports.left.setPos(glm::vec2(cropLevel, 0.f));
            mSubViewports.left.setSize(glm::vec2(1.f - cropLevel, 1.f));

            mSubViewports.left.getProjectionPlane().setCoordinateLowerLeft(
                glm::vec3(rotMat * lowerLeft)
            );
            mSubViewports.left.getProjectionPlane().setCoordinateUpperLeft(
                glm::vec3(rotMat * upperLeft)
            );
            mSubViewports.left.getProjectionPlane().setCoordinateUpperRight(
                glm::vec3(rotMat * upperRight)
            );
        }

        // +Y face
        {
            mSubViewports.bottom.setName("Fisheye +Y");
            
            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(-90.f),
                glm::vec3(1.f, 0.f, 0.f)
            );

            glm::vec4 lowerLeft = lowerLeftBase;
            lowerLeft.y = -projectionOffset;
            glm::vec4 upperLeft = upperLeftBase;
            glm::vec4 upperRight = upperRightBase;

            mSubViewports.bottom.setPos(glm::vec2(0.f, cropLevel));
            mSubViewports.bottom.setSize(glm::vec2(1.f, 1.f - cropLevel));

            mSubViewports.bottom.getProjectionPlane().setCoordinateLowerLeft(
                glm::vec3(rotMat * lowerLeft)
            );
            mSubViewports.bottom.getProjectionPlane().setCoordinateUpperLeft(
                glm::vec3(rotMat * upperLeft)
            );
            mSubViewports.bottom.getProjectionPlane().setCoordinateUpperRight(
                glm::vec3(rotMat * upperRight)
            );
        }

        // -Y face
        {
            mSubViewports.top.setName("Fisheye -Y");
            
            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(90.f),
                glm::vec3(1.f, 0.f, 0.f)
            );

            glm::vec4 lowerLeft = lowerLeftBase;
            glm::vec4 upperLeft = upperLeftBase;
            upperLeft.y = projectionOffset;
            glm::vec4 upperRight = upperRightBase;
            upperRight.y = projectionOffset;

            mSubViewports.top.setSize(glm::vec2(1.f, 1.f - cropLevel));

            mSubViewports.top.getProjectionPlane().setCoordinateLowerLeft(
                glm::vec3(rotMat * lowerLeft)
            );
            mSubViewports.top.getProjectionPlane().setCoordinateUpperLeft(
                glm::vec3(rotMat * upperLeft)
            );
            mSubViewports.top.getProjectionPlane().setCoordinateUpperRight(
                glm::vec3(rotMat * upperRight)
            );
        }

        // +Z face
        {
            mSubViewports.front.setName("Fisheye +Z");
            
            mSubViewports.front.getProjectionPlane().setCoordinateLowerLeft(
                glm::vec3(rollRot * lowerLeftBase)
            );
            mSubViewports.front.getProjectionPlane().setCoordinateUpperLeft(
                glm::vec3(rollRot * upperLeftBase)
            );
            mSubViewports.front.getProjectionPlane().setCoordinateUpperRight(
                glm::vec3(rollRot * upperRightBase)
            );
        }

        // -Z face
        {
            mSubViewports.back.setName("Fisheye -Z");
            if (mMethod == FisheyeMethod::FiveFaceCube) {
                mSubViewports.back.setEnabled(false);
            }

            const glm::mat4 rotMat = glm::rotate(
                rollRot,
                glm::radians(180.f),
                glm::vec3(0.f, 1.f, 0.f)
            );

            mSubViewports.back.getProjectionPlane().setCoordinateLowerLeft(
                glm::vec3(rotMat * lowerLeftBase)
            );
            mSubViewports.back.getProjectionPlane().setCoordinateUpperLeft(
                glm::vec3(rotMat * upperLeftBase)
            );
            mSubViewports.back.getProjectionPlane().setCoordinateUpperRight(
                glm::vec3(rotMat * upperRightBase)
            );
        }
    }
    else {
        const glm::mat4 panRot = glm::rotate(
            tiltMat,
            glm::radians(45.f),
            glm::vec3(0.f, 1.f, 0.f)
        );


        // +X face
        {
            mSubViewports.right.setName("Fisheye +X");
            
            const glm::mat4 rotMat = glm::rotate(
                panRot,
                glm::radians(-90.f),
                glm::vec3(0.f, 1.f, 0.f)
            );

            mSubViewports.right.getProjectionPlane().setCoordinateLowerLeft(
                glm::vec3(rotMat * lowerLeftBase)
            );
            mSubViewports.right.getProjectionPlane().setCoordinateUpperLeft(
                glm::vec3(rotMat * upperLeftBase)
            );
            mSubViewports.right.getProjectionPlane().setCoordinateUpperRight(
                glm::vec3(rotMat * upperRightBase)
            );
        }

        // -X face
        {
            mSubViewports.left.setName("Fisheye -X");
            mSubViewports.left.setEnabled(false);

            const glm::mat4 rotMat = glm::rotate(
                panRot,
                glm::radians(90.f),
                glm::vec3(0.f, 1.f, 0.f)
            );

            mSubViewports.left.getProjectionPlane().setCoordinateLowerLeft(
                glm::vec3(rotMat * lowerLeftBase)
            );
            mSubViewports.left.getProjectionPlane().setCoordinateUpperLeft(
                glm::vec3(rotMat * upperLeftBase)
            );
            mSubViewports.left.getProjectionPlane().setCoordinateUpperRight(
                glm::vec3(rotMat * upperRightBase)
            );
        }

        // +Y face
        {
            mSubViewports.bottom.setName("Fisheye +Y");

            const glm::mat4 rotMat = glm::rotate(
                panRot,
                glm::radians(-90.f),
                glm::vec3(1.f, 0.f, 0.f)
            );

            mSubViewports.bottom.getProjectionPlane().setCoordinateLowerLeft(
                glm::vec3(rotMat * lowerLeftBase)
            );
            mSubViewports.bottom.getProjectionPlane().setCoordinateUpperLeft(
                glm::vec3(rotMat * upperLeftBase)
            );
            mSubViewports.bottom.getProjectionPlane().setCoordinateUpperRight(
                glm::vec3(rotMat * upperRightBase)
            );
        }

        // -Y face
        {
            mSubViewports.top.setName("Fisheye -Y");
            
            const glm::mat4 rotMat = glm::rotate(
                panRot,
                glm::radians(90.f),
                glm::vec3(1.f, 0.f, 0.f)
            );

            mSubViewports.top.getProjectionPlane().setCoordinateLowerLeft(
                glm::vec3(rotMat * lowerLeftBase)
            );
            mSubViewports.top.getProjectionPlane().setCoordinateUpperLeft(
                glm::vec3(rotMat * upperLeftBase)
            );
            mSubViewports.top.getProjectionPlane().setCoordinateUpperRight(
                glm::vec3(rotMat * upperRightBase)
            );
        }

        // +Z face
        {
            mSubViewports.front.setName("Fisheye +Z");
            
            mSubViewports.front.getProjectionPlane().setCoordinateLowerLeft(
                glm::vec3(panRot * lowerLeftBase)
            );
            mSubViewports.front.getProjectionPlane().setCoordinateUpperLeft(
                glm::vec3(panRot * upperLeftBase)
            );
            mSubViewports.front.getProjectionPlane().setCoordinateUpperRight(
                glm::vec3(panRot * upperRightBase)
            );

        }

        // -Z face
        {
            mSubViewports.back.setName("Fisheye -Z");
            
            mSubViewports.back.setEnabled(false);
            glm::vec4 lowerLeft = lowerLeftBase;
            glm::vec4 upperLeft = upperLeftBase;
            glm::vec4 upperRight = upperRightBase;

            const glm::mat4 rotMat = glm::rotate(
                panRot,
                glm::radians(180.f),
                glm::vec3(0.f, 1.f, 0.f)
            );

            mSubViewports.back.getProjectionPlane().setCoordinateLowerLeft(
                glm::vec3(rotMat * lowerLeft)
            );
            mSubViewports.back.getProjectionPlane().setCoordinateUpperLeft(
                glm::vec3(rotMat * upperLeft)
            );
            mSubViewports.back.getProjectionPlane().setCoordinateUpperRight(
                glm::vec3(rotMat * upperRight)
            );
        }
    }
}

void FisheyeProjection::initShaders() {
    if (mStereo || mPreferedMonoFrustumMode != Frustum::Mode::MonoEye) {
        // if any frustum mode other than Mono (or stereo)
        mOffAxis = true;
    }

    // reload shader program if it exists
    if (mShader.isLinked()) {
        mShader.deleteProgram();
    }

    std::string fisheyeFragmentShader;
    std::string fisheyeVertexShader;

    const bool isCubic = (mInterpolationMode == InterpolationMode::Cubic);

    if (Engine::instance()->isOGLPipelineFixed()) {
        fisheyeVertexShader = isCubic ?
            shaders_fisheye_cubic::FisheyeVert : 
            shaders_fisheye::FisheyeVert;

        if (mOffAxis) {
            if (Settings::instance()->useDepthTexture()) {
                switch (Settings::instance()->getCurrentDrawBufferType()) {
                    case Settings::DrawBufferType::Diffuse:
                    default:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragOffAxisDepth :
                            shaders_fisheye::FisheyeFragOffAxisDepth;
                        break;
                    case Settings::DrawBufferType::DiffuseNormal:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragOffAxisDepthNormal :
                            shaders_fisheye::FisheyeFragOffAxisDepthNormal;
                        break;
                    case Settings::DrawBufferType::DiffusePosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragOffAxisDepthPosition :
                            shaders_fisheye::FisheyeFragOffAxisDepthPosition;
                        break;
                    case Settings::DrawBufferType::DiffuseNormalPosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragOffAxisDepthNormalPosition :
                            shaders_fisheye::FisheyeFragOffAxisDepthNormalPosition;
                        break;
                }
            }
            else  {
                // no depth
                switch (Settings::instance()->getCurrentDrawBufferType()) {
                    case Settings::DrawBufferType::Diffuse:
                    default:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragOffAxis :
                            shaders_fisheye::FisheyeFragOffAxis;
                        break;
                    case Settings::DrawBufferType::DiffuseNormal:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragOffAxisNormal :
                            shaders_fisheye::FisheyeFragOffAxisNormal;
                        break;
                    case Settings::DrawBufferType::DiffusePosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragOffAxisPosition :
                            shaders_fisheye::FisheyeFragOffAxisPosition;
                        break;
                    case Settings::DrawBufferType::DiffuseNormalPosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragOffAxisNormalPosition :
                            shaders_fisheye::FisheyeFragOffAxisNormalPosition;
                        break;
                }
            }
        }
        else {
            // not off-axis
            if (Settings::instance()->useDepthTexture()) {
                switch (Settings::instance()->getCurrentDrawBufferType()) {
                    case Settings::DrawBufferType::Diffuse:
                    default:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragDepth :
                            shaders_fisheye::FisheyeFragDepth;
                        break;
                    case Settings::DrawBufferType::DiffuseNormal:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragDepthNormal :
                            shaders_fisheye::FisheyeFragDepthNormal;
                        break;
                    case Settings::DrawBufferType::DiffusePosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragDepthPosition :
                            shaders_fisheye::FisheyeFragDepthPosition;
                        break;
                    case Settings::DrawBufferType::DiffuseNormalPosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragDepthNormalPosition :
                            shaders_fisheye::FisheyeFragDepthNormalPosition;
                        break;
                }
            }
            else {
                // no depth
                switch (Settings::instance()->getCurrentDrawBufferType()) {
                    case Settings::DrawBufferType::Diffuse:
                    default:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFrag :
                            shaders_fisheye::FisheyeFrag;
                        break;
                    case Settings::DrawBufferType::DiffuseNormal:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragNormal :
                            shaders_fisheye::FisheyeFragNormal;
                        break;
                    case Settings::DrawBufferType::DiffusePosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragPosition :
                            shaders_fisheye::FisheyeFragPosition;
                        break;
                    case Settings::DrawBufferType::DiffuseNormalPosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragNormalPosition :
                            shaders_fisheye::FisheyeFragNormalPosition;
                        break;
                }
            }
        }

        // depth correction shader only
        if (Settings::instance()->useDepthTexture()) {
            std::string depthCorrFragShader = shaders_fisheye::BaseVert;
            std::string depthCorrVertShader =
                shaders_fisheye::FisheyeDepthCorrectionFrag;

            const std::string glsl = Engine::instance()->getGLSLVersion();
            helpers::findAndReplace(depthCorrFragShader, "**glsl_version**", glsl);
            helpers::findAndReplace(depthCorrVertShader, "**glsl_version**", glsl);

            bool depthCorrFrag = mDepthCorrectionShader.addShaderSrc(
                depthCorrFragShader,
                GL_VERTEX_SHADER,
                ShaderProgram::ShaderSourceType::String
            );
            if (!depthCorrFrag) {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Error,
                    "Failed to load fisheye depth correction vertex shader\n"
                );
            }
            bool depthCorrVert = mDepthCorrectionShader.addShaderSrc(
                depthCorrVertShader,
                GL_FRAGMENT_SHADER,
                ShaderProgram::ShaderSourceType::String
            );
            if (!depthCorrVert) {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Error,
                    "Failed to load fisheye depth correction fragment shader\n"
                );
            }
        }
    }
    else {
        // modern pipeline
        fisheyeVertexShader = isCubic ?
            shaders_modern_fisheye_cubic::FisheyeVert :
            shaders_modern_fisheye::FisheyeVert;

        if (mOffAxis) {
            if (Settings::instance()->useDepthTexture()) {
                switch (Settings::instance()->getCurrentDrawBufferType()) {
                    case Settings::DrawBufferType::Diffuse:
                    default:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragOffAxisDepth :
                            shaders_modern_fisheye::FisheyeFragOffAxisDepth;
                        break;
                    case Settings::DrawBufferType::DiffuseNormal:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragOffAxisDepthNormal :
                            shaders_modern_fisheye::FisheyeFragOffAxisDepthNormal;
                        break;
                    case Settings::DrawBufferType::DiffusePosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragOffAxisDepthPosition :
                            shaders_modern_fisheye::FisheyeFragOffAxisDepthPosition;
                        break;
                    case Settings::DrawBufferType::DiffuseNormalPosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragOffAxisDepthNormalPosition :
                            shaders_modern_fisheye::FisheyeFragOffAxisDepthNormalPosition;
                        break;
                }
            }
            else {
                // no depth
                switch (Settings::instance()->getCurrentDrawBufferType()) {
                    case Settings::DrawBufferType::Diffuse:
                    default:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragOffAxis :
                            shaders_modern_fisheye::FisheyeFragOffAxis;
                        break;
                    case Settings::DrawBufferType::DiffuseNormal:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragOffAxisNormal :
                            shaders_modern_fisheye::FisheyeFragOffAxisNormal;
                        break;
                    case Settings::DrawBufferType::DiffusePosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragOffAxisPosition :
                            shaders_modern_fisheye::FisheyeFragOffAxisPosition;
                        break;
                    case Settings::DrawBufferType::DiffuseNormalPosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragOffAxisNormalPosition :
                            shaders_modern_fisheye::FisheyeFragOffAxisNormalPosition;
                        break;
                }
            }
        }
        else {
            // not off axis
            if (Settings::instance()->useDepthTexture()) {
                switch (Settings::instance()->getCurrentDrawBufferType()) {
                    case Settings::DrawBufferType::Diffuse:
                    default:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragDepth :
                            shaders_modern_fisheye::FisheyeFragDepth;
                        break;
                    case Settings::DrawBufferType::DiffuseNormal:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragDepthNormal :
                            shaders_modern_fisheye::FisheyeFragDepthNormal;
                        break;
                    case Settings::DrawBufferType::DiffusePosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragDepthPosition :
                            shaders_modern_fisheye::FisheyeFragDepthPosition;
                        break;
                    case Settings::DrawBufferType::DiffuseNormalPosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragDepthNormalPosition :
                            shaders_modern_fisheye::FisheyeFragDepthNormalPosition;
                        break;
                }
            }
            else {
                // no depth
                switch (Settings::instance()->getCurrentDrawBufferType()) {
                    case Settings::DrawBufferType::Diffuse:
                    default:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFrag :
                            shaders_modern_fisheye::FisheyeFrag;
                        break;
                    case Settings::DrawBufferType::DiffuseNormal:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragNormal :
                            shaders_modern_fisheye::FisheyeFragNormal;
                        break;
                    case Settings::DrawBufferType::DiffusePosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragPosition :
                            shaders_modern_fisheye::FisheyeFragPosition;
                        break;
                    case Settings::DrawBufferType::DiffuseNormalPosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragNormalPosition :
                            shaders_modern_fisheye::FisheyeFragNormalPosition;
                        break;
                }
            }
        }

        // depth correction shader only
        if (Settings::instance()->useDepthTexture()) {
            std::string depthCorrFragShader = shaders_modern_fisheye::BaseVert;
            std::string depthCorrVertShader =
                shaders_modern_fisheye::FisheyeDepthCorrectionFrag;

            const std::string glsl = Engine::instance()->getGLSLVersion();
            // replace glsl version
            helpers::findAndReplace(depthCorrFragShader, "**glsl_version**", glsl);
            helpers::findAndReplace(depthCorrVertShader, "**glsl_version**", glsl);

            bool depthCorrFrag = mDepthCorrectionShader.addShaderSrc(
                depthCorrFragShader,
                GL_VERTEX_SHADER,
                ShaderProgram::ShaderSourceType::String
            );
            if (!depthCorrFrag) {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Error,
                    "Failed to load fisheye depth correction vertex shader\n"
                );
            }
            bool depthCorrVert = mDepthCorrectionShader.addShaderSrc(
                depthCorrVertShader,
                GL_FRAGMENT_SHADER,
                ShaderProgram::ShaderSourceType::String
            );
            if (!depthCorrVert) {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Error,
                    "Failed to load fisheye depth correction fragment shader\n"
                );
            }
        }
    }

    // add functions to shader
    if (mOffAxis) {
        helpers::findAndReplace(
            fisheyeFragmentShader,
            "**sample_fun**",
            Engine::instance()->isOGLPipelineFixed() ?
                shaders_fisheye::SampleOffsetFun :
                shaders_modern_fisheye::SampleOffsetFun
        );
    }
    else {
        helpers::findAndReplace(
            fisheyeFragmentShader,
            "**sample_fun**",
            Engine::instance()->isOGLPipelineFixed() ?
                shaders_fisheye::SampleFun :
                shaders_modern_fisheye::SampleFun
        );
    }

    if (isCubic) {
        // add functions to shader
        helpers::findAndReplace(
            fisheyeFragmentShader, "**cubic_fun**",
            Engine::instance()->isOGLPipelineFixed() ?
                shaders_fisheye_cubic::catmullRomFun :
                shaders_modern_fisheye_cubic::catmullRomFun
        );
        helpers::findAndReplace(
            fisheyeFragmentShader, "**interpolatef**",
            Engine::instance()->isOGLPipelineFixed() ?
                shaders_fisheye_cubic::interpolate4_f :
                shaders_modern_fisheye_cubic::interpolate4_f
        );
        helpers::findAndReplace(
            fisheyeFragmentShader, "**interpolate3f**",
            shaders_modern_fisheye_cubic::interpolate4_3f
        );
        helpers::findAndReplace(
            fisheyeFragmentShader, "**interpolate4f**",
            Engine::instance()->isOGLPipelineFixed() ?
                shaders_fisheye_cubic::interpolate4_4f :
                shaders_modern_fisheye_cubic::interpolate4_4f
        );

        helpers::findAndReplace(
            fisheyeFragmentShader,
            "**size**",
            std::to_string(mCubemapResolution) + ".0"
        );

        helpers::findAndReplace(fisheyeFragmentShader, "**step**", "1.0");
    }

    // replace add correct transform in the fragment shader
    if (mMethod == FisheyeMethod::FourFaceCube) {
        helpers::findAndReplace(
            fisheyeFragmentShader,
            "**rotVec**",
            "vec3 rotVec = vec3(angle45Factor*x + angle45Factor*z, y, "
            "-angle45Factor*x + angle45Factor*z)"
        );
    }
    else {
        // five or six face
        helpers::findAndReplace(
            fisheyeFragmentShader,
            "**rotVec**",
            "vec3 rotVec = vec3(angle45Factor*x - angle45Factor*y, "
            "angle45Factor*x + angle45Factor*y, z)"
        );
    }

    // replace glsl version
    helpers::findAndReplace(
        fisheyeVertexShader,
        "**glsl_version**",
        Engine::instance()->getGLSLVersion()
    );
    helpers::findAndReplace(
        fisheyeFragmentShader,
        "**glsl_version**",
        Engine::instance()->getGLSLVersion()
    );

    // replace color
    std::stringstream ssColor;
    ssColor.precision(2);
    ssColor << std::fixed << "vec4(" << mClearColor.r << ", " << mClearColor.g
            << ", " << mClearColor.b << ", " << mClearColor.a << ")";
    helpers::findAndReplace(fisheyeFragmentShader, "**bgColor**", ssColor.str());

    bool fisheyeVertex = mShader.addShaderSrc(
        fisheyeVertexShader,
        GL_VERTEX_SHADER,
        ShaderProgram::ShaderSourceType::String
    );
    if (!fisheyeVertex) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Failed to load fisheye vertex shader:\n%s\n",
            fisheyeVertexShader.c_str()
        );
    }
    bool fisheyeFragment = mShader.addShaderSrc(
        fisheyeFragmentShader,
        GL_FRAGMENT_SHADER,
        ShaderProgram::ShaderSourceType::String
    );
    if (!fisheyeFragment) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Failed to load fisheye fragment shader\n%s\n",
            fisheyeFragmentShader.c_str()
        );
    }

    mShader.setName("FisheyeShader");
    mShader.createAndLinkProgram();
    mShader.bind();

    mShaderLoc.cubemapLoc = mShader.getUniformLocation("cubemap");
    glUniform1i(mShaderLoc.cubemapLoc, 0);

    if (Settings::instance()->useDepthTexture()) {
        mShaderLoc.depthCubemapLoc = mShader.getUniformLocation("depthmap");
        glUniform1i(mShaderLoc.depthCubemapLoc, 1);
    }

    if (Settings::instance()->useNormalTexture()) {
        mShaderLoc.normalCubemapLoc = mShader.getUniformLocation("normalmap");
        glUniform1i(mShaderLoc.normalCubemapLoc, 2);
    }
    
    if (Settings::instance()->usePositionTexture()) {
        mShaderLoc.positionCubemapLoc = mShader.getUniformLocation("positionmap");
        glUniform1i(mShaderLoc.positionCubemapLoc, 3);
    }

    mShaderLoc.halfFovLoc = mShader.getUniformLocation("halfFov");
    glUniform1f(mShaderLoc.halfFovLoc, glm::half_pi<float>());

    if (mOffAxis) {
        mShaderLoc.offsetLoc = mShader.getUniformLocation("offset");
        glUniform3f(mShaderLoc.offsetLoc, mTotalOffset.x, mTotalOffset.y, mTotalOffset.z);
    }

    ShaderProgram::unbind();

    if (Settings::instance()->useDepthTexture()) {
        mDepthCorrectionShader.setName("FisheyeDepthCorrectionShader");
        mDepthCorrectionShader.createAndLinkProgram();
        mDepthCorrectionShader.bind();

        mShaderLoc.swapColorLoc = mDepthCorrectionShader.getUniformLocation("cTex");
        glUniform1i(mShaderLoc.swapColorLoc, 0);
        mShaderLoc.swapDepthLoc = mDepthCorrectionShader.getUniformLocation("dTex");
        glUniform1i(mShaderLoc.swapDepthLoc, 1);
        mShaderLoc.swapNearLoc = mDepthCorrectionShader.getUniformLocation("near");
        mShaderLoc.swapFarLoc = mDepthCorrectionShader.getUniformLocation("far");

        ShaderProgram::unbind();
    }
}

void FisheyeProjection::drawCubeFace(BaseViewport& face) {
    glLineWidth(1.0);
    if (Engine::instance()->getWireframe()) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glDepthFunc(GL_LESS);

    glEnable(GL_SCISSOR_TEST);
    setupViewport(face);

    if (Engine::instance()->mClearBufferFnPtr) {
        Engine::instance()->mClearBufferFnPtr();
    }
    else {
        glm::vec4 color = Engine::instance()->getClearColor();
        glClearColor(color.r, color.g, color.b, color.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    glDisable(GL_SCISSOR_TEST);

    if (Engine::instance()->isOGLPipelineFixed()) {
        glMatrixMode(GL_PROJECTION);
        Projection& p = face.getProjection(Engine::instance()->getCurrentFrustumMode());
        glLoadMatrixf(glm::value_ptr(p.getProjectionMatrix()));
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(glm::value_ptr(
            p.getViewMatrix() * sgct::Engine::instance()->getModelMatrix()
        ));
    }

    Engine::instance()->mDrawFnPtr();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void FisheyeProjection::blitCubeFace(int face) {
    // copy AA-buffer to "regular"/non-AA buffer
    // bind separate read and draw buffers to prepare blit operation
    mCubeMapFbo->bindBlit();
    attachTextures(face);
    mCubeMapFbo->blit();
}

void FisheyeProjection::attachTextures(int face) {
    if (Settings::instance()->useDepthTexture()) {
        mCubeMapFbo->attachDepthTexture(mTextures.depthSwap);
        mCubeMapFbo->attachColorTexture(mTextures.colorSwap);
    }
    else {
        mCubeMapFbo->attachCubeMapTexture(mTextures.cubeMapColor, face);
    }

    if (Settings::instance()->useNormalTexture()) {
        mCubeMapFbo->attachCubeMapTexture(
            mTextures.cubeMapNormals,
            face,
            GL_COLOR_ATTACHMENT1
        );
    }

    if (Settings::instance()->usePositionTexture()) {
        mCubeMapFbo->attachCubeMapTexture(
            mTextures.cubeMapPositions,
            face,
            GL_COLOR_ATTACHMENT2
        );
    }
}

void FisheyeProjection::renderInternal() {
    glEnable(GL_SCISSOR_TEST);
    Engine::instance()->enterCurrentViewport();
    glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    mShader.bind();

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // if for some reson the active texture has been reset
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures.cubeMapColor);

    if (Settings::instance()->useDepthTexture()) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures.cubeMapDepth);
        glUniform1i(mShaderLoc.depthCubemapLoc, 1);
    }

    if (Settings::instance()->useNormalTexture()) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures.cubeMapNormals);
        glUniform1i(mShaderLoc.normalCubemapLoc, 2);
    }

    if (Settings::instance()->usePositionTexture()) {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures.cubeMapPositions);
        glUniform1i(mShaderLoc.positionCubemapLoc, 3);
    }

    glDisable(GL_CULL_FACE);
    const bool alpha = Engine::instance()->getCurrentWindow().getAlpha();
    if (alpha) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else {
        glDisable(GL_BLEND);
    }
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    glUniform1i(mShaderLoc.cubemapLoc, 0);
    glUniform1f(mShaderLoc.halfFovLoc, glm::radians<float>(mFOV / 2.f));

    if (mOffAxis) {
        glUniform3f(mShaderLoc.offsetLoc, mTotalOffset.x, mTotalOffset.y, mTotalOffset.z);
    }

    glBindVertexArray(mVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(GL_FALSE);

    ShaderProgram::unbind();

    glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glDisable(GL_DEPTH_TEST);

    if (alpha) {
        glDisable(GL_BLEND);
    }

    // restore depth func
    glDepthFunc(GL_LESS);
}

void FisheyeProjection::renderInternalFixedPipeline() {
    glEnable(GL_SCISSOR_TEST);
    Engine::mInstance->enterCurrentViewport();
    glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    mShader.bind();

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    glActiveTexture(GL_TEXTURE0);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures.cubeMapColor);

    if (Settings::instance()->useDepthTexture()) {
        glActiveTexture(GL_TEXTURE1);
        glEnable(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures.cubeMapDepth);
        glUniform1i(mShaderLoc.depthCubemapLoc, 1);
    }

    if (Settings::instance()->useNormalTexture()) {
        glActiveTexture(GL_TEXTURE2);
        glEnable(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures.cubeMapNormals);
        glUniform1i(mShaderLoc.normalCubemapLoc, 2);
    }

    if (Settings::instance()->usePositionTexture()) {
        glActiveTexture(GL_TEXTURE3);
        glEnable(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures.cubeMapPositions);
        glUniform1i(mShaderLoc.positionCubemapLoc, 3);
    }

    glDisable(GL_CULL_FACE);
    const bool alpha = Engine::instance()->getCurrentWindow().getAlpha();
    if (alpha) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else {
        glDisable(GL_BLEND);
    }
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    glUniform1i(mShaderLoc.cubemapLoc, 0);
    glUniform1f(mShaderLoc.halfFovLoc, glm::radians<float>(mFOV / 2.f));

    if (mOffAxis) {
        glUniform3f(mShaderLoc.offsetLoc, mTotalOffset.x, mTotalOffset.y, mTotalOffset.z);
    }

    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    //make sure that VBOs are unbound; to not mess up the vertex array
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glClientActiveTexture(GL_TEXTURE0);

    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(0));

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(8));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindBuffer(GL_ARRAY_BUFFER, GL_FALSE);

    ShaderProgram::unbind();

    glPopClientAttrib();
    glPopAttrib();
}

void FisheyeProjection::renderCubemapInternal(size_t* subViewPortIndex) {
    auto internalRender = [this, subViewPortIndex](BaseViewport& vp, int idx) {
        *subViewPortIndex = idx;

        if (!vp.isEnabled()) {
            return;
        }

        mCubeMapFbo->bind();
        if (!mCubeMapFbo->isMultiSampled()) {
            attachTextures(idx);
        }

        Engine::instance()->getCurrentWindow().setCurrentViewport(&vp);
        drawCubeFace(vp);

        // blit MSAA fbo to texture
        if (mCubeMapFbo->isMultiSampled()) {
            blitCubeFace(idx);
        }

        // re-calculate depth values from a cube to spherical model
        if (Settings::instance()->useDepthTexture()) {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
            mCubeMapFbo->bind(false, 1, buffers); // bind no multi-sampled

            mCubeMapFbo->attachCubeMapTexture(mTextures.cubeMapColor, idx);
            mCubeMapFbo->attachCubeMapDepthTexture(mTextures.cubeMapDepth, idx);

            glViewport(0, 0, mCubemapResolution, mCubemapResolution);
            glScissor(0, 0, mCubemapResolution, mCubemapResolution);
            glEnable(GL_SCISSOR_TEST);

            Engine::instance()->mClearBufferFnPtr();

            glDisable(GL_CULL_FACE);
            const bool alpha = Engine::instance()->getCurrentWindow().getAlpha();
            if (alpha) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
            else {
                glDisable(GL_BLEND);
            }

            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_ALWAYS);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mTextures.colorSwap);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, mTextures.depthSwap);

            mDepthCorrectionShader.bind();
            glUniform1i(mShaderLoc.swapColorLoc, 0);
            glUniform1i(mShaderLoc.swapDepthLoc, 1);
            glUniform1f(
                mShaderLoc.swapNearLoc,
                Engine::instance()->mNearClippingPlaneDist
            );
            glUniform1f(mShaderLoc.swapFarLoc, Engine::instance()->mFarClippingPlaneDist);

            Engine::instance()->getCurrentWindow().bindVAO();
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            Engine::instance()->getCurrentWindow().unbindVAO();

            ShaderProgram::unbind();

            glDisable(GL_DEPTH_TEST);

            if (alpha) {
                glDisable(GL_BLEND);
            }

            glDepthFunc(GL_LESS);
            glDisable(GL_SCISSOR_TEST);
        }
    };

    internalRender(mSubViewports.right, 0);
    internalRender(mSubViewports.left, 1);
    internalRender(mSubViewports.bottom, 2);
    internalRender(mSubViewports.top, 3);
    internalRender(mSubViewports.front, 4);
    internalRender(mSubViewports.back, 5);
}

void FisheyeProjection::renderCubemapInternalFixedPipeline(size_t* subViewPortIndex) {
    auto internalRender = [this, subViewPortIndex](BaseViewport& vp, int idx) {
        *subViewPortIndex = idx;

        if (!vp.isEnabled()) {
            return;
        }
        mCubeMapFbo->bind();
        if (!mCubeMapFbo->isMultiSampled()) {
            attachTextures(idx);
        }

        Engine::instance()->getCurrentWindow().setCurrentViewport(&vp);
        drawCubeFace(vp);

        // blit MSAA fbo to texture
        if (mCubeMapFbo->isMultiSampled()) {
            blitCubeFace(idx);
        }

        // re-calculate depth values from a cube to spherical model
        if (Settings::instance()->useDepthTexture()) {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
            mCubeMapFbo->bind(false, 1, buffers); // bind no multi-sampled

            mCubeMapFbo->attachCubeMapTexture(mTextures.cubeMapColor, idx);
            mCubeMapFbo->attachCubeMapDepthTexture(mTextures.cubeMapDepth, idx);

            glViewport(0, 0, mCubemapResolution, mCubemapResolution);
            glScissor(0, 0, mCubemapResolution, mCubemapResolution);

            glPushAttrib(GL_ALL_ATTRIB_BITS);
            glEnable(GL_SCISSOR_TEST);

            Engine::instance()->mClearBufferFnPtr();

            glActiveTexture(GL_TEXTURE0);
            glMatrixMode(GL_TEXTURE);
            glLoadIdentity();

            glMatrixMode(GL_MODELVIEW);

            mDepthCorrectionShader.bind();
            glUniform1i(mShaderLoc.swapColorLoc, 0);
            glUniform1i(mShaderLoc.swapDepthLoc, 1);
            glUniform1f(
                mShaderLoc.swapNearLoc,
                Engine::instance()->mNearClippingPlaneDist
            );
            glUniform1f(mShaderLoc.swapFarLoc, Engine::instance()->mFarClippingPlaneDist);

            glDisable(GL_CULL_FACE);
            bool alpha = Engine::instance()->getCurrentWindow().getAlpha();
            if (alpha) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
            else {
                glDisable(GL_BLEND);
            }

            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_ALWAYS);

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, mTextures.colorSwap);

            glActiveTexture(GL_TEXTURE1);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, mTextures.depthSwap);

            glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
            Engine::instance()->getCurrentWindow().bindVBO();
            glClientActiveTexture(GL_TEXTURE0);

            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(0));

            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(8));
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            Engine::instance()->getCurrentWindow().unbindVBO();
            glPopClientAttrib();

            ShaderProgram::unbind();
            glPopAttrib();
        }
    };

    internalRender(mSubViewports.right, 0);
    internalRender(mSubViewports.left, 1);
    internalRender(mSubViewports.bottom, 2);
    internalRender(mSubViewports.top, 3);
    internalRender(mSubViewports.front, 4);
    internalRender(mSubViewports.back, 5);
}

} // namespace sgct::core
