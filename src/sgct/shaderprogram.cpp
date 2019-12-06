/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/shaderprogram.h>

#include <sgct/error.h>
#include <sgct/log.h>
#include <sgct/ogl_headers.h>

#define Err(code, msg) Error(Error::Component::Shader, code, msg)

namespace {
    bool checkLinkStatus(GLint programId, const std::string& name) {
        GLint linkStatus;
        glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);

        if (linkStatus == 0) {
            GLint logLength;
            glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);

            std::vector<GLchar> log(logLength);
            glGetProgramInfoLog(programId, logLength, nullptr, log.data());

            sgct::Log::Error(
                "Shader program[%s] linking error: %s", name.c_str(), log.data()
            );
        }
        return linkStatus != 0;
    }
} // namespace

namespace sgct {

ShaderProgram::ShaderProgram(std::string name) : _name(std::move(name)) {}

ShaderProgram::ShaderProgram(ShaderProgram&& rhs) noexcept
    : _name(std::move(rhs._name))
    , _isLinked(rhs._isLinked)
    , _programId(rhs._programId)
    , _shaders(std::move(rhs._shaders))
{
    rhs._isLinked = false;
    rhs._programId = 0;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& rhs) noexcept {
    if (&rhs != this) {
        _name = std::move(rhs._name);
        _isLinked = rhs._isLinked;
        rhs._isLinked = false;
        _programId = rhs._programId;
        rhs._programId = 0;
        _shaders = std::move(rhs._shaders);
    }
    return *this;
}

void ShaderProgram::deleteProgram() {
    for (Shader& shader : _shaders) {
        if (shader.id() > 0) {
            glDetachShader(_programId, shader.id());
        }
    }
    _shaders.clear();

    glDeleteProgram(_programId);
    _programId = 0;
}

void ShaderProgram::addShaderSource(std::string src, GLenum type) {
    Shader v(type, std::move(src));
    _shaders.push_back(std::move(v));
}

void ShaderProgram::addShaderSource(std::string vertexSrc, std::string fragmentSrc) {
    addShaderSource(std::move(vertexSrc), GL_VERTEX_SHADER);
    addShaderSource(std::move(fragmentSrc), GL_FRAGMENT_SHADER);
}

std::string ShaderProgram::name() const {
    return _name;
}

bool ShaderProgram::isLinked() const {
    return _isLinked;
}

int ShaderProgram::id() const {
    return _programId;
}

void ShaderProgram::createAndLinkProgram() {
    if (_shaders.empty()) {
        throw Err(7010, "No shaders have been added to the program " + _name);
    }

    // Create the program
    createProgram();

    // Link shaders
    for (const Shader& shader : _shaders) {
        if (shader.id() > 0) {
            glAttachShader(_programId, shader.id());
        }
    }
    glLinkProgram(_programId);
    _isLinked = checkLinkStatus(_programId, _name);
    if (!_isLinked) {
        throw Err(7011, "Error linking the program " + _name);
    }
}

void ShaderProgram::createProgram() {
    if (_programId > 0) {
        // If the program is already created don't recreate it. But the function should
        // only return true if it hasn't been linked yet. If it has been already linked
        // it can't be reused
        if (_isLinked) {
            throw Err(
                7012, "Failed to create shader program " + _name + ": Already linked"
            );
        }

        // If the program is already created but not linked yet it can be reused
        return;
    }

    _programId = glCreateProgram();
    if (_programId == 0) {
        throw Err(7013, "Failed to create shader program " + _name + ": Unknown error");
    }
}

void ShaderProgram::bind() const {
    glUseProgram(_programId);
}

void ShaderProgram::unbind() {
    glUseProgram(0);
}

} // namespace sgct
