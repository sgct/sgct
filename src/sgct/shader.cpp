/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/Shader.h>

#include <sgct/ogl_headers.h>
#include <sgct/MessageHandler.h>
#include <fstream>

namespace sgct::core {

Shader::Shader(ShaderType shaderType) : mShaderType(shaderType) {}

void Shader::setShaderType(ShaderType shaderType) {
    mShaderType = shaderType;
}

bool Shader::setSourceFromFile(const std::string& file) {
    // Make sure file can be opened
    std::ifstream shaderFile(file);

    if (!shaderFile.is_open()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Could not open %s file[%s]\n",
            getShaderTypeName(mShaderType).c_str(), file.c_str()
        );
        return false;
    }

    // Create needed resources by reading file length
    shaderFile.seekg(0, std::ios_base::end);
    std::streamoff fileSize = shaderFile.tellg();
    shaderFile.seekg(0, std::ios_base::beg);

    // Make sure the file is not empty
    if (fileSize == 0) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Can't create source for %s: empty file [%s]\n",
            getShaderTypeName(mShaderType).c_str(), file.c_str()
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
    if (mShaderId > 0) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "%s is already set for specified shader\n",
            getShaderTypeName(mShaderType).c_str()
        );
        return false;
    }

    // Prepare source code for shader
    const char* shaderSrc[] = { sourceString.c_str() };

    mShaderId = glCreateShader(mShaderType);
    glShaderSource(mShaderId, 1, shaderSrc, nullptr);

    // Compile and check status
    glCompileShader(mShaderId);

    return checkCompilationStatus();
}

void Shader::deleteShader() {
    glDeleteShader(mShaderId);
    mShaderId = 0;
}

int Shader::getId() const {
    return mShaderId;
}

bool Shader::checkCompilationStatus() const {
    GLint compilationStatus;
    glGetShaderiv(mShaderId, GL_COMPILE_STATUS, &compilationStatus);

    if (compilationStatus == 0) {
        GLint logLength;
        glGetShaderiv(mShaderId, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength == 0) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "%s compile error: Unknown error\n",
                getShaderTypeName(mShaderType).c_str()
            );
            return false;
        }

        std::vector<GLchar> log(logLength);
        glGetShaderInfoLog(mShaderId, logLength, nullptr, log.data());
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "%s compile error: %s\n",
            getShaderTypeName(mShaderType).c_str(), log.data()
        );

        return false;
    }

    return compilationStatus == GL_TRUE;
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
