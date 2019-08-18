/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/FisheyeProjection.h>

#include <sgct/Engine.h>
#include <sgct/MessageHandler.h>
#include <sgct/SGCTSettings.h>
#include <sgct/SGCTUser.h>
#include <sgct/SGCTWindow.h>
#include <sgct/OffScreenBuffer.h>
#include <sgct/helpers/SGCTStringFunctions.h>
#include <sgct/shaders/SGCTInternalFisheyeShaders.h>
#include <sgct/shaders/SGCTInternalFisheyeShaders_modern.h>
#include <sgct/shaders/SGCTInternalFisheyeShaders_cubic.h>
#include <sgct/shaders/SGCTInternalFisheyeShaders_modern_cubic.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>

//#define DebugCubemap

namespace sgct_core {

/*!
Update projection when aspect ratio changes for the viewport.
*/
void FisheyeProjection::update(float width, float height) {
    updateGeometry(width, height);
}

/*!
Render the non linear projection to currently bounded FBO
*/
void FisheyeProjection::render() {
    if (sgct::Engine::instance()->isOGLPipelineFixed()) {
        renderInternalFixedPipeline();
        //mInternalRenderCubemapFn = &FisheyeProjection::renderCubemapInternalFixedPipeline;
    }
    else {
        renderInternal();
        //mInternalRenderCubemapFn = &FisheyeProjection::renderCubemapInternal;
    }
}

/*!
Render the enabled faces of the cubemap
*/
void FisheyeProjection::renderCubemap(std::size_t* subViewPortIndex) {
    if (sgct::Engine::instance()->getCurrentFrustumMode() == Frustum::StereoLeftEye) {
        setOffset(
            -sgct::Engine::instance()->getDefaultUserPtr()->getEyeSeparation() / mDiameter,
            0.f
        );
    }
    else if (sgct::Engine::instance()->getCurrentFrustumMode() == Frustum::StereoRightEye) {
        setOffset(
            sgct::Engine::instance()->getDefaultUserPtr()->getEyeSeparation() / mDiameter,
            0.f
        );
    }

    if (sgct::Engine::instance()->isOGLPipelineFixed()) {
        renderCubemapInternalFixedPipeline(subViewPortIndex);
    }
    else {
        renderCubemapInternal(subViewPortIndex);
    }
}

/*!
Set the dome diameter used in the fisheye renderer (used for the viewplane distance calculations)

@param diameter diameter of the dome diameter in meters
*/
void FisheyeProjection::setDomeDiameter(float diameter) {
    mDiameter = diameter;
    //generateCubeMapViewports();
}

/*!
Set the fisheye/dome tilt angle used in the fisheye renderer.
The tilt angle is from the horizontal.

@param angle the tilt angle in degrees
*/
void FisheyeProjection::setTilt(float angle) {
    mTilt = angle;
}

/*!
Set the fisheye/dome field-of-view angle used in the fisheye renderer.

@param angle the FOV angle in degrees
*/
void FisheyeProjection::setFOV(float angle) {
    mFOV = angle;
}

/*!
Set the method used for rendering the fisheye projection.

@param method the selected method
*/
void FisheyeProjection::setRenderingMethod(FisheyeMethod method) {
    mMethod = method;
}

/*!
Set the fisheye crop values. Theese values are used when rendering content for a single projector dome.
The elumenati geodome has usually a 4:3 SXGA+ (1400x1050) projector and the fisheye is cropped 25% (350 pixels) at the top.
*/
void FisheyeProjection::setCropFactors(float left, float right, float bottom, float top) {
    mCropFactors[CropLeft] = (left < 1.f && left > 0.f) ? left : 0.f;
    mCropFactors[CropRight] = (right < 1.f && right > 0.f) ? right : 0.f;
    mCropFactors[CropBottom] = (bottom < 1.f && bottom > 0.f) ? bottom : 0.f;
    mCropFactors[CropTop] = (top < 1.f && top > 0.f) ? top : 0.f;
}

/*!
Set fisheye offset to render offaxis. Length of vector must be smaller then 1.
Base of fisheye is the XY-plane. This function is normally used in fisheye stereo rendering.
*/
void FisheyeProjection::setOffset(glm::vec3 offset) {
    mOffset = std::move(offset);
    mTotalOffset = mBaseOffset + mOffset;

    mOffAxis = glm::length(mTotalOffset) > 0.f;
}

/*!
Set fisheye offset to render offaxis. Length of vector must be smaller then 1.
Base of fisheye is the XY-plane. This function is normally used in fisheye stereo rendering.
*/
void FisheyeProjection::setOffset(float x, float y, float z) {
    mOffset.x = x;
    mOffset.y = y;
    mOffset.z = z;

    mTotalOffset = mBaseOffset + mOffset;
    mOffAxis = glm::length(mTotalOffset) > 0.f;
}

/*!
Set fisheye base offset to render offaxis. Length of vector must be smaller then 1.
Base of fisheye is the XY-plane. The base offset will be added to the offset specified by setFisheyeOffset.
These values are set from the XML config.
*/
void FisheyeProjection::setBaseOffset(glm::vec3 offset) {
    mBaseOffset = std::move(offset);
    mTotalOffset = mBaseOffset + mOffset;

    mOffAxis = glm::length(mTotalOffset) > 0.f;
}

/*!
Ignore the framebuffer aspect ratio to allow non-circular fisheye. This is usefull for spherical mirror projections.
*/
void FisheyeProjection::setIgnoreAspectRatio(bool state) {
    mIgnoreAspectRatio = state;
}

/*!
Get the lens offset for off-axis projection.

@returns offset
*/
glm::vec3 FisheyeProjection::getOffset() const {
    return mTotalOffset;
}

void FisheyeProjection::initViewports() {
    enum cubeFaces { Pos_X = 0, Neg_X, Pos_Y, Neg_Y, Pos_Z, Neg_Z };

    //radius is needed to calculate the distance to all view planes
    float radius = mDiameter / 2.f;

    //setup base viewport that will be rotated to create the other cubemap views
    //+Z face
    const glm::vec4 lowerLeftBase(-radius, -radius, radius, 1.f);
    const glm::vec4 upperLeftBase(-radius, radius, radius, 1.f);
    const glm::vec4 upperRightBase(radius, radius, radius, 1.f);

    //250.5287794 degree FOV covers exactly five sides of a cube, larger FOV needs six sides
    const float fiveFaceLimit = 2.f * glm::degrees(acosf(-1.f / sqrtf(3.f)));
    //109.4712206 degree FOV is needed to cover the entire top face
    const float topFaceLimit = 2.f * glm::degrees(acosf(1.f / sqrtf(3.f)));

    float cropLevel = 0.5f; //how much of the side faces that are used
    float projectionOffset = 0.f; //the projection offset
    float normalizedProjectionOffset = 0.f;

    //four faces doesn't cover more than 180 degrees
    if (mFOV > 180.f && mFOV <= fiveFaceLimit) {
        mMethod = FisheyeMethod::FiveFaceCube;
    }

    if (mMethod == FisheyeMethod::FiveFaceCube &&
        mFOV >= topFaceLimit && mFOV <= fiveFaceLimit)
    {
        //helper variable
        float cosAngle = cosf(glm::radians(mFOV / 2.f));
        if (mFOV < 180.f) {
            normalizedProjectionOffset = 1.f - mFOV / 180.f; //[-1, 0]
        }
        else {
            normalizedProjectionOffset = sqrtf((2.f * cosAngle * cosAngle) /
                                               (1.f - cosAngle * cosAngle)); //[0, 1]
        }

        projectionOffset = normalizedProjectionOffset * radius;
        cropLevel = (1.f - normalizedProjectionOffset) / 2.f;

        //cropLevel = 0.0f;
        //projectionOffset = radius;
    }
    else if (mFOV > fiveFaceLimit) {
        mMethod = FisheyeMethod::SixFaceCube;
        cropLevel = 0.f;
        projectionOffset = radius;
    }

    if (mMethod == FisheyeMethod::FiveFaceCube || mMethod == FisheyeMethod::SixFaceCube) {
        //tilt
        glm::mat4 tiltMat = glm::rotate(
            glm::mat4(1.f),
            glm::radians(90.f - mTilt),
            glm::vec3(1.f, 0.f, 0.f)
        );
        //glm::mat4 tiltMat(1.0f);

        //roll 45 deg
        glm::mat4 rollRot = glm::rotate(
            tiltMat,
            glm::radians(45.f),
            glm::vec3(0.f, 0.f, 1.f)
        );
        //glm::mat4 rollRot(1.0f);
        //glm::mat4 rollRot = tiltMat;

        //add viewports
        for (unsigned int i = 0; i < 6; i++) {
            mSubViewports[i].setName("Fisheye " + std::to_string(i));

            glm::vec4 lowerLeft = lowerLeftBase;
            glm::vec4 upperLeft = upperLeftBase;
            glm::vec4 upperRight = upperRightBase;

            glm::mat4 rotMat(1.f);

            /*
            Rotate and clamp the halv height viewports
            */
            switch (i) {
                case Pos_X: //+X face
                    rotMat = glm::rotate(
                        rollRot,
                        glm::radians(-90.f),
                        glm::vec3(0.f, 1.f, 0.f)
                    );
                    upperRight.x = projectionOffset;
                    mSubViewports[i].setSize(1.f - cropLevel, 1.f);
                    break;
                case Neg_X: //-X face
                    rotMat = glm::rotate(
                        rollRot,
                        glm::radians(90.f),
                        glm::vec3(0.f, 1.f, 0.f)
                    );
                    lowerLeft.x = -projectionOffset;
                    upperLeft.x = -projectionOffset;
                    mSubViewports[i].setPos(cropLevel, 0.f);
                    mSubViewports[i].setSize(1.f - cropLevel, 1.f);
                    break;
                case Pos_Y: //+Y face
                    rotMat = glm::rotate(
                        rollRot,
                        glm::radians(-90.f),
                        glm::vec3(1.f, 0.f, 0.f)
                    );
                    lowerLeft.y = -projectionOffset;
                    mSubViewports[i].setPos(0.f, cropLevel);
                    mSubViewports[i].setSize(1.f, 1.f - cropLevel);
                    break;
                case Neg_Y: //-Y face
                    rotMat = glm::rotate(
                        rollRot,
                        glm::radians(90.f),
                        glm::vec3(1.f, 0.f, 0.f)
                    );
                    upperLeft.y = projectionOffset;
                    upperRight.y = projectionOffset;
                    mSubViewports[i].setSize(1.f, 1.f - cropLevel);
                    break;
                case Pos_Z: //+Z face
                    rotMat = rollRot;
                    break;
                case Neg_Z: //-Z face
                    if (mMethod == FisheyeMethod::FiveFaceCube) {
                        mSubViewports[i].setEnabled(false);
                    }
                    rotMat = glm::rotate(
                        rollRot,
                        glm::radians(180.f),
                        glm::vec3(0.f, 1.f, 0.f)
                    );
                    break;
            }

            mSubViewports[i].getProjectionPlane().setCoordinate(
                SGCTProjectionPlane::LowerLeft,
                glm::vec3(rotMat * lowerLeft)
            );
            mSubViewports[i].getProjectionPlane().setCoordinate(
                SGCTProjectionPlane::UpperLeft,
                glm::vec3(rotMat * upperLeft)
            );
            mSubViewports[i].getProjectionPlane().setCoordinate(
                SGCTProjectionPlane::UpperRight,
                glm::vec3(rotMat * upperRight)
            );
        }
    }
    else {
        //tilt
        glm::mat4 tiltMat = glm::rotate(
            glm::mat4(1.f),
            glm::radians(90.f - mTilt),
            glm::vec3(1.f, 0.f, 0.f)
        );
        //glm::mat4 tiltMat(1.0f);

        //pan 45 deg
        glm::mat4 panRot = glm::rotate(
            tiltMat,
            glm::radians(45.f),
            glm::vec3(0.f, 1.f, 0.f)
        );
        //glm::mat4 panRot(1.0f);
        //glm::mat4 panRot = tiltMat;

        //add viewports
        for (unsigned int i = 0; i < 6; i++) {
            mSubViewports[i].setName("Fisheye " + std::to_string(i));

            glm::vec4 lowerLeft = lowerLeftBase;
            glm::vec4 upperLeft = upperLeftBase;
            glm::vec4 upperRight = upperRightBase;

            glm::mat4 rotMat(1.f);

            switch (i) {
                case Pos_X: //+X face
                    rotMat = glm::rotate(
                        panRot,
                        glm::radians(-90.f),
                        glm::vec3(0.f, 1.f, 0.f)
                    );
                    break;
                case Neg_X: //-X face
                    mSubViewports[i].setEnabled(false);
                    rotMat = glm::rotate(
                        panRot,
                        glm::radians(90.f),
                        glm::vec3(0.f, 1.f, 0.f)
                    );
                    break;
                case Pos_Y: //+Y face
                    rotMat = glm::rotate(
                        panRot,
                        glm::radians(-90.f),
                        glm::vec3(1.f, 0.f, 0.f)
                    );
                    break;
                case Neg_Y: //-Y face
                    rotMat = glm::rotate(
                        panRot,
                        glm::radians(90.f),
                        glm::vec3(1.f, 0.f, 0.f)
                    );
                    break;
                case Pos_Z: //+Z face
                    rotMat = panRot;
                    break;
                case Neg_Z: //-Z face
                    mSubViewports[i].setEnabled(false);
                    rotMat = glm::rotate(
                        panRot,
                        glm::radians(180.f),
                        glm::vec3(0.f, 1.f, 0.f)
                    );
                    break;
            } //end switch

            //add viewplane vertices
            mSubViewports[i].getProjectionPlane().setCoordinate(
                SGCTProjectionPlane::LowerLeft,
                glm::vec3(rotMat * lowerLeft)
            );
            mSubViewports[i].getProjectionPlane().setCoordinate(
                SGCTProjectionPlane::UpperLeft,
                glm::vec3(rotMat * upperLeft)
            );
            mSubViewports[i].getProjectionPlane().setCoordinate(
                SGCTProjectionPlane::UpperRight,
                glm::vec3(rotMat * upperRight)
            );
        } //end for
    } //end if
}

void FisheyeProjection::initShaders() {
    if (mStereo || mPreferedMonoFrustumMode != Frustum::MonoEye) {
        // if any frustum mode other than Mono (or stereo)
        mOffAxis = true;
    }

    // reload shader program if it exists
    if (mShader.isLinked()) {
        mShader.deleteProgram();
    }

    std::string fisheyeFragmentShader;
    std::string fisheyeVertexShader;

    bool isCubic = (mInterpolationMode == InterpolationMode::Cubic);

    if (sgct::Engine::instance()->isOGLPipelineFixed()) {
        fisheyeVertexShader = isCubic ?
            shaders_fisheye_cubic::FisheyeVert : 
            shaders_fisheye::FisheyeVert;

        if (mOffAxis) {
            if (sgct::SGCTSettings::instance()->useDepthTexture()) {
                switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType()) {
                    case sgct::SGCTSettings::DrawBufferType::Diffuse:
                    default:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragOffAxisDepth :
                            shaders_fisheye::FisheyeFragOffAxisDepth;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffuseNormal:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragOffAxisDepthNormal :
                            shaders_fisheye::FisheyeFragOffAxisDepthNormal;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffusePosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragOffAxisDepthPosition :
                            shaders_fisheye::FisheyeFragOffAxisDepthPosition;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffuseNormalPosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragOffAxisDepthNormalPosition :
                            shaders_fisheye::FisheyeFragOffAxisDepthNormalPosition;
                        break;
                }
            }
            else  {
                // no depth
                switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType()) {
                    case sgct::SGCTSettings::DrawBufferType::Diffuse:
                    default:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragOffAxis :
                            shaders_fisheye::FisheyeFragOffAxis;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffuseNormal:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragOffAxisNormal :
                            shaders_fisheye::FisheyeFragOffAxisNormal;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffusePosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragOffAxisPosition :
                            shaders_fisheye::FisheyeFragOffAxisPosition;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffuseNormalPosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragOffAxisNormalPosition :
                            shaders_fisheye::FisheyeFragOffAxisNormalPosition;
                        break;
                }
            }
        }
        else {
            // not off-axis
            if (sgct::SGCTSettings::instance()->useDepthTexture()) {
                switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType()) {
                    case sgct::SGCTSettings::DrawBufferType::Diffuse:
                    default:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragDepth :
                            shaders_fisheye::FisheyeFragDepth;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffuseNormal:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragDepthNormal :
                            shaders_fisheye::FisheyeFragDepthNormal;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffusePosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragDepthPosition :
                            shaders_fisheye::FisheyeFragDepthPosition;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffuseNormalPosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragDepthNormalPosition :
                            shaders_fisheye::FisheyeFragDepthNormalPosition;
                        break;
                }
            }
            else {
                // no depth
                switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType()) {
                    case sgct::SGCTSettings::DrawBufferType::Diffuse:
                    default:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFrag :
                            shaders_fisheye::FisheyeFrag;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffuseNormal:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragNormal :
                            shaders_fisheye::FisheyeFragNormal;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffusePosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragPosition :
                            shaders_fisheye::FisheyeFragPosition;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffuseNormalPosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_fisheye_cubic::FisheyeFragNormalPosition :
                            shaders_fisheye::FisheyeFragNormalPosition;
                        break;
                }
            }
        }

        //depth correction shader only
        if (sgct::SGCTSettings::instance()->useDepthTexture()) {
            std::string depthCorrFragSshader = shaders_fisheye::BaseVert;
            std::string depthCorrVertSshader =
                shaders_fisheye::FisheyeDepthCorrectionFrag;

            //replace glsl version
            sgct_helpers::findAndReplace(
                depthCorrFragSshader,
                "**glsl_version**",
                sgct::Engine::instance()->getGLSLVersion()
            );
            sgct_helpers::findAndReplace(
                depthCorrVertSshader,
                "**glsl_version**",
                sgct::Engine::instance()->getGLSLVersion()
            );

            bool depthCorrFrag = mDepthCorrectionShader.addShaderSrc(
                depthCorrFragSshader,
                GL_VERTEX_SHADER,
                sgct::ShaderProgram::ShaderSourceType::String
            );
            if (!depthCorrFrag) {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
                    "Failed to load fisheye depth correction vertex shader\n"
                );
            }
            bool depthCorrVert = mDepthCorrectionShader.addShaderSrc(
                depthCorrVertSshader,
                GL_FRAGMENT_SHADER,
                sgct::ShaderProgram::ShaderSourceType::String
            );
            if (!depthCorrVert) {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
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
            if (sgct::SGCTSettings::instance()->useDepthTexture()) {
                switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType()) {
                    case sgct::SGCTSettings::DrawBufferType::Diffuse:
                    default:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragOffAxisDepth :
                            shaders_modern_fisheye::FisheyeFragOffAxisDepth;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffuseNormal:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragOffAxisDepthNormal :
                            shaders_modern_fisheye::FisheyeFragOffAxisDepthNormal;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffusePosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragOffAxisDepthPosition :
                            shaders_modern_fisheye::FisheyeFragOffAxisDepthPosition;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffuseNormalPosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragOffAxisDepthNormalPosition :
                            shaders_modern_fisheye::FisheyeFragOffAxisDepthNormalPosition;
                        break;
                }
            }
            else {
                // no depth
                switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType()) {
                    case sgct::SGCTSettings::DrawBufferType::Diffuse:
                    default:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragOffAxis :
                            shaders_modern_fisheye::FisheyeFragOffAxis;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffuseNormal:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragOffAxisNormal :
                            shaders_modern_fisheye::FisheyeFragOffAxisNormal;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffusePosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragOffAxisPosition :
                            shaders_modern_fisheye::FisheyeFragOffAxisPosition;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffuseNormalPosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragOffAxisNormalPosition :
                            shaders_modern_fisheye::FisheyeFragOffAxisNormalPosition;
                        break;
                }
            }
        }
        else {
            // not off axis
            if (sgct::SGCTSettings::instance()->useDepthTexture()) {
                switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType()) {
                    case sgct::SGCTSettings::DrawBufferType::Diffuse:
                    default:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragDepth :
                            shaders_modern_fisheye::FisheyeFragDepth;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffuseNormal:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragDepthNormal :
                            shaders_modern_fisheye::FisheyeFragDepthNormal;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffusePosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragDepthPosition :
                            shaders_modern_fisheye::FisheyeFragDepthPosition;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffuseNormalPosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragDepthNormalPosition :
                            shaders_modern_fisheye::FisheyeFragDepthNormalPosition;
                        break;
                }
            }
            else {
                // no depth
                switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType()) {
                    case sgct::SGCTSettings::DrawBufferType::Diffuse:
                    default:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFrag :
                            shaders_modern_fisheye::FisheyeFrag;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffuseNormal:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragNormal :
                            shaders_modern_fisheye::FisheyeFragNormal;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffusePosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragPosition :
                            shaders_modern_fisheye::FisheyeFragPosition;
                        break;

                    case sgct::SGCTSettings::DrawBufferType::DiffuseNormalPosition:
                        fisheyeFragmentShader = isCubic ?
                            shaders_modern_fisheye_cubic::FisheyeFragNormalPosition :
                            shaders_modern_fisheye::FisheyeFragNormalPosition;
                        break;
                }
            }
        }

        // depth correction shader only
        if (sgct::SGCTSettings::instance()->useDepthTexture()) {
            std::string depthCorrFragShader = shaders_modern_fisheye::BaseVert;
            std::string depthCorrVertShader = shaders_modern_fisheye::FisheyeDepthCorrectionFrag;

            //replace glsl version
            sgct_helpers::findAndReplace(
                depthCorrFragShader,
                "**glsl_version**",
                sgct::Engine::instance()->getGLSLVersion()
            );
            sgct_helpers::findAndReplace(
                depthCorrVertShader,
                "**glsl_version**",
                sgct::Engine::instance()->getGLSLVersion()
            );

            bool depthCorrFrag = mDepthCorrectionShader.addShaderSrc(
                depthCorrFragShader,
                GL_VERTEX_SHADER,
                sgct::ShaderProgram::ShaderSourceType::String
            );
            if (!depthCorrFrag) {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
                    "Failed to load fisheye depth correction vertex shader\n"
                );
            }
            bool depthCorrVert = mDepthCorrectionShader.addShaderSrc(
                depthCorrVertShader,
                GL_FRAGMENT_SHADER,
                sgct::ShaderProgram::ShaderSourceType::String
            );
            if (!depthCorrVert) {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
                    "Failed to load fisheye depth correction fragment shader\n"
                );
            }
        }
    }

    // add functions to shader
    if (mOffAxis) {
        sgct_helpers::findAndReplace(
            fisheyeFragmentShader,
            "**sample_fun**",
            sgct::Engine::instance()->isOGLPipelineFixed() ?
                shaders_fisheye::SampleOffsetFun :
                shaders_modern_fisheye::SampleOffsetFun
        );
    }
    else {
        sgct_helpers::findAndReplace(
            fisheyeFragmentShader,
            "**sample_fun**",
            sgct::Engine::instance()->isOGLPipelineFixed() ?
                shaders_fisheye::SampleFun :
                shaders_modern_fisheye::SampleFun
        );
    }

    if (isCubic) {
        // add functions to shader
        sgct_helpers::findAndReplace(
            fisheyeFragmentShader, "**cubic_fun**",
            sgct::Engine::instance()->isOGLPipelineFixed() ?
                shaders_fisheye_cubic::catmullRomFun :
                shaders_modern_fisheye_cubic::catmullRomFun
        );
        sgct_helpers::findAndReplace(
            fisheyeFragmentShader, "**interpolatef**",
            sgct::Engine::instance()->isOGLPipelineFixed() ?
                shaders_fisheye_cubic::interpolate4_f :
                shaders_modern_fisheye_cubic::interpolate4_f
        );
        sgct_helpers::findAndReplace(
            fisheyeFragmentShader, "**interpolate3f**",
            shaders_modern_fisheye_cubic::interpolate4_3f
        );
        sgct_helpers::findAndReplace(
            fisheyeFragmentShader, "**interpolate4f**",
            sgct::Engine::instance()->isOGLPipelineFixed() ?
                shaders_fisheye_cubic::interpolate4_4f :
                shaders_modern_fisheye_cubic::interpolate4_4f
        );

        // set size
        sgct_helpers::findAndReplace(
            fisheyeFragmentShader,
            "**size**",
            std::to_string(mCubemapResolution) + ".0"
        );

        // set step
        sgct_helpers::findAndReplace(fisheyeFragmentShader, "**step**", "1.0");
    }

    //replace add correct transform in the fragment shader
    if (mMethod == FisheyeMethod::FourFaceCube) {
        sgct_helpers::findAndReplace(
            fisheyeFragmentShader,
            "**rotVec**",
            "vec3 rotVec = vec3(angle45Factor*x + angle45Factor*z, y, "
            "-angle45Factor*x + angle45Factor*z)"
        );
    }
    else {
        // five or six face
        sgct_helpers::findAndReplace(
            fisheyeFragmentShader,
            "**rotVec**",
            "vec3 rotVec = vec3(angle45Factor*x - angle45Factor*y, "
            "angle45Factor*x + angle45Factor*y, z)"
        );
    }

    // replace glsl version
    sgct_helpers::findAndReplace(
        fisheyeVertexShader,
        "**glsl_version**",
        sgct::Engine::instance()->getGLSLVersion()
    );
    sgct_helpers::findAndReplace(
        fisheyeFragmentShader,
        "**glsl_version**",
        sgct::Engine::instance()->getGLSLVersion()
    );

    //replace color
    std::stringstream ssColor;
    ssColor.precision(2);
    ssColor << std::fixed << "vec4(" << mClearColor.r << ", " << mClearColor.g
            << ", " << mClearColor.b << ", " << mClearColor.a << ")";
    sgct_helpers::findAndReplace(fisheyeFragmentShader, "**bgColor**", ssColor.str());

    bool fisheyeVertex = mShader.addShaderSrc(
        fisheyeVertexShader,
        GL_VERTEX_SHADER,
        sgct::ShaderProgram::ShaderSourceType::String
    );
    if (!fisheyeVertex) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Failed to load fisheye vertex shader:\n%s\n",
            fisheyeVertexShader.c_str()
        );
    }
    bool fisheyeFragment = mShader.addShaderSrc(
        fisheyeFragmentShader,
        GL_FRAGMENT_SHADER,
        sgct::ShaderProgram::ShaderSourceType::String
    );
    if (!fisheyeFragment) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Failed to load fisheye fragment shader\n%s\n",
            fisheyeFragmentShader.c_str()
        );
    }

    mShader.setName("FisheyeShader");
    mShader.createAndLinkProgram();
    mShader.bind();

    mShaderLoc.cubemapLoc = mShader.getUniformLocation("cubemap");
    glUniform1i(mShaderLoc.cubemapLoc, 0);

    if (sgct::SGCTSettings::instance()->useDepthTexture()) {
        mShaderLoc.depthCubemapLoc = mShader.getUniformLocation("depthmap");
        glUniform1i(mShaderLoc.depthCubemapLoc, 1);
    }

    if (sgct::SGCTSettings::instance()->useNormalTexture()) {
        mShaderLoc.normalCubemapLoc = mShader.getUniformLocation("normalmap");
        glUniform1i(mShaderLoc.normalCubemapLoc, 2);
    }
    
    if (sgct::SGCTSettings::instance()->usePositionTexture()) {
        mShaderLoc.positionCubemapLoc = mShader.getUniformLocation("positionmap");
        glUniform1i(mShaderLoc.positionCubemapLoc, 3);
    }

    mShaderLoc.halfFovLoc = mShader.getUniformLocation("halfFov");
    glUniform1f(mShaderLoc.halfFovLoc, glm::half_pi<float>());

    if (mOffAxis) {
        mShaderLoc.offsetLoc = mShader.getUniformLocation("offset");
        glUniform3f(mShaderLoc.offsetLoc, mTotalOffset.x, mTotalOffset.y, mTotalOffset.z);
    }

    sgct::ShaderProgram::unbind();

    if (sgct::SGCTSettings::instance()->useDepthTexture()) {
        mDepthCorrectionShader.setName("FisheyeDepthCorrectionShader");
        mDepthCorrectionShader.createAndLinkProgram();
        mDepthCorrectionShader.bind();

        mShaderLoc.swapColorLoc = mDepthCorrectionShader.getUniformLocation("cTex");
        glUniform1i(mShaderLoc.swapColorLoc, 0);
        mShaderLoc.swapDepthLoc = mDepthCorrectionShader.getUniformLocation("dTex");
        glUniform1i(mShaderLoc.swapDepthLoc, 1);
        mShaderLoc.swapNearLoc = mDepthCorrectionShader.getUniformLocation("near");
        mShaderLoc.swapFarLoc = mDepthCorrectionShader.getUniformLocation("far");

        sgct::ShaderProgram::unbind();
    }
}

