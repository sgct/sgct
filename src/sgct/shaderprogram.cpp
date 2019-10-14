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

namespace sgct {

ShaderProgram::ShaderProgram(std::string name) : _name(std::move(name)) {}

void ShaderProgram::deleteProgram() {
    for (core::ShaderData& sd : _shaders) {
        if (sd.shader.getId() > 0) {
            glDetachShader(_programId, sd.shader.getId());
            sd.shader.deleteShader();
        }
    }

    glDeleteProgram(_programId);
    _programId = 0;
}

void ShaderProgram::setName(std::string name) {
    _name = std::move(name);
}

bool ShaderProgram::addShaderSrc(std::string src, core::Shader::ShaderType type,
                                 ShaderSourceType sSrcType)
{
    core::ShaderData sd;
    sd.shader.setShaderType(type);
    sd.isSrcFile = sSrcType == ShaderSourceType::File;
    sd.source = std::move(src);

    bool success;
    if (sd.isSrcFile) {
        success = sd.shader.setSourceFromFile(sd.source);
    }
    else {
        success = sd.shader.setSourceFromString(sd.source);
    }

    if (success) {
        _shaders.push_back(std::move(sd));
    }

    return success;
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

bool ShaderProgram::createAndLinkProgram() {
    if (_shaders.empty()) {
        MessageHandler::printError(
            "ShaderProgram: No shaders has been added to program '%s'", _name.c_str()
        );
        return false;
    }

    // Create the program
    bool createSuccess = createProgram();
    if (!createSuccess) {
        // Error text handled in createProgram()
        return false;
    }

    // Link shaders
    for (const core::ShaderData& sd : _shaders) {
        if (sd.shader.getId() > 0) {
            glAttachShader(_programId, sd.shader.getId());
        }
    }

    glLinkProgram(_programId);

    _isLinked = checkLinkStatus();
    return _isLinked;
}

bool ShaderProgram::reload() {
    MessageHandler::printInfo("ShaderProgram: Reloading program '%s'", _name.c_str());
    
    deleteProgram();

    for (core::ShaderData& sd : _shaders) {
        bool success;
        if (sd.isSrcFile) {
            success = sd.shader.setSourceFromFile(sd.source);
        }
        else {
            success = sd.shader.setSourceFromString(sd.source);
        }

        if (!success) {
            MessageHandler::printError(
                "ShaderProgram: Failed to load '%s'", sd.source.c_str()
            );
            return false;
        }
    }

    return createAndLinkProgram();
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

bool ShaderProgram::checkLinkStatus() const {
    GLint linkStatus;
    glGetProgramiv(_programId, GL_LINK_STATUS, &linkStatus);

    if (!linkStatus) {
        GLint logLength;
        glGetProgramiv(_programId, GL_INFO_LOG_LENGTH, &logLength);

        std::vector<GLchar> log(logLength);
        glGetProgramInfoLog(_programId, logLength, nullptr, log.data());

        MessageHandler::printError(
            "Shader program[%s] linking error: %s", _name.c_str(), log.data()
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
