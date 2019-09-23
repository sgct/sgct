/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/nonlinearprojection.h>

#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/settings.h>
#include <algorithm>
#include <unordered_map>

namespace sgct::core {

NonLinearProjection::~NonLinearProjection() {
    glDeleteTextures(1, &mTextures.cubeMapColor);
    glDeleteTextures(1, &mTextures.cubeMapDepth);
    glDeleteTextures(1, &mTextures.cubeMapNormals);
    glDeleteTextures(1, &mTextures.cubeMapPositions);
    glDeleteTextures(1, &mTextures.colorSwap);
    glDeleteTextures(1, &mTextures.depthSwap);
    glDeleteTextures(1, &mTextures.cubeFaceRight);
    glDeleteTextures(1, &mTextures.cubeFaceLeft);
    glDeleteTextures(1, &mTextures.cubeFaceBottom);
    glDeleteTextures(1, &mTextures.cubeFaceTop);
    glDeleteTextures(1, &mTextures.cubeFaceFront);
    glDeleteTextures(1, &mTextures.cubeFaceBack);

    if (mCubeMapFbo) {
        mCubeMapFbo->destroy();
    }

    glDeleteBuffers(1, &mVBO);
    mVBO = 0;

    glDeleteVertexArrays(1, &mVAO);
    mVAO = 0;

    mShader.deleteProgram();
    mDepthCorrectionShader.deleteProgram();
}

void NonLinearProjection::init(int internalTextureFormat, unsigned int textureFormat,
                               unsigned int textureType, int samples)
{
    mTexInternalFormat = internalTextureFormat;
    mTexFormat = textureFormat;
    mTexType = textureType;
    mSamples = samples;

    initViewports();
    initTextures();
    initFBO();
    initVBO();
    initShaders();
}

void NonLinearProjection::updateFrustums(const Frustum::Mode& mode, float nearClip,
                                         float farClip)
{
    if (mSubViewports.right.isEnabled()) {
        mSubViewports.right.calculateNonLinearFrustum(mode, nearClip, farClip);
    }
    if (mSubViewports.left.isEnabled()) {
        mSubViewports.left.calculateNonLinearFrustum(mode, nearClip, farClip);
    }
    if (mSubViewports.bottom.isEnabled()) {
        mSubViewports.bottom.calculateNonLinearFrustum(mode, nearClip, farClip);
    }
    if (mSubViewports.top.isEnabled()) {
        mSubViewports.top.calculateNonLinearFrustum(mode, nearClip, farClip);
    }
    if (mSubViewports.front.isEnabled()) {
        mSubViewports.front.calculateNonLinearFrustum(mode, nearClip, farClip);
    }
    if (mSubViewports.back.isEnabled()) {
        mSubViewports.back.calculateNonLinearFrustum(mode, nearClip, farClip);
    }
}

void NonLinearProjection::setCubemapResolution(int resolution) {
    mCubemapResolution = resolution;
}

void NonLinearProjection::setInterpolationMode(InterpolationMode im) {
    mInterpolationMode = im;
}

void NonLinearProjection::setUseDepthTransformation(bool state) {
    mUseDepthTransformation = state;
}

void NonLinearProjection::setStereo(bool state) {
    mStereo = state;
}

void NonLinearProjection::setClearColor(glm::vec4 color) {
    mClearColor = std::move(color);
}

void NonLinearProjection::setAlpha(float alpha) {
    mClearColor.a = alpha;
}

void NonLinearProjection::setPreferedMonoFrustumMode(Frustum::Mode fm) {
    mPreferedMonoFrustumMode = fm;
}

void NonLinearProjection::setUser(User& user) {
    mSubViewports.right.setUser(user);
    mSubViewports.left.setUser(user);
    mSubViewports.bottom.setUser(user);
    mSubViewports.top.setUser(user);
    mSubViewports.front.setUser(user);
    mSubViewports.back.setUser(user);
}

int NonLinearProjection::getCubemapResolution() const {
    return mCubemapResolution;
}

NonLinearProjection::InterpolationMode NonLinearProjection::getInterpolationMode() const {
    return mInterpolationMode;
}

OffScreenBuffer* NonLinearProjection::getOffScreenBuffer() {
    return mCubeMapFbo.get();
}

glm::ivec4 NonLinearProjection::getViewportCoords() {
    return mVpCoords;
}

void NonLinearProjection::initTextures() {    
    if (Engine::instance()->isOpenGLCompatibilityMode()) {
        glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT);
        glEnable(GL_TEXTURE_2D);
    }
    
