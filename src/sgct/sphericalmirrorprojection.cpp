/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/sphericalmirrorprojection.h>

#include <sgct/engine.h>
#include <sgct/messageHandler.h>
#include <sgct/offScreenBuffer.h>
#include <sgct/window.h>
#include <sgct/viewport.h>
#include <sgct/helpers/stringfunctions.h>
#include <sgct/shaders/internalsphericalprojectionshaders.h>
#include <sgct/shaders/internalsphericalprojectionshaders_modern.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//#define DebugCubemap

namespace sgct_core {

void SphericalMirrorProjection::update(glm::vec2) {}

void SphericalMirrorProjection::render() {
    if (sgct::Engine::instance()->isOGLPipelineFixed()) {
        renderInternalFixedPipeline();
    }
    else {
        renderInternal();
    }
}

void SphericalMirrorProjection::renderCubemap(size_t* subViewPortIndex) {
    if (sgct::Engine::instance()->isOGLPipelineFixed()) {
        renderCubemapInternalFixedPipeline(subViewPortIndex);
    }
    else {
        renderCubemapInternal(subViewPortIndex);
    }
}

void SphericalMirrorProjection::setTilt(float angle) {
    mTilt = angle;
}

void SphericalMirrorProjection::setMeshPaths(std::string bottom, std::string left,
                                             std::string right, std::string top)
{
    mMeshPaths.bottom = std::move(bottom);
    mMeshPaths.left = std::move(left);
    mMeshPaths.right = std::move(right);
    mMeshPaths.top = std::move(top);
}

void sgct_core::SphericalMirrorProjection::initTextures() {
    const bool compatProfile = sgct::Engine::instance()->isOpenGLCompatibilityMode();
    if (compatProfile) {
        glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT);
        glEnable(GL_TEXTURE_2D);
    }

    auto generate = [this](const BaseViewport& bv, unsigned int& texture) {
        if (!bv.isEnabled()) {
            return;
        }
        generateMap(texture, mTexInternalFormat, mTexFormat, mTexType);
        if (sgct::Engine::checkForOGLErrors()) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Debug,
                "NonLinearProjection: %dx%d cube face texture (id: %d) generated\n",
                mCubemapResolution, mCubemapResolution, texture
            );
        }
        else {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "NonLinearProjection: Error occured while generating %dx%d cube face "
                "texture (id: %d)\n", mCubemapResolution, mCubemapResolution, texture
            );
        }
    };

    generate(mSubViewports[0], mTextures.cubeFaceRight);
    generate(mSubViewports[1], mTextures.cubeFaceLeft);
    generate(mSubViewports[2], mTextures.cubeFaceBottom);
    generate(mSubViewports[3], mTextures.cubeFaceTop);
    generate(mSubViewports[4], mTextures.cubeFaceFront);
    generate(mSubViewports[5], mTextures.cubeFaceBack);

    if (compatProfile) {
        glPopAttrib();
    }
}

void SphericalMirrorProjection::initVBO() {
    Viewport* vp = dynamic_cast<Viewport*>(
        sgct::Engine::instance()->getCurrentWindow().getCurrentViewport()
    );
    if (vp) {
        mMeshes.bottom.readAndGenerateMesh(mMeshPaths.bottom, *vp);
        mMeshes.left.readAndGenerateMesh(mMeshPaths.left, *vp);
        mMeshes.right.readAndGenerateMesh(mMeshPaths.right, *vp);
        mMeshes.top.readAndGenerateMesh(mMeshPaths.top, *vp);
    }
}

