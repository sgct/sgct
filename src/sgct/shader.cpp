/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/shader.h>

#include <sgct/ogl_headers.h>
#include <sgct/messagehandler.h>
#include <fstream>

namespace sgct::core {

Shader::Shader(ShaderType shaderType) : _shaderType(shaderType) {}

void Shader::setShaderType(ShaderType shaderType) {
    _shaderType = shaderType;
}

bool Shader::setSourceFromFile(const std::string& file) {
    // Make sure file can be opened
    std::ifstream shaderFile(file);

    if (!shaderFile.is_open()) {
        MessageHandler::instance()->printError(
            "Could not open %s file [%s]",
            getShaderTypeName(_shaderType).c_str(), file.c_str()
        );
        return false;
    }

    // Create needed resources by reading file length
    shaderFile.seekg(0, std::ios_base::end);
    std::streamoff fileSize = shaderFile.tellg();
    shaderFile.seekg(0, std::ios_base::beg);

    // Make sure the file is not empty
    if (fileSize == 0) {
        MessageHandler::instance()->printError(
            "Can't create source for %s: empty file [%s]",
            getShaderTypeName(_shaderType).c_str(), file.c_str()
        );
        return false;
    }

    // Copy file content to string
    std::vector<char> bytes(fileSize);
    shaderFile.read(bytes.data(), fileSize);
    std::string shaderSrc(bytes.data(), fileSize);
    shaderFile.close();

    // Compile shader source
    return setSourceFromString(shaderSrc);
}

bool Shader::setSourceFromString(const std::string& sourceString) {
    // At this point no resetting of shaders are supported
    if (_shaderId > 0) {
        MessageHandler::instance()->printWarning(
            "%s is already set for specified shader",
            getShaderTypeName(_shaderType).c_str()
        );
        return false;
    }

    // Prepare source code for shader
    const char* shaderSrc[] = { sourceString.c_str() };

    _shaderId = glCreateShader(_shaderType);
    glShaderSource(_shaderId, 1, shaderSrc, nullptr);

    // Compile and check status
    glCompileShader(_shaderId);

    return checkCompilationStatus();
}

void Shader::deleteShader() {
    glDeleteShader(_shaderId);
    _shaderId = 0;
}

int Shader::getId() const {
    return _shaderId;
}

bool Shader::checkCompilationStatus() const {
    GLint compilationStatus;
    glGetShaderiv(_shaderId, GL_COMPILE_STATUS, &compilationStatus);

    if (compilationStatus == 0) {
        GLint logLength;
        glGetShaderiv(_shaderId, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength == 0) {
            MessageHandler::instance()->printError(
                "%s compile error: Unknown error",
                getShaderTypeName(_shaderType).c_str()
            );
            return false;
        }

        std::vector<GLchar> log(logLength);
        glGetShaderInfoLog(_shaderId, logLength, nullptr, log.data());
        MessageHandler::instance()->printError(
            "%s compile error: %s", getShaderTypeName(_shaderType).c_str(), log.data()
        );

        return false;
    }

    return compilationStatus;
}

std::string Shader::getShaderTypeName(ShaderType shaderType) const {
    switch (shaderType) {
        case GL_VERTEX_SHADER:
            return "Vertex shader";
        case GL_FRAGMENT_SHADER:
            return "Fragment shader";
        case GL_GEOMETRY_SHADER:
            return "Geometry shader";
        case GL_COMPUTE_SHADER:
            return "Compute shader";
        case GL_TESS_CONTROL_SHADER:
            return "Tesselation control shader";
        case GL_TESS_EVALUATION_SHADER:
            return "Tesselation evaluation shader";
        default:
            return "Unknown shader";
    };
}

} // namespace sgct::core
