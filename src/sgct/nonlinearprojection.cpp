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

namespace sgct_core {

int cubeMapResolutionForQuality(const std::string& quality) {
    std::string q = quality;
    q.resize(quality.size());
    std::transform(
        quality.begin(),
        quality.end(),
        q.begin(),
        [](char c) { return static_cast<char>(::tolower(c)); }
    );

    static const std::unordered_map<std::string, int> Map = {
        { "low",     256 },
        { "256",     256 },
        { "medium",  512 },
        { "512",     512 },
        { "high",   1024 },
        { "1k",     1024 },
        { "1024",   1024 },
        { "1.5k",   1536 },
        { "1536",   1536 },
        { "2k",     2048 },
        { "2048",   2048 },
        { "4k",     4096 },
        { "4096",   4096 },
        { "8k",     8192 },
        { "8192",   8192 },
        { "16k",   16384 },
        { "16384", 16384 },
    };

    auto it = Map.find(quality);
    if (it != Map.end()) {
        return it->second;
    }
    else {
        return -1;
    }
}

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

void NonLinearProjection::init(int internalTextureFormat,
                               unsigned int textureFormat, unsigned int textureType,
                               int samples)
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

void NonLinearProjection::updateFrustums(const Frustum::FrustumMode& frustumMode,
                                         float nearClipPlane,
                                         float farClipPlane)
{
    for (int side = 0; side < 6; side++) {
        if (mSubViewports[side].isEnabled()) {
            mSubViewports[side].calculateNonLinearFrustum(
                frustumMode,
                nearClipPlane,
                farClipPlane
            );
        }
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

void NonLinearProjection::setPreferedMonoFrustumMode(Frustum::FrustumMode fm) {
    mPreferedMonoFrustumMode = fm;
}

void NonLinearProjection::setUser(User& user) {
    for (int i = 0; i < 6; ++i) {
        mSubViewports[i].setUser(user);
    }
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
    if (sgct::Engine::instance()->isOpenGLCompatibilityMode()) {
        glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT);
        glEnable(GL_TEXTURE_2D);
    }
    
    generateCubeMap(mTextures.cubeMapColor, mTexInternalFormat, mTexFormat, mTexType);
    if (sgct::Engine::checkForOGLErrors()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "NonLinearProjection: %dx%d color cube map texture (id: %d) generated\n",
            mCubemapResolution, mCubemapResolution, mTextures.cubeMapColor
        );
    }
    else {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "NonLinearProjection: Error occured while generating %dx%d color "
            "cube texture (id: %d)\n",
            mCubemapResolution, mCubemapResolution, mTextures.cubeMapColor
        );
    }
    