void FisheyeProjection::updateGeometry(float width, float height) {
    //create proxy geometry
    float leftcrop = mCropFactors[CropLeft];
    float rightcrop = mCropFactors[CropRight];
    float bottomcrop = mCropFactors[CropBottom];
    float topcrop = mCropFactors[CropTop];

    float cropAspect = ((1.f - 2.f * bottomcrop) + (1.f - 2.f * topcrop)) /
                       ((1.f - 2.f * leftcrop) + (1.f - 2.f * rightcrop));

    float x = 1.f;
    float y = 1.f;

    float frameBufferAspect = mIgnoreAspectRatio ? 1.f : width / height;

    if (sgct::SGCTSettings::instance()->getTryMaintainAspectRatio()) {
        float aspect = frameBufferAspect * cropAspect;
        if (aspect >= 1.0f) {
            x = 1.0f / aspect;
        }
        else {
            y = aspect;
        }
    }

    mVerts[0] = leftcrop;
    mVerts[1] = bottomcrop;
    mVerts[2] = -x;
    mVerts[3] = -y;
    mVerts[4] = -1.f;

    mVerts[5] = leftcrop;
    mVerts[6] = 1.f - topcrop;
    mVerts[7] = -x;
    mVerts[8] = y;
    mVerts[9] = -1.f;

    mVerts[10] = 1.f - rightcrop;
    mVerts[11] = bottomcrop;
    mVerts[12] = x;
    mVerts[13] = -y;
    mVerts[14] = -1.f;

    mVerts[15] = 1.f - rightcrop;
    mVerts[16] = 1.f - topcrop;
    mVerts[17] = x;
    mVerts[18] = y;
    mVerts[19] = -1.f;

    //update VBO
    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        glBindVertexArray(mVAO);
    }
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);

    GLvoid* PositionBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    memcpy(PositionBuffer, mVerts, 20 * sizeof(float));
    glUnmapBuffer(GL_ARRAY_BUFFER);

    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        glBindVertexArray(0);
    }
    else {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void FisheyeProjection::drawCubeFace(size_t face) {
    glLineWidth(1.0);
    if (sgct::Engine::instance()->getWireframe()) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    //reset depth function (to opengl default)
    glDepthFunc(GL_LESS);

    //run scissor test to prevent clearing of entire buffer
    glEnable(GL_SCISSOR_TEST);
    setupViewport(face);

#ifdef DebugCubemap
    float color[4];
    switch (face) {
        case 0:
            color[0] = 0.5f;
            color[1] = 0.f;
            color[2] = 0.f;
            color[3] = 1.f
            break;
        case 1:
            color[0] = 0.5f;
            color[1] = 0.5f;
            color[2] = 0.f;
            color[3] = 1.f;
            break;
        case 2:
            color[0] = 0.f;
            color[1] = 0.5f;
            color[2] = 0.f;
            color[3] = 1.f;
            break;
        case 3:
            color[0] = 0.f;
            color[1] = 0.5f;
            color[2] = 0.5f;
            color[3] = 1.f;
            break;
        case 4:
            color[0] = 0.f;
            color[1] = 0.f;
            color[2] = 0.5f;
            color[3] = 1.f;
            break;
        case 5:
            color[0] = 0.5f;
            color[1] = 0.f;
            color[2] = 0.5f;
            color[3] = 1.f;
            break;
    }
    glClearColor(color[0], color[1], color[2], color[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
    if (sgct::Engine::mInstance->mClearBufferFnPtr != nullptr) {
        sgct::Engine::mInstance->mClearBufferFnPtr();
    }
    else {
        glm::vec4 color = sgct::Engine::instance()->getClearColor();
        glClearColor(color.r, color.g, color.b, color.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
#endif

    glDisable(GL_SCISSOR_TEST);

    if (sgct::Engine::instance()->isOGLPipelineFixed()) {
        glMatrixMode(GL_PROJECTION);
        SGCTProjection& proj = mSubViewports[face].getProjection(
            sgct::Engine::instance()->getCurrentFrustumMode()
        );
        glLoadMatrixf(glm::value_ptr(proj.getProjectionMatrix()));
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(glm::value_ptr(
            proj.getViewMatrix() * sgct::Engine::instance()->getModelMatrix()
        ));
    }

    // render
    sgct::Engine::mInstance->mDrawFnPtr();

    // restore polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void FisheyeProjection::blitCubeFace(int face) {
    //copy AA-buffer to "regular"/non-AA buffer
    mCubeMapFBO_Ptr->bindBlit(); //bind separate read and draw buffers to prepare blit operation
    attachTextures(face);
    mCubeMapFBO_Ptr->blit();
}

void FisheyeProjection::attachTextures(int face) {
    if (sgct::SGCTSettings::instance()->useDepthTexture()) {
        mCubeMapFBO_Ptr->attachDepthTexture(mTextures[DepthSwap]);
        mCubeMapFBO_Ptr->attachColorTexture(mTextures[ColorSwap]);
    }
    else {
        mCubeMapFBO_Ptr->attachCubeMapTexture(mTextures[CubeMapColor], face);
    }

    if (sgct::SGCTSettings::instance()->useNormalTexture()) {
        mCubeMapFBO_Ptr->attachCubeMapTexture(
            mTextures[CubeMapNormals],
            face,
            GL_COLOR_ATTACHMENT1
        );
    }

    if (sgct::SGCTSettings::instance()->usePositionTexture()) {
        mCubeMapFBO_Ptr->attachCubeMapTexture(
            mTextures[CubeMapPositions],
            face,
            GL_COLOR_ATTACHMENT2
        );
    }
}

void FisheyeProjection::renderInternal() {
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

    if (sgct::SGCTSettings::instance()->useDepthTexture()) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures[CubeMapDepth]);
        glUniform1i(mShaderLoc.depthCubemapLoc, 1);
    }

    if (sgct::SGCTSettings::instance()->useNormalTexture()) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures[CubeMapNormals]);
        glUniform1i(mShaderLoc.normalCubemapLoc, 2);
    }

    if (sgct::SGCTSettings::instance()->usePositionTexture()) {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures[CubeMapPositions]);
        glUniform1i(mShaderLoc.positionCubemapLoc, 3);
    }

    glDisable(GL_CULL_FACE);
    bool alpha = sgct::Engine::mInstance->getCurrentWindowPtr().getAlpha();
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

    sgct::ShaderProgram::unbind();

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

    if (sgct::SGCTSettings::instance()->useDepthTexture()) {
        glActiveTexture(GL_TEXTURE1);
        glEnable(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures[CubeMapDepth]);
        glUniform1i(mShaderLoc.depthCubemapLoc, 1);
    }

    if (sgct::SGCTSettings::instance()->useNormalTexture()) {
        glActiveTexture(GL_TEXTURE2);
        glEnable(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures[CubeMapNormals]);
        glUniform1i(mShaderLoc.normalCubemapLoc, 2);
    }

    if (sgct::SGCTSettings::instance()->usePositionTexture()) {
        glActiveTexture(GL_TEXTURE3);
        glEnable(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures[CubeMapPositions]);
        glUniform1i(mShaderLoc.positionCubemapLoc, 3);
    }

    glDisable(GL_CULL_FACE);
    bool alpha = sgct::Engine::mInstance->getCurrentWindowPtr().getAlpha();
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

void FisheyeProjection::renderCubemapInternal(std::size_t* subViewPortIndex) {
    BaseViewport* vp;
    unsigned int faceIndex;
    for (size_t i = 0; i < 6; i++) {
        vp = &mSubViewports[i];
        *subViewPortIndex = i;
        faceIndex = static_cast<unsigned int>(i);

        if (vp->isEnabled()) {
            //bind & attach buffer
            //osg seems to unbind FBO when rendering with osg FBO cameras
            mCubeMapFBO_Ptr->bind();
            if (!mCubeMapFBO_Ptr->isMultiSampled()) {
                attachTextures(faceIndex);
            }

            sgct::Engine::mInstance->getCurrentWindowPtr().setCurrentViewport(vp);
            drawCubeFace(i);

            // blit MSAA fbo to texture
            if (mCubeMapFBO_Ptr->isMultiSampled()) {
                blitCubeFace(faceIndex);
            }

            //re-calculate depth values from a cube to spherical model
            if (sgct::SGCTSettings::instance()->useDepthTexture()) {
                GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
                mCubeMapFBO_Ptr->bind(false, 1, buffers); //bind no multi-sampled

                mCubeMapFBO_Ptr->attachCubeMapTexture(
                    mTextures[CubeMapColor],
                    faceIndex
                );
                mCubeMapFBO_Ptr->attachCubeMapDepthTexture(
                    mTextures[CubeMapDepth],
                    faceIndex
                );

                glViewport(0, 0, mCubemapResolution, mCubemapResolution);
                glScissor(0, 0, mCubemapResolution, mCubemapResolution);
                glEnable(GL_SCISSOR_TEST);

                sgct::Engine::mInstance->mClearBufferFnPtr();

                glDisable(GL_CULL_FACE);
                bool alpha = sgct::Engine::mInstance->getCurrentWindowPtr().getAlpha();
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
                glBindTexture(GL_TEXTURE_2D, mTextures[ColorSwap]);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, mTextures[DepthSwap]);

                //bind shader
                bindDepthCorrectionShaderProgram();
                glUniform1i(mShaderLoc.swapColorLoc, 0);
                glUniform1i(mShaderLoc.swapDepthLoc, 1);
                glUniform1f(
                    mShaderLoc.swapNearLoc,
                    sgct::Engine::mInstance->mNearClippingPlaneDist
                );
                glUniform1f(
                    mShaderLoc.swapFarLoc,
                    sgct::Engine::mInstance->mFarClippingPlaneDist
                );

                sgct::Engine::mInstance->getCurrentWindowPtr().bindVAO();
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                sgct::Engine::mInstance->getCurrentWindowPtr().unbindVAO();

                //unbind shader
                sgct::ShaderProgram::unbind();

                glDisable(GL_DEPTH_TEST);

                if (alpha) {
                    glDisable(GL_BLEND);
                }

                // restore depth func
                glDepthFunc(GL_LESS);
                glDisable(GL_SCISSOR_TEST);
            }//end if depthmap
        }//end if viewport is enabled
    }//end for
}