    generateCubeMap(mTextures.cubeMapColor, mTexInternalFormat, mTexFormat, mTexType);
    if (Engine::checkForOGLErrors()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "NonLinearProjection: %dx%d color cube map texture (id: %d) generated\n",
            mCubemapResolution, mCubemapResolution, mTextures.cubeMapColor
        );
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "NonLinearProjection: Error occured while generating %dx%d color "
            "cube texture (id: %d)\n",
            mCubemapResolution, mCubemapResolution, mTextures.cubeMapColor
        );
    }
    
    if (Settings::instance()->useDepthTexture()) {
        generateCubeMap(
            mTextures.cubeMapDepth,
            GL_DEPTH_COMPONENT32,
            GL_DEPTH_COMPONENT,
            GL_FLOAT
        );
        if (Engine::checkForOGLErrors()) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Debug,
                "NonLinearProjection: %dx%d depth cube map texture (id: %d) generated\n",
                mCubemapResolution, mCubemapResolution, mTextures.cubeMapDepth
            );
        }
        else {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "NonLinearProjection: Error occured while generating %dx%d depth "
                "cube texture (id: %d)\n",
                mCubemapResolution, mCubemapResolution, mTextures.cubeMapDepth
            );
        }

        if (mUseDepthTransformation) {
            // generate swap textures
            generateMap(
                mTextures.depthSwap,
                GL_DEPTH_COMPONENT32,
                GL_DEPTH_COMPONENT,
                GL_FLOAT
            );
            if (Engine::checkForOGLErrors()) {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Debug,
                    "NonLinearProjection: %dx%d depth swap map texture (id: %d) generated\n",
                    mCubemapResolution, mCubemapResolution, mTextures.depthSwap
                );
            }
            else {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Error,
                    "NonLinearProjection: Error occured while generating %dx%d depth "
                    "swap texture (id: %d)\n",
                    mCubemapResolution, mCubemapResolution, mTextures.depthSwap
                );
            }

            generateMap(mTextures.colorSwap, mTexInternalFormat, mTexFormat, mTexType);
            if (Engine::checkForOGLErrors()) {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Debug,
                    "NonLinearProjection: %dx%d color swap map texture (id: %d) generated\n",
                    mCubemapResolution, mCubemapResolution, mTextures.colorSwap
                );
            }
            else {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Error,
                    "NonLinearProjection: Error occured while generating %dx%d color "
                    "swap texture (id: %d)\n",
                    mCubemapResolution, mCubemapResolution, mTextures.colorSwap
                );
            }
        }
    }

    if (Settings::instance()->useNormalTexture()) {
        generateCubeMap(
            mTextures.cubeMapNormals,
            Settings::instance()->getBufferFloatPrecisionAsGLint(),
            GL_BGR,
            GL_FLOAT
        );
        if (Engine::checkForOGLErrors()) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Debug,
                "NonLinearProjection: %dx%d normal cube map texture (id: %d) generated\n",
                mCubemapResolution, mCubemapResolution, mTextures.cubeMapNormals
            );
        }
        else {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "NonLinearProjection: Error occured while generating %dx%d normal "
                "cube texture (id: %d)\n",
                mCubemapResolution, mCubemapResolution, mTextures.cubeMapNormals
            );
        }
    }

    if (Settings::instance()->usePositionTexture()) {
        generateCubeMap(
            mTextures.cubeMapPositions,
            Settings::instance()->getBufferFloatPrecisionAsGLint(),
            GL_BGR,
            GL_FLOAT
        );
        if (Engine::checkForOGLErrors()) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Debug,
                "NonLinearProjection: %dx%d position cube map texture (%d) generated\n",
                mCubemapResolution, mCubemapResolution, mTextures.cubeMapPositions
            );
        }
        else {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "NonLinearProjection: Error occured while generating %dx%d position "
                "cube texture (id: %d)\n",
                mCubemapResolution, mCubemapResolution, mTextures.cubeMapPositions
            );
        }
    }

    if (Engine::instance()->isOpenGLCompatibilityMode()) {
        glPopAttrib();
    }
}