    if (sgct::Settings::instance()->useDepthTexture()) {
        generateCubeMap(
            mTextures.cubeMapDepth,
            GL_DEPTH_COMPONENT32,
            GL_DEPTH_COMPONENT,
            GL_FLOAT
        );
        if (sgct::Engine::checkForOGLErrors()) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Debug,
                "NonLinearProjection: %dx%d depth cube map texture (id: %d) generated\n",
                mCubemapResolution, mCubemapResolution, mTextures.cubeMapDepth
            );
        }
        else {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
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
            if (sgct::Engine::checkForOGLErrors()) {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Debug,
                    "NonLinearProjection: %dx%d depth swap map texture (id: %d) generated\n",
                    mCubemapResolution, mCubemapResolution, mTextures.depthSwap
                );
            }
            else {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
                    "NonLinearProjection: Error occured while generating %dx%d depth "
                    "swap texture (id: %d)\n",
                    mCubemapResolution, mCubemapResolution, mTextures.depthSwap
                );
            }

            generateMap(mTextures.colorSwap, mTexInternalFormat, mTexFormat, mTexType);
            if (sgct::Engine::checkForOGLErrors()) {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Debug,
                    "NonLinearProjection: %dx%d color swap map texture (id: %d) generated\n",
                    mCubemapResolution, mCubemapResolution, mTextures.colorSwap
                );
            }
            else {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
                    "NonLinearProjection: Error occured while generating %dx%d color "
                    "swap texture (id: %d)\n",
                    mCubemapResolution, mCubemapResolution, mTextures.colorSwap
                );
            }
        }
    }

    if (sgct::Settings::instance()->useNormalTexture()) {
        generateCubeMap(
            mTextures.cubeMapNormals,
            sgct::Settings::instance()->getBufferFloatPrecisionAsGLint(),
            GL_BGR,
            GL_FLOAT
        );
        if (sgct::Engine::checkForOGLErrors()) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Debug,
                "NonLinearProjection: %dx%d normal cube map texture (id: %d) generated\n",
                mCubemapResolution, mCubemapResolution, mTextures.cubeMapNormals
            );
        }
        else {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "NonLinearProjection: Error occured while generating %dx%d normal "
                "cube texture (id: %d)\n",
                mCubemapResolution, mCubemapResolution, mTextures.cubeMapNormals
            );
        }
    }

    if (sgct::Settings::instance()->usePositionTexture()) {
        generateCubeMap(
            mTextures.cubeMapPositions,
            sgct::Settings::instance()->getBufferFloatPrecisionAsGLint(),
            GL_BGR,
            GL_FLOAT
        );
        if (sgct::Engine::checkForOGLErrors()) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Debug,
                "NonLinearProjection: %dx%d position cube map texture (id: %d) generated\n",
                mCubemapResolution, mCubemapResolution, mTextures.cubeMapPositions
            );
        }
        else {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "NonLinearProjection: Error occured while generating %dx%d position "
                "cube texture (id: %d)\n",
                mCubemapResolution, mCubemapResolution, mTextures.cubeMapPositions
            );
        }
    }

    if (sgct::Engine::instance()->isOpenGLCompatibilityMode()) {
        glPopAttrib();
    }
}

void NonLinearProjection::initFBO() {
    mCubeMapFbo = std::make_unique<sgct_core::OffScreenBuffer>();
    mCubeMapFbo->setInternalColorFormat(mTexInternalFormat);
    mCubeMapFbo->createFBO(mCubemapResolution, mCubemapResolution, mSamples);

    if (mCubeMapFbo->checkForErrors()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "NonLinearProjection: Cube map FBO created\n"
        );
    }
    else {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "NonLinearProjection: Cube map FBO created with errors\n"
        );
    }

    OffScreenBuffer::unBind();
}

void NonLinearProjection::initVBO() {
    mVerts.resize(20, 0.f);
    
    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        glGenVertexArrays(1, &mVAO);
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "NonLinearProjection: Generating VAO: %d\n",
            mVAO
        );
    }

    glGenBuffers(1, &mVBO);
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "NonLinearProjection: Generating VBO: %d\n",
        mVBO
    );
    
    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        glBindVertexArray(mVAO);
    }
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    // 2TF + 3VF = 2*4 + 3*4 = 20
    glBufferData(GL_ARRAY_BUFFER, 20 * sizeof(float), mVerts.data(), GL_STREAM_DRAW);
    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,                  // attribute 0. No particular reason for 0, but must
                                // match the layout in the shader.
            2,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            5 * sizeof(float),    // stride
            reinterpret_cast<void*>(0) // array buffer offset
        );

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1,                  // attribute 1. No particular reason for 1, but
                                // must match the layout in the shader.
            3,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            5 * sizeof(float),    // stride
            reinterpret_cast<void*>(8) // array buffer offset
        );
    }

    //unbind
    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
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
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "NonLinearProjection: Cubemap size set to max size: %d\n",
            MaxCubeMapRes
        );
    }

    //set up texture target
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

    //---------------------
    // Disable mipmaps
    //---------------------
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    if (sgct::Engine::instance()->isOGLPipelineFixed() ||
        sgct::Settings::instance()->getForceGlTexImage2D())
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
    //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "NonLinearProjection: Requested map size is too big (%d > %d)!\n",
            mCubemapResolution, MaxMapRes
        );
    }

    //set up texture target
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    //---------------------
    // Disable mipmaps
    //---------------------
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    if (sgct::Engine::instance()->isOGLPipelineFixed() ||
        sgct::Settings::instance()->getForceGlTexImage2D())
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

    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

} // namespace sgct_core
