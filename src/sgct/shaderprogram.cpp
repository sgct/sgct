/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/shaderprogram.h>

#include <sgct/messagehandler.h>
#include <sgct/ogl_headers.h>

namespace {
    bool checkLinkStatus(GLint programId, const std::string& name) {
        GLint linkStatus;
        glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);

        if (!linkStatus) {
            GLint logLength;
            glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);

            std::vector<GLchar> log(logLength);
            glGetProgramInfoLog(programId, logLength, nullptr, log.data());

            sgct::MessageHandler::printError(
                "Shader program[%s] linking error: %s", name.c_str(), log.data()
            );
        }
        return linkStatus != 0;
    }
} // namespace

namespace sgct {

ShaderProgram::ShaderProgram(std::string name) : _name(std::move(name)) {}

void ShaderProgram::deleteProgram() {
    for (core::Shader& shader : _shaders) {
        if (shader.getId() > 0) {
            glDetachShader(_programId, shader.getId());
        }
    }
    _shaders.clear();

    glDeleteProgram(_programId);
    _programId = 0;
}

void ShaderProgram::addShaderSource(std::string src, GLenum type) {
    core::Shader v(type, std::move(src));
    _shaders.push_back(std::move(v));
}

void ShaderProgram::addShaderSource(std::string vertexSrc, std::string fragmentSrc) {
    addShaderSource(std::move(vertexSrc), GL_VERTEX_SHADER);
    addShaderSource(std::move(fragmentSrc), GL_FRAGMENT_SHADER);
}

int ShaderProgram::getAttribLocation(const std::string& name) const {
    return glGetAttribLocation(_programId, name.c_str());
}

int ShaderProgram::getUniformLocation(const std::string& name) const {
    return glGetUniformLocation(_programId, name.c_str());
}

void ShaderProgram::bindFragDataLocation(unsigned int colorNumber,
                                         const std::string& name) const
{
    glBindFragDataLocation(_programId, colorNumber, name.c_str());
}

std::string ShaderProgram::getName() const {
    return _name;
}

bool ShaderProgram::isLinked() const {
    return _isLinked;
}

int ShaderProgram::getId() const {
    return _programId;
}

void ShaderProgram::createAndLinkProgram() {
    if (_shaders.empty()) {
        throw std::runtime_error(
            "No shaders have been added the program: " + _name
        );
    }

    // Create the program
    bool createSuccess = createProgram();
    if (!createSuccess) {
        throw std::runtime_error("Error creating the program");
    }

    // Link shaders
    for (const core::Shader& shader : _shaders) {
        if (shader.getId() > 0) {
            glAttachShader(_programId, shader.getId());
        }
    }
    glLinkProgram(_programId);
    _isLinked = checkLinkStatus(_programId, _name);
    if (!_isLinked) {
        throw std::runtime_error("Error linking the program: " + _name);
    }
}

bool ShaderProgram::createProgram() {
    if (_programId > 0) {
        // If the program is already created don't recreate it.
        // but should only return true if it hasn't been linked yet.
        // if it has been linked already it can't be reused
        if (_isLinked) {
            MessageHandler::printError(
                "Could not create shader program [%s]: Already linked to shaders",
                _name.c_str()
            );
            return false;
        }

        // If the program is already created but not linked yet it can be reused
        return true;
    }

    _programId = glCreateProgram();

    if (_programId == 0) {
        MessageHandler::printError(
            "Could not create shader program [%s]: Unknown error", _name.c_str()
        );
        return false;
    }

    return true;
}

bool ShaderProgram::bind() const {
    // Make sure the program is linked before it can be used
    if (!_isLinked) {
        return false;
    }

    glUseProgram(_programId);
    return true;
}

void ShaderProgram::unbind() {
    glUseProgram(0);
}

} // namespace sgct
