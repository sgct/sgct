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
#include <sgct/shaders/internalsphericalprojectionshaders_modern.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//#define DebugCubemap

namespace sgct::core {

void SphericalMirrorProjection::update(glm::vec2) {}

void SphericalMirrorProjection::render() {
    renderInternal();
}

void SphericalMirrorProjection::renderCubemap(size_t* subViewPortIndex) {
    renderCubemapInternal(subViewPortIndex);
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

void core::SphericalMirrorProjection::initTextures() {
    auto generate = [this](const BaseViewport& bv, unsigned int& texture) {
        if (!bv.isEnabled()) {
            return;
        }
        generateMap(texture, mTexInternalFormat, mTexFormat, mTexType);
        if (Engine::checkForOGLErrors()) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Debug,
                "NonLinearProjection: %dx%d cube face texture (id: %d) generated\n",
                mCubemapResolution, mCubemapResolution, texture
            );
        }
        else {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "NonLinearProjection: Error occured while generating %dx%d cube face "
                "texture (id: %d)\n", mCubemapResolution, mCubemapResolution, texture
            );
        }
    };

    generate(mSubViewports.right, mTextures.cubeFaceRight);
    generate(mSubViewports.left, mTextures.cubeFaceLeft);
    generate(mSubViewports.bottom, mTextures.cubeFaceBottom);
    generate(mSubViewports.top, mTextures.cubeFaceTop);
    generate(mSubViewports.front, mTextures.cubeFaceFront);
    generate(mSubViewports.back, mTextures.cubeFaceBack);
}

void SphericalMirrorProjection::initVBO() {
    Viewport* vp = dynamic_cast<Viewport*>(
        Engine::instance()->getCurrentWindow().getCurrentViewport()
    );
    if (vp) {
        mMeshes.bottom.readAndGenerateMesh(mMeshPaths.bottom, *vp);
        mMeshes.left.readAndGenerateMesh(mMeshPaths.left, *vp);
        mMeshes.right.readAndGenerateMesh(mMeshPaths.right, *vp);
        mMeshes.top.readAndGenerateMesh(mMeshPaths.top, *vp);
    }
}