void SphericalMirrorProjection::initViewports() {
    enum class CubeFaces { PosX = 0, NegX, PosY, NegY, PosZ, NegZ };

    // radius is needed to calculate the distance to all view planes
    float radius = mDiameter / 2.f;

    // setup base viewport that will be rotated to create the other cubemap views
    // +Z face
    const glm::vec4 lowerLeftBase(-radius, -radius, radius, 1.f);
    const glm::vec4 upperLeftBase(-radius, radius, radius, 1.f);
    const glm::vec4 upperRightBase(radius, radius, radius, 1.f);

    // tilt
    glm::mat4 tiltMat = glm::rotate(
        glm::mat4(1.f),
        glm::radians(45.f - mTilt),
        glm::vec3(1.f, 0.f, 0.f)
    );

    // add viewports
    for (unsigned int i = 0; i < 6; i++) {
        mSubViewports[i].setName("SphericalMirror " + std::to_string(i));

        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        glm::mat4 rotMat(1.f);

        switch (static_cast<CubeFaces>(i)) {
            case CubeFaces::PosX:
                // +X face, right
                rotMat = glm::rotate(
                    tiltMat,
                    glm::radians(-90.f),
                    glm::vec3(0.f, 1.f, 0.f)
                );
                break;
            case CubeFaces::NegX:
                // -X face, left
                rotMat = glm::rotate(
                    tiltMat,
                    glm::radians(90.f),
                    glm::vec3(0.f, 1.f, 0.f)
                );
                break;
            case CubeFaces::PosY:
                // +Y face, bottom
                mSubViewports[i].setEnabled(false);
                rotMat = glm::rotate(
                    tiltMat,
                    glm::radians(-90.f),
                    glm::vec3(1.f, 0.f, 0.f)
                );
                break;
            case CubeFaces::NegY:
                // -Y face, top
                rotMat = glm::rotate(
                    tiltMat,
                    glm::radians(90.f),
                    glm::vec3(1.f, 0.f, 0.f)
                );
                break;
            case CubeFaces::PosZ:
                // +Z face, front
                rotMat = tiltMat;
                break;
            case CubeFaces::NegZ:
                // -Z face, back
                mSubViewports[i].setEnabled(false);
                rotMat = glm::rotate(
                    tiltMat,
                    glm::radians(180.f),
                    glm::vec3(0.f, 1.f, 0.f)
                );
                break;
        }

        // add viewplane vertices
        mSubViewports[i].getProjectionPlane().setCoordinateLowerLeft(
            glm::vec3(rotMat * lowerLeft)
        );
        mSubViewports[i].getProjectionPlane().setCoordinateUpperLeft(
            glm::vec3(rotMat * upperLeft)
        );
        mSubViewports[i].getProjectionPlane().setCoordinateUpperRight(
            glm::vec3(rotMat * upperRight)
        );
    }
}

void SphericalMirrorProjection::initShaders() {
    if (mStereo || mPreferedMonoFrustumMode != Frustum::MonoEye) {
        // if any frustum mode other than Mono (or stereo)
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Warning,
            "Stereo rendering not supported in spherical projection\n"
        );
    }

    //reload shader program if it exists
    if (mShader.isLinked()) {
        mShader.deleteProgram();
    }

    std::string sphericalMirrorVertexShader =
        sgct::Engine::instance()->isOGLPipelineFixed() ?
        sgct_core::shaders::SphericalProjectionVert :
        sgct_core::shaders_modern::SphericalProjectionVert;

    std::string sphericalMirrorFragmentShader =
        sgct::Engine::instance()->isOGLPipelineFixed() ?
        sgct_core::shaders::SphericalProjectionFrag :
        sgct_core::shaders_modern::SphericalProjectionFrag;

    // replace glsl version
    sgct_helpers::findAndReplace(
        sphericalMirrorVertexShader,
        "**glsl_version**",
        sgct::Engine::instance()->getGLSLVersion()
    );
    sgct_helpers::findAndReplace(
        sphericalMirrorFragmentShader,
        "**glsl_version**",
        sgct::Engine::instance()->getGLSLVersion()
    );

    bool vertShader = mShader.addShaderSrc(
        sphericalMirrorVertexShader,
        GL_VERTEX_SHADER,
        sgct::ShaderProgram::ShaderSourceType::String
    );
    if (!vertShader) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Failed to load spherical mirror vertex shader:\n%s\n",
            sphericalMirrorVertexShader.c_str()
        );
    }
    bool fragShader = mShader.addShaderSrc(
        sphericalMirrorFragmentShader,
        GL_FRAGMENT_SHADER,
        sgct::ShaderProgram::ShaderSourceType::String
    );
    if (!fragShader) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Failed to load spherical mirror fragment shader\n%s\n",
            sphericalMirrorFragmentShader.c_str()
        );
    }
    mShader.setName("SphericalMirrorShader");
    mShader.createAndLinkProgram();
    mShader.bind();

    mTexLoc = mShader.getUniformLocation("Tex");
    glUniform1i(mTexLoc, 0);

    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        mMatrixLoc = mShader.getUniformLocation("MVP");
    }

    sgct::ShaderProgram::unbind();
}

