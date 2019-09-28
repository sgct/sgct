/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/PostFX.h>

#include <sgct/ClusterManager.h>
#include <sgct/Engine.h>
#include <sgct/MessageHandler.h>
#include <sgct/OffScreenBuffer.h>

namespace sgct {

bool PostFX::init(std::string name, const std::string& vertShaderSrc,
                  const std::string& fragShaderSrc,
                  ShaderProgram::ShaderSourceType srcType)
{
    mName = std::move(name);
    mShaderProgram.setName(mName);

    if (!mShaderProgram.addShaderSrc(vertShaderSrc, GL_VERTEX_SHADER, srcType)) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "PostFX: Pass '%s' failed to load or set vertex shader\n", mName.c_str()
        );
        return false;
    }

    if (!mShaderProgram.addShaderSrc(fragShaderSrc, GL_FRAGMENT_SHADER, srcType)) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "PostFX: Pass '%s' failed to load or set fragment shader\n", mName.c_str()
        );
        return false;
    }

    if (!mShaderProgram.createAndLinkProgram()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "PostFX: Pass '%s' failed to link shader\n", mName.c_str()
        );
        return false;
    }

    mRenderFn = &PostFX::internalRender;

    return true;
}

void PostFX::destroy() {
    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "PostFX: Pass '%s' destroying shader and texture...\n",
        mName.c_str()
    );

    mRenderFn = nullptr;
    mUpdateFn = nullptr;

    if (!mDeleted) {
        mShaderProgram.deleteProgram();
        mDeleted = true;
    }
}

void PostFX::render() {
    if (mRenderFn) {
        (this->*mRenderFn)();
    }
}

void PostFX::setUpdateUniformsFunction(void(*fnPtr)()) {
    mUpdateFn = fnPtr;
}

void PostFX::setInputTexture(unsigned int inputTex) {
    mInputTexture = inputTex;
}

void PostFX::setOutputTexture(unsigned int outputTex) {
    mOutputTexture = outputTex;
}

unsigned int PostFX::getOutputTexture() const {
    return mOutputTexture;
}

unsigned int PostFX::getInputTexture() const {
    return mInputTexture;
}

ShaderProgram& PostFX::getShaderProgram() {
    return mShaderProgram;
}

const ShaderProgram& PostFX::getShaderProgram() const {
    return mShaderProgram;
}

const std::string& PostFX::getName() const {
    return mName;
}

void PostFX::internalRender() {
    Window& win = core::ClusterManager::instance()->getThisNode()->getCurrentWindow();

    // bind target FBO
    win.getFBO()->attachColorTexture(mOutputTexture);

    mSize = win.getFramebufferResolution();

    glViewport(0, 0, mSize.x, mSize.y);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mInputTexture);

    mShaderProgram.bind();

    if (mUpdateFn) {
        mUpdateFn();
    }

    win.bindVAO();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    win.unbindVAO();

    ShaderProgram::unbind();
}

void PostFX::internalRenderFixedPipeline() {
    Window& win = core::ClusterManager::instance()->getThisNode()->getCurrentWindow();

    // bind target FBO
    win.getFBO()->attachColorTexture(mOutputTexture);
    
    mSize = win.getFramebufferResolution();

    // if for some reson the active texture has been reset
    glActiveTexture(GL_TEXTURE0);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);

    // if for some reson the active texture has been reset
    glViewport(0, 0, mSize.x, mSize.y);
    
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindTexture(GL_TEXTURE_2D, mInputTexture );

    mShaderProgram.bind();

    if (mUpdateFn) {
        mUpdateFn();
    }

    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

    win.bindVBO();
    glClientActiveTexture(GL_TEXTURE0);

    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(0));

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(8));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    win.unbindVBO();
    ShaderProgram::unbind();
    glPopClientAttrib();
}

} // namespace sgct
