/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/shaderprogram.h>

#include <sgct/error.h>
#include <sgct/log.h>
#include <sgct/opengl.h>
#include <fmt/format.h>

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
                fmt::format("Shader [{}] linking error: {}", name, log.data())
            );
        }
        return linkStatus != 0;
    }

    std::string shaderTypeName(GLenum shaderType) {
        switch (shaderType) {
            case GL_VERTEX_SHADER:   return "Vertex shader";
            case GL_FRAGMENT_SHADER: return "Fragment shader";
            case GL_GEOMETRY_SHADER: return "Geometry shader";
            default:                 throw std::logic_error("Unhandled case label");
        };
    }

    void checkCompilationStatus(GLenum type, GLint id) {
        GLint compilationStatus;
        glGetShaderiv(id, GL_COMPILE_STATUS, &compilationStatus);

        if (compilationStatus == 0) {
            GLint logLength;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLength);

            if (logLength == 0) {
                sgct::Log::Error(fmt::format(
                    "{} compile error: Unknown error", shaderTypeName(type)
                ));
            }

            std::vector<GLchar> log(logLength);
            glGetShaderInfoLog(id, logLength, nullptr, log.data());
            sgct::Log::Error(fmt::format(
                "{} compile error: {}", shaderTypeName(type), log.data()
            ));
        }
    }
} // namespace

namespace sgct {

ShaderProgram::Shader::Shader(GLenum shaderType, const std::string& sourceString)
    : _shaderType(shaderType)
{
    // Prepare source code for shader
    const char* shaderSrc[] = { sourceString.c_str() };

    _shaderId = glCreateShader(_shaderType);
    glShaderSource(_shaderId, 1, shaderSrc, nullptr);

    glCompileShader(_shaderId);
    checkCompilationStatus(_shaderType, _shaderId);
}

ShaderProgram::Shader::Shader(const Shader& rhs)
    : _shaderType(rhs._shaderType)
    , _shaderId(rhs._shaderId)
{}

ShaderProgram::Shader::Shader(Shader&& rhs) noexcept
    : _shaderType(rhs._shaderType)
    , _shaderId(rhs._shaderId)
{
    rhs._shaderId = 0;
}

ShaderProgram::Shader& ShaderProgram::Shader::operator=(const Shader& rhs) noexcept {
    if (this != &rhs) {
        _shaderId = rhs._shaderId;
    }
    return *this;
}
ShaderProgram::Shader& ShaderProgram::Shader::operator=(Shader&& rhs) noexcept {
    if (this != &rhs) {
        _shaderId = rhs._shaderId;
        rhs._shaderId = 0;
    }
    return *this;
}

ShaderProgram::Shader::~Shader() {
    glDeleteShader(_shaderId);
}

int ShaderProgram::Shader::id() const {
    return _shaderId;
}

ShaderProgram::ShaderProgram(std::string name) : _name(std::move(name)) {}

ShaderProgram::ShaderProgram(ShaderProgram&& rhs) noexcept
    : _name(std::move(rhs._name))
    , _programId(rhs._programId)
    , _shaders(std::move(rhs._shaders))
{
    rhs._programId = 0;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& rhs) noexcept {
    if (&rhs != this) {
        _name = std::move(rhs._name);
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

int ShaderProgram::id() const {
    return _programId;
}

void ShaderProgram::createAndLinkProgram() {
    if (_shaders.empty()) {
        throw Err(
            7010,
            fmt::format("No shaders have been added to the program {}", _name)
        );
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
    bool isLinked = checkLinkStatus(_programId, _name);
    if (!isLinked) {
        throw Err(7011, fmt::format("Error linking the program {}", _name));
    }
}

void ShaderProgram::createProgram() {
    if (_programId > 0) {
        throw Err(
            7012,
            fmt::format("Failed to create shader program {}: Already created", _name)
        );
    }

    _programId = glCreateProgram();
    if (_programId == 0) {
        throw Err(
            7013,
            fmt::format("Failed to create shader program {}: Unknown error", _name)
        );
    }
}

void ShaderProgram::bind() const {
    glUseProgram(_programId);
}

void ShaderProgram::unbind() {
    glUseProgram(0);
}

} // namespace sgct