void SphericalMirrorProjection::initViewports() {
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

    // Right
    {
        mSubViewports.right.setName("Spherical Mirror 0");
        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        glm::mat4 rotMat = glm::rotate(
            tiltMat,
            glm::radians(-90.f),
            glm::vec3(0.f, 1.f, 0.f)
        );

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

    // left
    {
        mSubViewports.left.setName("SphericalMirror 1");

        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        glm::mat4 rotMat = glm::rotate(
            tiltMat,
            glm::radians(90.f),
            glm::vec3(0.f, 1.f, 0.f)
        );

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

    // bottom
    {
        mSubViewports.bottom.setName("SphericalMirror 2");

        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        mSubViewports.bottom.setEnabled(false);
        glm::mat4 rotMat = glm::rotate(
            tiltMat,
            glm::radians(-90.f),
            glm::vec3(1.f, 0.f, 0.f)
        );

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

    // top
    {
        mSubViewports.top.setName("SphericalMirror 3");

        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        glm::mat4 rotMat = glm::rotate(
            tiltMat,
            glm::radians(90.f),
            glm::vec3(1.f, 0.f, 0.f)
        );

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

    // front
    {
        mSubViewports.front.setName("SphericalMirror 4");

        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        mSubViewports.front.getProjectionPlane().setCoordinateLowerLeft(
            glm::vec3(tiltMat * lowerLeft)
        );
        mSubViewports.front.getProjectionPlane().setCoordinateUpperLeft(
            glm::vec3(tiltMat * upperLeft)
        );
        mSubViewports.front.getProjectionPlane().setCoordinateUpperRight(
            glm::vec3(tiltMat * upperRight)
        );
    }

    // back
    {
        mSubViewports.back.setName("SphericalMirror 5");

        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;
        
        mSubViewports.back.setEnabled(false);

        glm::mat4 rotMat = glm::rotate(
            tiltMat,
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

void SphericalMirrorProjection::initShaders() {
    if (mStereo || mPreferedMonoFrustumMode != Frustum::Mode::MonoEye) {
        // if any frustum mode other than Mono (or stereo)
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Stereo rendering not supported in spherical projection\n"
        );
    }

    //reload shader program if it exists
    if (mShader.isLinked()) {
        mShader.deleteProgram();
    }

    std::string sphericalMirrorVertexShader =
        core::shaders_modern::SphericalProjectionVert;

    std::string sphericalMirrorFragmentShader =
        core::shaders_modern::SphericalProjectionFrag;

    // replace glsl version
    helpers::findAndReplace(
        sphericalMirrorVertexShader,
        "**glsl_version**",
        Engine::instance()->getGLSLVersion()
    );
    helpers::findAndReplace(
        sphericalMirrorFragmentShader,
        "**glsl_version**",
        Engine::instance()->getGLSLVersion()
    );

    bool vertShader = mShader.addShaderSrc(
        sphericalMirrorVertexShader,
        GL_VERTEX_SHADER,
        ShaderProgram::ShaderSourceType::String
    );
    if (!vertShader) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Failed to load spherical mirror vertex shader:\n%s\n",
            sphericalMirrorVertexShader.c_str()
        );
    }
    bool fragShader = mShader.addShaderSrc(
        sphericalMirrorFragmentShader,
        GL_FRAGMENT_SHADER,
        ShaderProgram::ShaderSourceType::String
    );
    if (!fragShader) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Failed to load spherical mirror fragment shader\n%s\n",
            sphericalMirrorFragmentShader.c_str()
        );
    }
    mShader.setName("SphericalMirrorShader");
    mShader.createAndLinkProgram();
    mShader.bind();

    mTexLoc = mShader.getUniformLocation("Tex");
    glUniform1i(mTexLoc, 0);

    mMatrixLoc = mShader.getUniformLocation("MVP");

    ShaderProgram::unbind();
}

void SphericalMirrorProjection::drawCubeFace(size_t face) {
    BaseViewport& vp = [this](size_t face) -> BaseViewport& {
        switch (face) {
            default:
            case 0: return mSubViewports.right;
            case 1: return mSubViewports.left;
            case 2: return mSubViewports.bottom;
            case 3: return mSubViewports.top;
            case 4: return mSubViewports.front;
            case 5: return mSubViewports.back;
        }
    }(face);

    glLineWidth(1.0);
    if (Engine::instance()->getWireframe()) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    //reset depth function (to opengl default)
    glDepthFunc(GL_LESS);

    setupViewport(vp);

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
    if (Engine::instance()->mClearBufferFnPtr) {
        Engine::instance()->mClearBufferFnPtr();
    }
    else {
        glm::vec4 color = Engine::instance()->getClearColor();
        glClearColor(color.r, color.g, color.b, color.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
#endif

    Engine::instance()->mDrawFnPtr();

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
    Engine::instance()->enterCurrentViewport();

    Window& winPtr = Engine::instance()->getCurrentWindow();
    BaseViewport* vpPtr = winPtr.getCurrentViewport();

    float aspect = winPtr.getAspectRatio() * (vpPtr->getSize().x / vpPtr->getSize().y);
    glm::mat4 MVP = glm::ortho(-aspect, aspect, -1.f, 1.f, -1.f, 1.f);

    glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mShader.bind();

    // if for some reson the active texture has been reset
    glActiveTexture(GL_TEXTURE0);
    
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

    ShaderProgram::unbind();

    glDisable(GL_DEPTH_TEST);

    if (alpha) {
        glDisable(GL_BLEND);
    }

    // restore depth func
    glDepthFunc(GL_LESS);
}

void SphericalMirrorProjection::renderInternalFixedPipeline() {
    Engine::instance()->enterCurrentViewport();

    Window& winPtr = Engine::instance()->getCurrentWindow();
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

    ShaderProgram::unbind();

    glPopClientAttrib();
    glPopAttrib();

    // exit ortho mode
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

void SphericalMirrorProjection::renderCubemapInternal(size_t* subViewPortIndex) {
    auto renderInternal =
        [this, subViewPortIndex](BaseViewport& bv, unsigned int& texture, int idx)
    {
        *subViewPortIndex = static_cast<size_t>(idx);
        if (!bv.isEnabled()) {
            return;
        }
        mCubeMapFbo->bind();
        if (!mCubeMapFbo->isMultiSampled()) {
            attachTextures(texture);
        }

        Engine::instance()->getCurrentWindow().setCurrentViewport(&bv);
        drawCubeFace(idx);

        // blit MSAA fbo to texture
        if (mCubeMapFbo->isMultiSampled()) {
            blitCubeFace(texture);
        }
    };

    renderInternal(mSubViewports.right, mTextures.cubeFaceRight, 0);
    renderInternal(mSubViewports.left, mTextures.cubeFaceLeft, 1);
    renderInternal(mSubViewports.bottom, mTextures.cubeFaceBottom, 2);
    renderInternal(mSubViewports.top, mTextures.cubeFaceTop, 3);
    renderInternal(mSubViewports.front, mTextures.cubeFaceFront, 4);
    renderInternal(mSubViewports.back, mTextures.cubeFaceBack, 5);
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

        Engine::instance()->getCurrentWindow().setCurrentViewport(&bv);
        drawCubeFace(idx);

        // blit MSAA fbo to texture
        if (mCubeMapFbo->isMultiSampled()) {
            blitCubeFace(texture);
        }
    };

    renderInternal(mSubViewports.right, mTextures.cubeFaceRight, 0);
    renderInternal(mSubViewports.left, mTextures.cubeFaceLeft, 1);
    renderInternal(mSubViewports.bottom, mTextures.cubeFaceBottom, 2);
    renderInternal(mSubViewports.top, mTextures.cubeFaceTop, 3);
    renderInternal(mSubViewports.front, mTextures.cubeFaceFront, 4);
    renderInternal(mSubViewports.back, mTextures.cubeFaceBack, 5);
}

} // namespace sgct::core