void FisheyeProjection::renderCubemapInternalFixedPipeline(size_t* subViewPortIndex) {
    BaseViewport* vp;
    unsigned int faceIndex;
    for (size_t i = 0; i < 6; i++) {
        vp = &mSubViewports[i];
        *subViewPortIndex = i;
        faceIndex = static_cast<unsigned int>(i);

        if (vp->isEnabled()) {
            //bind & attach buffer
            //osg seems to unbind FBO when rendering with osg FBO cameras
            mCubeMapFBO_Ptr->bind();
            if (!mCubeMapFBO_Ptr->isMultiSampled()) {
                attachTextures(faceIndex);
            }

            sgct::Engine::instance()->getCurrentWindowPtr().setCurrentViewport(vp);
            drawCubeFace(i);

            //blit MSAA fbo to texture
            if (mCubeMapFBO_Ptr->isMultiSampled()) {
                blitCubeFace(faceIndex);
            }

            //re-calculate depth values from a cube to spherical model
            if (sgct::SGCTSettings::instance()->useDepthTexture()) {
                GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
                mCubeMapFBO_Ptr->bind(false, 1, buffers); //bind no multi-sampled

                mCubeMapFBO_Ptr->attachCubeMapTexture(mTextures[CubeMapColor], faceIndex);
                mCubeMapFBO_Ptr->attachCubeMapDepthTexture(mTextures[CubeMapDepth], faceIndex);

                glViewport(0, 0, mCubemapResolution, mCubemapResolution);
                glScissor(0, 0, mCubemapResolution, mCubemapResolution);

                glPushAttrib(GL_ALL_ATTRIB_BITS);
                glEnable(GL_SCISSOR_TEST);
                
                sgct::Engine::instance()->mClearBufferFnPtr();

                glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
                glMatrixMode(GL_TEXTURE);
                glLoadIdentity();

                glMatrixMode(GL_MODELVIEW); //restore

                //bind shader
                bindDepthCorrectionShaderProgram();
                glUniform1i(mShaderLoc.swapColorLoc, 0);
                glUniform1i(mShaderLoc.swapDepthLoc, 1);
                glUniform1f(mShaderLoc.swapNearLoc, sgct::Engine::mInstance->mNearClippingPlaneDist);
                glUniform1f(mShaderLoc.swapFarLoc, sgct::Engine::mInstance->mFarClippingPlaneDist);

                glDisable(GL_CULL_FACE);
                bool alpha = sgct::Engine::mInstance->getCurrentWindowPtr().getAlpha();
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
                glBindTexture(GL_TEXTURE_2D, mTextures[ColorSwap]);

                glActiveTexture(GL_TEXTURE1);
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, mTextures[DepthSwap]);

                glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
                sgct::Engine::mInstance->getCurrentWindowPtr().bindVBO();
                glClientActiveTexture(GL_TEXTURE0);

                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                glTexCoordPointer(
                    2,
                    GL_FLOAT,
                    5 * sizeof(float),
                    reinterpret_cast<void*>(0)
                );

                glEnableClientState(GL_VERTEX_ARRAY);
                glVertexPointer(
                    3,
                    GL_FLOAT,
                    5 * sizeof(float),
                    reinterpret_cast<void*>(8)
                );
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                sgct::Engine::mInstance->getCurrentWindowPtr().unbindVBO();
                glPopClientAttrib();

                //unbind shader
                sgct::ShaderProgram::unbind();
                glPopAttrib();
            }//end if depthmap
        }//end if viewport is enabled
    }//end for
}

} // namespace sgct_core