void NonLinearProjection::initFBO() {
    mCubeMapFbo = std::make_unique<core::OffScreenBuffer>();
    mCubeMapFbo->setInternalColorFormat(mTexInternalFormat);
    mCubeMapFbo->createFBO(mCubemapResolution, mCubemapResolution, mSamples);

    if (mCubeMapFbo->checkForErrors()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "NonLinearProjection: Cube map FBO created\n"
        );
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "NonLinearProjection: Cube map FBO created with errors\n"
        );
    }

    OffScreenBuffer::unBind();
}

void NonLinearProjection::initVBO() {
    mVerts.resize(20);
    std::fill(mVerts.begin(), mVerts.end(), 0.f);
    
    if (!Engine::instance()->isOGLPipelineFixed()) {
        glGenVertexArrays(1, &mVAO);
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "NonLinearProjection: Generating VAO: %d\n", mVAO
        );
    }

    glGenBuffers(1, &mVBO);
    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "NonLinearProjection: Generating VBO: %d\n", mVBO
    );
    
    if (!Engine::instance()->isOGLPipelineFixed()) {
        glBindVertexArray(mVAO);
    }
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    // 2TF + 3VF = 2*4 + 3*4 = 20
    glBufferData(GL_ARRAY_BUFFER, 20 * sizeof(float), mVerts.data(), GL_STREAM_DRAW);
    if (!Engine::instance()->isOGLPipelineFixed()) {
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,
            2,
            GL_FLOAT,
            GL_FALSE,
            5 * sizeof(float),
            reinterpret_cast<void*>(0)
        );

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            5 * sizeof(float),
            reinterpret_cast<void*>(8)
        );
    }

    // unbind
    if (!Engine::instance()->isOGLPipelineFixed()) {
        glBindVertexArray(0);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void NonLinearProjection::generateCubeMap(unsigned int& texture, int internalFormat,
                                          unsigned int format, unsigned int type)
{
    glDeleteTextures(1, &texture);
    texture = 0;

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    GLint MaxCubeMapRes;
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &MaxCubeMapRes);
    if (mCubemapResolution > MaxCubeMapRes) {
        mCubemapResolution = MaxCubeMapRes;
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "NonLinearProjection: Cubemap size set to max size: %d\n", MaxCubeMapRes
        );
    }

    // set up texture target
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

    // Disable mipmaps
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    if (Engine::instance()->isOGLPipelineFixed() ||
        Settings::instance()->getForceGlTexImage2D())
    {
        for (int side = 0; side < 6; ++side) {
            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + side,
                0,
                internalFormat,
                mCubemapResolution,
                mCubemapResolution,
                0,
                format,
                type,
                nullptr
            );
        }
    }
    else {
        glTexStorage2D(
            GL_TEXTURE_CUBE_MAP,
            1,
            internalFormat,
            mCubemapResolution,
            mCubemapResolution
        );
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void NonLinearProjection::setupViewport(BaseViewport& vp) {
    float cmRes = static_cast<float>(mCubemapResolution);

    mVpCoords = glm::ivec4(
        static_cast<int>(floor(vp.getPosition().x * cmRes + 0.5f)),
        static_cast<int>(floor(vp.getPosition().y * cmRes + 0.5f)),
        static_cast<int>(floor(vp.getSize().x * cmRes + 0.5f)),
        static_cast<int>(floor(vp.getSize().y * cmRes + 0.5f))
    );

    glViewport(mVpCoords.x, mVpCoords.y, mVpCoords.z, mVpCoords.w);
    glScissor(mVpCoords.x, mVpCoords.y, mVpCoords.z, mVpCoords.w);
}

void NonLinearProjection::generateMap(unsigned int& texture, int internalFormat,
                                      unsigned int format, unsigned int type)
{
    glDeleteTextures(1, &texture);
    texture = 0;

    GLint MaxMapRes;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxMapRes);
    if (mCubemapResolution > MaxMapRes) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "NonLinearProjection: Requested map size is too big (%d > %d)!\n",
            mCubemapResolution, MaxMapRes
        );
    }

    // set up texture target
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Disable mipmaps
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    if (Engine::instance()->isOGLPipelineFixed() ||
        Settings::instance()->getForceGlTexImage2D())
    {
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            internalFormat,
            mCubemapResolution,
            mCubemapResolution,
            0,
            format,
            type,
            nullptr
        );
    }
    else {
        glTexStorage2D(
            GL_TEXTURE_2D,
            1,
            internalFormat,
            mCubemapResolution,
            mCubemapResolution
        );
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

} // namespace sgct::core
