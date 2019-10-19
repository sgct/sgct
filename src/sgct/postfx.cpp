/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/postfx.h>

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/offscreenbuffer.h>

namespace sgct {

PostFX::PostFX(std::string name, const std::string& vertShaderSrc,
               const std::string& fragShaderSrc, std::function<void()> updateFunction)
    : _name(std::move(name))
    , _updateFunction(std::move(updateFunction))
{
    _shaderProgram.setName(_name);
    _shaderProgram.addShaderSource(vertShaderSrc, fragShaderSrc);
    if (!_shaderProgram.createAndLinkProgram()) {
        MessageHandler::printError("PostFX '%s' failed to link shader", _name.c_str());
    }
}

PostFX::~PostFX() {
    _shaderProgram.deleteProgram();
}

void PostFX::render() {
    Window& win = core::ClusterManager::instance()->getThisNode().getCurrentWindow();

    // bind target FBO
    win.getFBO()->attachColorTexture(_outputTexture);

    _size = win.getFramebufferResolution();

    glViewport(0, 0, _size.x, _size.y);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _inputTexture);

    _shaderProgram.bind();

    if (_updateFunction) {
        _updateFunction();
    }

    win.bindVAO();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    win.unbindVAO();

    ShaderProgram::unbind();
}

void PostFX::setInputTexture(unsigned int inputTex) {
    _inputTexture = inputTex;
}

void PostFX::setOutputTexture(unsigned int outputTex) {
    _outputTexture = outputTex;
}

unsigned int PostFX::getOutputTexture() const {
    return _outputTexture;
}

unsigned int PostFX::getInputTexture() const {
    return _inputTexture;
}

const ShaderProgram& PostFX::getShaderProgram() const {
    return _shaderProgram;
}

const std::string& PostFX::getName() const {
    return _name;
}

} // namespace sgct