void SphericalMirrorProjection::drawCubeFace(size_t face) {
    glLineWidth(1.0);
    if (sgct::Engine::instance()->getWireframe()) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    //reset depth function (to opengl default)
    glDepthFunc(GL_LESS);

    setupViewport(mSubViewports[face]);

#if defined DebugCubemap
    float color[4];
    switch (face) {
        case 0:
            color[0] = 0.5f;
            color[1] = 0.f;
            color[2] = 0.f;
            color[3] = 1.f;
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
    if (sgct::Engine::mInstance->mClearBufferFnPtr) {
        sgct::Engine::mInstance->mClearBufferFnPtr();
    }
    else {
        glm::vec4 color = sgct::Engine::instance()->getClearColor();
        glClearColor(color.r, color.g, color.b, color.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
#endif

    if (sgct::Engine::instance()->isOGLPipelineFixed()) {
        glMatrixMode(GL_PROJECTION);
        Projection& proj = mSubViewports[face].getProjection(
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

void SphericalMirrorProjection::blitCubeFace(unsigned int texture) {
    // copy AA-buffer to "regular"/non-AA buffer
    // bind separate read and draw buffers to prepare blit operation
    mCubeMapFbo->bindBlit();
    attachTextures(texture);
    mCubeMapFbo->blit();
}

void SphericalMirrorProjection::attachTextures(unsigned int texture) {
    mCubeMapFbo->attachColorTexture(texture);
}

void SphericalMirrorProjection::renderInternal() {
    sgct::Engine::instance()->enterCurrentViewport();

    sgct::Window& winPtr = sgct::Engine::instance()->getCurrentWindow();
    BaseViewport* vpPtr = winPtr.getCurrentViewport();

    float aspect = winPtr.getAspectRatio() * (vpPtr->getSize().x / vpPtr->getSize().y);
    glm::mat4 MVP = glm::ortho(-aspect, aspect, -1.f, 1.f, -1.f, 1.f);

    glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mShader.bind();

    // if for some reson the active texture has been reset
    glActiveTexture(GL_TEXTURE0);
    
    glDisable(GL_CULL_FACE);
    const bool alpha = sgct::Engine::mInstance->getCurrentWindow().getAlpha();
    if (alpha) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else {
        glDisable(GL_BLEND);
    }
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    glUniform1i(mTexLoc, 0);
    glUniformMatrix4fv(mMatrixLoc, 1, GL_FALSE, glm::value_ptr(MVP));

    glBindTexture(GL_TEXTURE_2D, mTextures.cubeFaceFront);
    mMeshes.bottom.renderWarpMesh();

    glBindTexture(GL_TEXTURE_2D, mTextures.cubeFaceLeft);
    mMeshes.left.renderWarpMesh();

    glBindTexture(GL_TEXTURE_2D, mTextures.cubeFaceRight);
    mMeshes.right.renderWarpMesh();

    glBindTexture(GL_TEXTURE_2D, mTextures.cubeFaceTop);
    mMeshes.top.renderWarpMesh();

    sgct::ShaderProgram::unbind();

    glDisable(GL_DEPTH_TEST);

    if (alpha) {
        glDisable(GL_BLEND);
    }

    // restore depth func
    glDepthFunc(GL_LESS);
}

void SphericalMirrorProjection::renderInternalFixedPipeline() {
    sgct::Engine::mInstance->enterCurrentViewport();

    sgct::Window& winPtr = sgct::Engine::instance()->getCurrentWindow();
    BaseViewport* vpPtr = winPtr.getCurrentViewport();
    
    float aspect = winPtr.getAspectRatio() * (vpPtr->getSize().x / vpPtr->getSize().y);
    
    glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mShader.bind();

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    // if for some reson the active texture has been reset
    glActiveTexture(GL_TEXTURE0);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glPushMatrix();
    glOrtho(-aspect, aspect, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW); //restore
    glLoadIdentity();

    glDisable(GL_CULL_FACE);
    const bool alpha = winPtr.getAlpha();
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
    glUniform1i(mTexLoc, 0);

    glBindTexture(GL_TEXTURE_2D, mTextures.cubeFaceFront);
    mMeshes.bottom.renderWarpMesh();

    glBindTexture(GL_TEXTURE_2D, mTextures.cubeFaceLeft);
    mMeshes.left.renderWarpMesh();

    glBindTexture(GL_TEXTURE_2D, mTextures.cubeFaceRight);
    mMeshes.right.renderWarpMesh();

    glBindTexture(GL_TEXTURE_2D, mTextures.cubeFaceTop);
    mMeshes.top.renderWarpMesh();

    sgct::ShaderProgram::unbind();

    glPopClientAttrib();
    glPopAttrib();

    // exit ortho mode
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

void SphericalMirrorProjection::renderCubemapInternal(size_t* subViewPortIndex) {
    auto renderInternal = [this](BaseViewport& bv, unsigned int& texture, int idx) {
        if (!bv.isEnabled()) {
            return;
        }
        mCubeMapFbo->bind();
        if (!mCubeMapFbo->isMultiSampled()) {
            attachTextures(texture);
        }

        sgct::Engine::mInstance->getCurrentWindow().setCurrentViewport(&bv);
        drawCubeFace(idx);

        // blit MSAA fbo to texture
        if (mCubeMapFbo->isMultiSampled()) {
            blitCubeFace(texture);
        }
    };

    renderInternal(mSubViewports[0], mTextures.cubeFaceRight, 0);
    renderInternal(mSubViewports[1], mTextures.cubeFaceLeft, 1);
    renderInternal(mSubViewports[2], mTextures.cubeFaceBottom, 2);
    renderInternal(mSubViewports[3], mTextures.cubeFaceTop, 3);
    renderInternal(mSubViewports[4], mTextures.cubeFaceFront, 4);
    renderInternal(mSubViewports[5], mTextures.cubeFaceBack, 5);
}

void SphericalMirrorProjection::renderCubemapInternalFixedPipeline(size_t* subViewPortIndex)
{
    auto renderInternal = [this, subViewPortIndex](BaseViewport& bv,
                                                   unsigned int& texture, int idx)
    {
        if (!bv.isEnabled()) {
            return;
        }

        *subViewPortIndex = idx;


        mCubeMapFbo->bind();
        if (!mCubeMapFbo->isMultiSampled()) {
            attachTextures(texture);
        }

        sgct::Engine::mInstance->getCurrentWindow().setCurrentViewport(&bv);
        drawCubeFace(idx);

        // blit MSAA fbo to texture
        if (mCubeMapFbo->isMultiSampled()) {
            blitCubeFace(texture);
        }
    };

    renderInternal(mSubViewports[0], mTextures.cubeFaceRight, 0);
    renderInternal(mSubViewports[1], mTextures.cubeFaceLeft, 1);
    renderInternal(mSubViewports[2], mTextures.cubeFaceBottom, 2);
    renderInternal(mSubViewports[3], mTextures.cubeFaceTop, 3);
    renderInternal(mSubViewports[4], mTextures.cubeFaceFront, 4);
    renderInternal(mSubViewports[5], mTextures.cubeFaceBack, 5);
}

} // namespace sgct_core
