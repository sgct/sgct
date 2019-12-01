/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/postfx.h>

#include <sgct/window.h>

namespace sgct {

PostFX::PostFX(std::string name, const std::string& vertShaderSrc,
               const std::string& fragShaderSrc, std::function<void()> updateFunction)
    : _updateFunction(std::move(updateFunction))
    , _shaderProgram(name)
{
    _shaderProgram.addShaderSource(vertShaderSrc, fragShaderSrc);
    _shaderProgram.createAndLinkProgram();
}

PostFX::PostFX(PostFX&& rhs) noexcept {
    // (abock, 2019-10-28) I don't know why I had to manually write this, but if I do
    // =default in the header, VS 2017 complains the class is no longer move constructable
    // (shrug). If you think you can figure it out, make sure this works on VS 2017:
    // static_assert(std::is_nothrow_move_constructible_v<PostFX>);
    _updateFunction = std::move(rhs._updateFunction);
    _shaderProgram = std::move(rhs._shaderProgram);
    _inputTexture = std::move(rhs._inputTexture);
    _outputTexture = std::move(rhs._outputTexture);
}

PostFX::~PostFX() {
    _shaderProgram.deleteProgram();
}

void PostFX::render(Window& window) {
    // bind target FBO
    window.getFBO()->attachColorTexture(_outputTexture);

    glm::ivec2 size = window.getFramebufferResolution();
    glViewport(0, 0, size.x, size.y);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _inputTexture);

    _shaderProgram.bind();

    if (_updateFunction) {
        _updateFunction();
    }

    window.renderScreenQuad();
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

} // namespace sgct
