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

namespace {
    std::string getShaderTypeName(GLenum shaderType) {
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

    void checkCompilationStatus(GLenum type, GLint id) {
        GLint compilationStatus;
        glGetShaderiv(id, GL_COMPILE_STATUS, &compilationStatus);

        if (compilationStatus == 0) {
            GLint logLength;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLength);

            if (logLength == 0) {
                sgct::MessageHandler::printError(
                    "%s compile error: Unknown error", getShaderTypeName(type).c_str()
                );
            }

            std::vector<GLchar> log(logLength);
            glGetShaderInfoLog(id, logLength, nullptr, log.data());
            sgct::MessageHandler::printError(
                "%s compile error: %s", getShaderTypeName(type).c_str(), log.data()
            );
        }
    }
} // namespace

namespace sgct::core {

Shader::Shader(GLenum shaderType, const std::string& sourceString)
    : _shaderType(shaderType)
{
    // Prepare source code for shader
    const char* shaderSrc[] = { sourceString.c_str() };

    _shaderId = glCreateShader(_shaderType);
    glShaderSource(_shaderId, 1, shaderSrc, nullptr);

    glCompileShader(_shaderId);
    checkCompilationStatus(_shaderType, _shaderId);
}

// Shader::Shader(Shader&& rhs)
//     : _shaderId(rhs._shaderId)
// {
//     _shaderId = 0;
// }

Shader::~Shader() {
    glDeleteShader(_shaderId);
}

// Shader& Shader::operator=(Shader&& rhs) {
//     if (&rhs == this) {
//         return *this;
//     }

//     _shaderId = rhs._shaderId;
//     rhs._shaderId = 0;
//     return *this;
// }

int Shader::getId() const {
    return _shaderId;
}


} // namespace sgct::core
